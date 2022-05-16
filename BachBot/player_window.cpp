/**
 * @file player_window.cpp
 * @brief Application main window
 * @copyright
 * 2022 Andrew Buettner (ABi)
 *
 * @section LICENSE
 *
 * BachBot - A hymn Midi player for Schlicker organs
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


//  system includes
#include <algorithm>  //  std::foreach
#include <stdexcept>  //  std::runtime_error. std::out_of_range
#include <string>  //  std::string
#include <string_view>  //  sv, std::swap
#include <array>  //  std::array
#include <fmt/xchar.h>  //  fmt::format(L
#include <fmt/format.h>  //  fmt::format
#include <wx/xml/xml.h>  //  wxXml API

//  module includes
// -none-

//  local includes
#include "player_window.h"  //  local include
#include "player_thread.h"  //  PlayerThread
#include "organ_midi_event.h"  //  OrganMidiEvent, BankConfig
#include "syndyne_importer.h"  //  SyndineImporter
#include "playlist_loader.h"  //  PlaylistLoader


namespace {
    using namespace std::literals::string_view_literals;
    constexpr const auto EDITION = L"Ascention"sv;

    constexpr const auto NOW_PLAYING_LEN = 40U;
    constexpr const auto UP_NEXT_LEN = 38U;

    enum AcceleratorEntries : size_t
    {
        MOVE_UP_ACCEL = 0U,
        MOVE_DOWN_ACCEL,
        PLAY_NEXT_ACCEL1,
        PLAY_NEXT_ACCEL2,
        NUM_ACCEL_ENTRIES
    };
    
    //wxAcceleratorEntry g_accel_entries[2U];
    std::array<wxAcceleratorEntry, NUM_ACCEL_ENTRIES> g_accel_entries;
}  //  end anonymous namespace


namespace bach_bot {
namespace ui {

void initialize_global_accelerator_table()
{
    g_accel_entries[MOVE_UP_ACCEL].Set(wxACCEL_CTRL, WXK_UP,
                                       PlayerWindowEvents::MOVE_UP_EVENT);
    g_accel_entries[MOVE_DOWN_ACCEL].Set(wxACCEL_CTRL, WXK_DOWN,
                                         PlayerWindowEvents::MOVE_DOWN_EVENT);
    g_accel_entries[PLAY_NEXT_ACCEL1].Set(wxACCEL_CTRL, WXK_RETURN,
                                          PlayerWindowEvents::SET_NEXT_EVENT);
    g_accel_entries[PLAY_NEXT_ACCEL2].Set(wxACCEL_CTRL, WXK_NUMPAD_ENTER,
                                          PlayerWindowEvents::SET_NEXT_EVENT);
}


PlayerWindow::PlayerWindow() :
    MainWindow(nullptr),
    wxLog(),
    m_counter{0U},
    m_player_thread(),
    m_midi_devices(),
    m_midi_out(),
    m_current_device_id{0U},
    m_current_song_event_count{0U},
    m_current_song_id{0U},
    m_next_song_id{0U},
    m_song_list(0U, 0U),
    m_song_labels(),
    m_current_config{0U, 0U},
    m_playlist_name(),
    m_playlist_changed{false},
    m_selected_control{nullptr},
    m_accel_table(int(NUM_ACCEL_ENTRIES), g_accel_entries.data()),
    m_ui_animation_timer(this, PlayerWindowEvents::UI_ANIMATE_TICK),
    m_up_next_label(next_label, UP_NEXT_LEN),
    m_playing_label(track_label, NOW_PLAYING_LEN)
{
    for (auto i = 0U; i < m_midi_out.getPortCount(); ++i) {
        m_midi_devices.emplace_back(
            device_select, wxID_ANY, wxString(m_midi_out.getPortName(i)),
            wxEmptyString, wxITEM_RADIO
        );
        device_select->Append(&m_midi_devices.back());
        device_select->Bind(wxEVT_COMMAND_MENU_SELECTED, 
                            [=](wxCommandEvent&) { on_device_changed(i); },
                            m_midi_devices.back().GetId());
    }
    m_midi_devices.front().Check();
    header_container->Show(false);
    layout_scroll_panel();

    wxAcceleratorEntry accel_entries[2U];
    accel_entries[0].Set(wxACCEL_CTRL, WXK_UP,
                         PlayerWindowEvents::MOVE_UP_EVENT);
    accel_entries[1].Set(wxACCEL_CTRL, WXK_DOWN,
                         PlayerWindowEvents::MOVE_DOWN_EVENT);

    SetAcceleratorTable(m_accel_table);
    m_ui_animation_timer.Start(LabelAnimator::RECOMMENDED_TICK_MS);
}


void PlayerWindow::on_play_advance(wxCommandEvent &event)
{
    static_cast<void>(event);
    if (nullptr == m_player_thread.get()) {
        start_player_thread();
    } else {
        m_player_thread->signal_advance();
    }
}


void PlayerWindow::on_stop(wxCommandEvent &event)
{
    static_cast<void>(event);
    if (nullptr != m_player_thread) {
        m_player_thread->signal_stop();
        m_player_thread->Wait();
    }
}


void PlayerWindow::on_new_playlist(wxCommandEvent &event)
{
    static_cast<void>(event);
    if (m_playlist_changed) {
        wxMessageDialog confirm_dialog(
            this,  wxT("Playlist not saved.  Clear playlist?"),
            wxT("Confirm clear"),
            wxOK|wxCANCEL|wxCANCEL_DEFAULT|wxICON_WARNING);
        if (confirm_dialog.ShowModal() != wxID_OK) {
            return;
        }
    }

    clear_playlist_window();
}


void PlayerWindow::on_load_playlist(wxCommandEvent &event)
{
    wxFileDialog open_dialog(this, "Open Playlist", "", "", 
                             "BachBot Playlist|*.bbp", 
                             wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (open_dialog.ShowModal() == wxID_CANCEL) {
        return;
    }

    if (m_playlist_changed) {
        wxMessageDialog confirm_dialog(
            this,  wxT("Clear current playlist?"), wxT("Confirm clear"),
            wxYES|wxNO|wxCANCEL|wxCANCEL_DEFAULT|wxICON_WARNING);
        const auto response = confirm_dialog.ShowModal();
        if (wxCANCEL == response) {
            return;
        } else if (wxYES == response) {
            on_save_playlist(event);
        }
    }

    PlaylistXmlLoader loader(this, open_dialog.GetPath());
    if (loader.ShowModal() != wxID_OK) {
        wxMessageBox(fmt::format(L"Error loading playlist:\n"
                                  "Error reported was: {}",
                                 loader.get_error_text().value()));
        return;
    }

    clear_playlist_window();
    const auto playlist = loader.get_playlist();
    if (playlist.size() > 0U) {
        for (const auto &i: playlist) {
            add_playlist_entry(i);
        }
        layout_scroll_panel();
    }

    m_playlist_name = open_dialog.GetPath();
}


void PlayerWindow::on_save_playlist(wxCommandEvent &event)
{
    if (!m_playlist_name.has_value()) {
        on_save_as(event);
    } else if (m_playlist_changed) {
        wxXmlDocument playlist_doc;
        auto playlist_root = new wxXmlNode(nullptr, 
                                           wxXML_ELEMENT_NODE,
                                           wxT("BachBot_Playlist"));

        playlist_doc.SetRoot(playlist_root);
        auto song_id = m_song_list.first;
        auto order = 0U;
        while (song_id > 0U) {
            auto playlist_entry = new wxXmlNode(playlist_root, 
                                                wxXML_ELEMENT_NODE, 
                                                wxT("song"));

            const auto *const song = m_song_labels[song_id].get();
            song->save_config(playlist_entry);
            playlist_entry->AddAttribute(wxT("order"),
                                         wxString::Format(wxT("%u"), ++order));

            song_id = song->get_sequence().second;
        }

        playlist_doc.Save(m_playlist_name.value());
        m_playlist_changed = false;
    }
}


void PlayerWindow::on_open_midi(wxCommandEvent &event)
{
    wxFileDialog open_dialog(this, "Open MIDI File", "", "", 
                             "MIDI Files|*.mid", 
                             wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (open_dialog.ShowModal() == wxID_CANCEL) {
        return;
    }

    PlayListEntry song_entry;
    song_entry.song_id = uint32_t(m_song_labels.size()) + 1U;
    auto importer = std::make_unique<SyndineImporter>(
        open_dialog.GetPath().ToStdString(), song_entry.song_id);

    song_entry.file_name = open_dialog.GetPath();
    song_entry.tempo_detected = importer->get_tempo();
    auto tempo = DEFAULT_NO_TEMPO;
    if (song_entry.tempo_detected.has_value()) {
        tempo = song_entry.tempo_detected.value();
    }

    LoadMidiDialog import_dialog(this);
    import_dialog.file_name_label->SetLabelText(open_dialog.GetPath());
    import_dialog.tempo_label->SetLabelText(fmt::format(L"{}bpm", tempo));
    import_dialog.select_tempo->SetValue(tempo);

    std::optional<wxString> error_text;
    do {
        if (error_text.has_value()) {
            wxMessageBox(error_text.value(), "Form Error",
                         wxOK | wxICON_INFORMATION);
        }

        if (import_dialog.ShowModal() == wxID_CANCEL) {
            return;
        }

        error_text = song_entry.load_config(import_dialog);
    } while (error_text.has_value());

    static_cast<void>(song_entry.import_midi(importer.get()));
    add_playlist_entry(song_entry);
    layout_scroll_panel();
    m_song_list.second = song_entry.song_id;
    m_playlist_changed = true;
}


void PlayerWindow::on_quit(wxCommandEvent &event)
{
    if (pre_close_check(event)) {
        Close(true);
    }
}


void PlayerWindow::on_about(wxCommandEvent &event)
{
    wxMessageBox(fmt::format(
        L"BachBot MIDI player for Schlicker Organs \"{}\" edition:\n\n"
         "BachBot is a MIDI player intended Schlicker Pipe Organs or other "
         "Organs using the Syndyne Console Control system.\n"
         "Written By Andrew Buettner for Zion Lutheran Church and School "
         "Hartland, WI\n"
         "https://www.github.com/KB3HNS/BachBot", EDITION),
        wxT("About BachBot"), wxOK | wxICON_INFORMATION);
}


void PlayerWindow::on_thread_tick(wxThreadEvent &event)
{
    ++m_counter;
    auto events_complete = 0;
    if (m_current_song_event_count > 0U) {
        events_complete = int(m_current_song_event_count)-event.GetInt();
    }
    event_count->SetValue(events_complete);
}


void PlayerWindow::on_thread_exit(wxThreadEvent &event)
{
    static_cast<void>(event);
    m_player_thread.reset();
    m_current_song_event_count = 0U;
    m_current_song_id = 0U;

    event_count->SetValue(0);
    m_playing_label.set_label_text(L"Not Playing");
    std::for_each(m_midi_devices.begin(), m_midi_devices.end(),
                  [](wxMenuItem& i) { i.Enable(); });

    new_playlist_menu->Enable();
    load_playlist_menu->Enable();

    if (0U != m_next_song_id) {
        m_song_labels[m_next_song_id]->reset_status();
    }
}


void PlayerWindow::on_device_changed(const uint32_t device_id)
{
    m_current_device_id = device_id;
}


void PlayerWindow::on_bank_changed(wxThreadEvent &event)
{
    const auto current_setting = uint32_t(event.GetInt());
    const auto bank = current_setting % 8U;
    const auto mode = current_setting / 8U;
    bank_label->SetLabelText(wxString(fmt::format(L"{}", bank + 1U)));
    mode_label->SetLabelText(wxString(fmt::format(L"{}", mode + 1U)));
    m_current_config = std::make_pair(uint8_t(bank), mode);
}


void PlayerWindow::on_song_starts_playing(wxThreadEvent &event)
{
    event_count->SetValue(0);
    const auto song_id = uint32_t(event.GetInt());
    if (0U != m_current_song_id) {
        m_song_labels[m_current_song_id]->reset_status();
    }

    if (song_id > 0U) {
        auto *const song_data = m_song_labels[song_id].get();
        m_current_song_id = song_id;
        set_next_song(song_data->get_sequence().second);
        m_current_song_event_count = song_data->get_song_events().size();
        event_count->SetRange(int(m_current_song_event_count));
        m_playing_label.set_label_text(song_data->get_filename());
        song_data->set_playing();

    }
}


void PlayerWindow::on_song_done_playing(wxThreadEvent &event)
{
    if (0U != m_current_song_id) {
        m_song_labels[m_current_song_id]->reset_status();
    }
    if (event.GetInt() == 0) {
        set_next_song(m_current_song_id);
    }
    m_current_song_id = 0U;
}


void PlayerWindow::on_accel_down_event(wxCommandEvent &event)
{
    static_cast<void>(event);
    if (nullptr != m_selected_control) {
        const auto sequence = m_selected_control->get_sequence();
        if (0U != sequence.second) {
            on_move_event(m_selected_control->get_song_id(),
                          m_selected_control, false);
        }
    }
}


void PlayerWindow::on_accel_up_event(wxCommandEvent & event)
{
    static_cast<void>(event);
    if (nullptr != m_selected_control) {
        const auto sequence = m_selected_control->get_sequence();
        if (0U != sequence.first) {
            on_move_event(m_selected_control->get_song_id(),
                          m_selected_control, true);
        }
    }
}


void PlayerWindow::on_accel_play_next_event(wxCommandEvent &event)
{
    static_cast<void>(event);
    if (nullptr != m_selected_control) {
        set_next_song(m_selected_control->get_song_id());
    }
}


void PlayerWindow::on_timer_tick(wxTimerEvent &event)
{
    static_cast<void>(event);
    m_up_next_label.animate_tick();
    m_playing_label.animate_tick();
}


void PlayerWindow::on_move_event(const uint32_t song_id,
                                 PlaylistEntryControl *control, 
                                 const bool direction)
{
    const auto sequence = control->get_sequence();
    const auto next_song_id = (direction ? sequence.first : sequence.second);
    auto other_control = m_song_labels[next_song_id].get();
    control->swap(other_control);
    std::swap(m_song_labels[song_id], m_song_labels[next_song_id]);
    if (direction) {
        std::swap(control, other_control);
    }

    //  order is now prev -> control -> other_control -> next
    const auto prev_sequence = control->get_sequence().first;
    const auto next_sequence = other_control->get_sequence().second;
    if (0U != prev_sequence) {
        m_song_labels[prev_sequence]->set_sequence(-1, control->get_song_id());
    } else {
        m_song_list.first = control->get_song_id();
    }

    if (0U != next_sequence) {
        m_song_labels[next_sequence]->set_sequence(other_control->get_song_id());
    } else {
        m_song_list.second = other_control->get_song_id();
    }

    if (0U != m_current_song_id) {
        const auto sequence = m_song_labels[m_current_song_id]->get_sequence();
        if (sequence.second != m_next_song_id) {
            if (0U != m_next_song_id) {
                m_song_labels[m_next_song_id]->reset_status();
            }
            set_next_song(sequence.second);
        }
    }

    m_playlist_changed = true;
}


void PlayerWindow::on_manual_advance(wxCommandEvent &event)
{
    static_cast<void>(event);
    send_manual_message(SyndyneBankCommands::NEXT_BANK);
}


void PlayerWindow::on_manual_prev(wxCommandEvent &event)
{
    static_cast<void>(event);
    send_manual_message(SyndyneBankCommands::PREV_BANK);
}


void PlayerWindow::on_manual_cancel(wxCommandEvent &event)
{
    static_cast<void>(event);
    send_manual_message(SyndyneBankCommands::GENERAL_CANCEL);
}


void PlayerWindow::on_close(wxCloseEvent &event)
{
    if (event.CanVeto()) {
        wxCommandEvent command_evt;
        if (!pre_close_check(command_evt)) {
            event.Veto();
            return;
        }
    }

    if (m_player_thread.get() != nullptr) {
        m_player_thread->signal_stop();
        m_player_thread->Wait();
        wxMilliSleep(10UL);
    }

    MainWindow::on_close(event);
}


void PlayerWindow::on_save_as(wxCommandEvent &event)
{
    wxFileDialog save_dialog(this, "Save Playlist", "", "", 
                             "BachBot Playlist|*.bbp", 
                             wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (save_dialog.ShowModal() == wxID_CANCEL) {
        return;
    }

    m_playlist_name = save_dialog.GetPath();
    m_playlist_changed = true;
    on_save_playlist(event);
}


void PlayerWindow::on_drop_midi_file(wxDropFilesEvent &event)
{
    PlaylistDndLoader loader(this, event, uint32_t(m_song_labels.size()) + 1U);
    if (loader.ShowModal() != wxID_OK) {
        wxMessageBox(fmt::format(L"Error with import:\n"
                                 "Error reported was: {}",
                                 loader.get_error_text().value()));
        return;
    }

    const auto playlist = loader.get_playlist();
    if (playlist.size() > 0U) {
        std::for_each(playlist.begin(), playlist.end(),
                      [=](const PlayListEntry &i) {
            add_playlist_entry(i);
        });
        layout_scroll_panel();
        m_playlist_changed = true;
    }
}


void PlayerWindow::send_manual_message(const SyndyneBankCommands value)
{
    const auto port_open = m_midi_out.isPortOpen();
    if (!port_open) {
        m_midi_out.openPort(m_current_device_id);
    }

    send_bank_change_message(m_midi_out, value);

    if (!port_open) {
        m_midi_out.closePort();
    }
}


void PlayerWindow::clear_playlist_window()
{
    std::for_each(m_song_labels.begin(), m_song_labels.end(),
                  [=](std::pair<const uint32_t, PlaylistEntryType> &label) {
        playlist_container->Detach(label.second.get());
    });

    m_song_labels.clear();
    m_up_next_label.set_label_text(wxT(""));
    static_cast<void>(playlist_label->Show(true));
    layout_scroll_panel();
    m_song_list = std::make_pair(0U, 0U);
    m_next_song_id = 0U;
    static_cast<void>(header_container->Show(false));
}


void PlayerWindow::add_playlist_entry(const PlayListEntry &song)
{
    const auto playing_last = (m_current_song_id == m_song_list.second);
    auto p_label = std::make_unique<PlaylistEntryControl>(playlist_panel, song);
    if (0U == m_song_list.first) {
        m_song_list.first = song.song_id;
        static_cast<void>(playlist_label->Show(false));
    } else {
        auto *const prev_song = m_song_labels[m_song_list.second].get();
        prev_song->set_sequence(-1, int(song.song_id));
    }

    static_cast<void>(playlist_container->Add(p_label.get(), 0, wxALL, 5));
    header_container->Show(true);
    p_label->set_sequence(int(m_song_list.second));
    m_song_list.second = song.song_id;

    p_label->move_event = [=](uint32_t song_id,
                              PlaylistEntryControl *control,
                              bool direction) {
        on_move_event(song_id, control, direction);
    };

    p_label->checkbox_event = [=](uint32_t song_id,
                                  PlaylistEntryControl*,
                                  bool checked) {
        if (song_id == m_current_song_id && m_player_thread.get() != nullptr) {
            if (checked && (0U != m_next_song_id)) {
                auto control = m_song_labels[m_next_song_id].get();
                control->set_next();
                m_player_thread->enqueue_next_song(control->get_song_events());
            } else {
                m_player_thread->enqueue_next_song({});
                if (0U != m_next_song_id) {
                    m_song_labels[m_next_song_id]->reset_status();
                }
            }
        }

        m_playlist_changed = true;
    };

    p_label->set_next_event = [=](uint32_t song_id,
                                  PlaylistEntryControl*,
                                  bool) {
        set_next_song(song_id);
    };

    p_label->selected_event = [=](uint32_t song_id,
                                  PlaylistEntryControl *control,
                                  bool selected) {
        if (selected) {
            for (auto &i: m_song_labels) {
                if (i.first != song_id) {
                    i.second->deselect();
                }
            }
            m_selected_control = control;
        }
    };

    m_song_labels[song.song_id] = std::move(p_label);

    if ((song.song_id == m_song_list.first) || playing_last) {
        set_next_song(song.song_id);
    }
}


void PlayerWindow::layout_scroll_panel() const
{
    playlist_panel->Layout();
    const auto size = playlist_panel->GetBestVirtualSize();
    playlist_panel->SetVirtualSize(size);
    playlist_panel->Refresh();
}


void PlayerWindow::set_next_song(const uint32_t song_id)
{
    if (0U != m_next_song_id) {
        m_song_labels[m_next_song_id]->reset_status();
    }
    m_next_song_id = song_id;
    if (0U != song_id) {
        const auto next_song = m_song_labels[song_id].get();
        m_up_next_label.set_label_text(next_song->get_filename());
        const auto cur_song = m_song_labels.find(m_current_song_id);
        if ((m_song_labels.end() != cur_song) && 
            cur_song->second->get_autoplay() &&
            (nullptr != m_player_thread.get()))
        {
            next_song->set_next();
            m_player_thread->enqueue_next_song(
                next_song->get_song_events());
        } else {
            next_song->reset_status();
        }
    } else {
        m_up_next_label.set_label_text(wxT(""));
    }
}


bool PlayerWindow::pre_close_check(wxCommandEvent &event)
{
    if (m_playlist_changed) {
        wxMessageDialog confirm_dialog(
            this,  wxT("Playlist not saved.  Save before exit?"),
            wxT("Confirm exit"),
            wxYES|wxNO|wxCANCEL|wxCANCEL_DEFAULT|wxICON_WARNING);
        const auto response = confirm_dialog.ShowModal();
        if (wxID_CANCEL == response) {
            return false;
        } else if (wxID_YES == response) {
            on_save_playlist(event);
        }
    }

    return true;
}


void PlayerWindow::start_player_thread()
{
    m_player_thread = std::make_unique<PlayerThread>(this, m_midi_out);
    m_midi_out.openPort(m_current_device_id);
    m_player_thread->set_bank_config(m_current_config.first, 
                                     m_current_config.second);

    if (0U != m_next_song_id) {
        auto control = m_song_labels[m_next_song_id].get();
        m_player_thread->enqueue_next_song(
            control->get_song_events());
    } else {
        m_player_thread->enqueue_next_song(
            generate_test_pattern());
    }

    m_player_thread->play();
    std::for_each(m_midi_devices.begin(), m_midi_devices.end(), 
                  [](wxMenuItem &i) { i.Enable(false); });

    new_playlist_menu->Enable(false);
    load_playlist_menu->Enable(false);
}


PlayerWindow::~PlayerWindow()
{
    wxCommandEvent e;
    on_stop(e);
    std::for_each(m_midi_devices.begin(), m_midi_devices.end(), 
                  [=](wxMenuItem &i) { device_select->Remove(&i); });

    clear_playlist_window();
}


wxBEGIN_EVENT_TABLE(PlayerWindow, wxFrame)
    EVT_THREAD(PlayerWindowEvents::TICK_EVENT, PlayerWindow::on_thread_tick)
    EVT_THREAD(PlayerWindowEvents::SONG_START_EVENT, 
               PlayerWindow::on_song_starts_playing)
    EVT_THREAD(PlayerWindowEvents::BANK_CHANGE_EVENT, PlayerWindow::on_bank_changed)
    EVT_THREAD(PlayerWindowEvents::SONG_END_EVENT, 
               PlayerWindow::on_song_done_playing)
    EVT_THREAD(PlayerWindowEvents::EXIT_EVENT, PlayerWindow::on_thread_exit)
    EVT_MENU(PlayerWindowEvents::MOVE_DOWN_EVENT, PlayerWindow::on_accel_down_event)
    EVT_MENU(PlayerWindowEvents::MOVE_UP_EVENT, PlayerWindow::on_accel_up_event)
    EVT_MENU(PlayerWindowEvents::SET_NEXT_EVENT, PlayerWindow::on_accel_play_next_event)
    EVT_TIMER(PlayerWindowEvents::UI_ANIMATE_TICK, PlayerWindow::on_timer_tick)
wxEND_EVENT_TABLE()

}  //  end ui
}  //  end bach_bot
