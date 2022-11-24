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
    constexpr const auto EDITION = L"Reformation R1"sv;

    constexpr const auto NOW_PLAYING_LEN = 78U;
    constexpr const auto UP_NEXT_LEN = 76U;

    enum AcceleratorEntries : size_t
    {
        MOVE_UP_ACCEL = 0U,
        MOVE_DOWN_ACCEL,
        PLAY_NEXT_ACCEL1,
        PLAY_NEXT_ACCEL2,
        PLAY_ACTIVATE_ACCEL,
        NUM_ACCEL_ENTRIES
    };

    std::array<wxAcceleratorEntry, NUM_ACCEL_ENTRIES> g_accel_entries;

    constexpr const auto image_name = L"wood.png"sv;

    std::optional<wxString> s_window_title;
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
    g_accel_entries[PLAY_ACTIVATE_ACCEL].Set(
        0, WXK_F5, PlayerWindowEvents::PLAY_ACTIVATE_EVENT);
}


PlayerWindow::PlayerWindow() :
    MainWindow(nullptr),
    wxLog(),
    m_player_thread(),
    m_midi_devices(),
    m_midi_out(),
    m_current_device_id{0U},
    m_current_song_event_count{0U},
    m_current_song_id{0U},
    m_next_song_id{0U, false},
    m_song_list(0U, 0U),
    m_song_labels(),
    m_current_config(),
    m_playlist_name(),
    m_playlist_changed{false},
    m_selected_control{nullptr},
    m_accel_table(int(NUM_ACCEL_ENTRIES), g_accel_entries.data()),
    m_ui_animation_timer(this, PlayerWindowEvents::UI_ANIMATE_TICK),
    m_up_next_label(next_label, UP_NEXT_LEN),
    m_playing_label(track_label, NOW_PLAYING_LEN),
    m_background(image_name.data()),
    m_sync_config{false}
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

#ifdef _WIN32
    playlist_panel->ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_ALWAYS);
    wxIcon icon;
    const auto loaded = load_image(icon, wxBITMAP_TYPE_ICO, "BachBot.ico");
    assert(loaded);
    assert(icon.IsOk());
    SetIcons(icon);
#endif

    PushEventHandler(&m_background);
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

    m_playlist_name.reset();
    clear_playlist_window();
    update_window_title(false);
}


void PlayerWindow::on_load_playlist(wxCommandEvent &event)
{
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

    wxFileDialog open_dialog(this, "Open Playlist", "", "",
                             "BachBot Playlist|*.bbp",
                             wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    [[maybe_unused]] auto s1 = m_staticline1->GetSize();
    [[maybe_unused]] auto s2 = playlist_label->GetSize();

    if (open_dialog.ShowModal() == wxID_CANCEL) {
        return;
    }

    PlaylistXmlLoader loader(this, open_dialog.GetPath());
    loader.set_on_success_callback([&](std::list<PlayListEntry> playlist) {
        clear_playlist_window();
        if (playlist.size() > 0U) {
            for (const auto &i: playlist) {
                add_playlist_entry(i);
            }
            layout_scroll_panel();
        }

        m_playlist_name = open_dialog.GetPath();
        update_window_title(false);
    });

    if (loader.ShowModal() != wxID_OK) {
        wxMessageBox(fmt::format(L"Error loading playlist:\n"
                                  "Error reported was: {}",
                                 loader.get_error_text().value()));
        return;
    }
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
        update_window_title(false);
    }
}


void PlayerWindow::on_open_midi(wxCommandEvent &event)
{
    static_cast<void>(event);
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

    set_label_filename(import_dialog.file_name_label,
                       open_dialog.GetPath(),
                       PlayListEntry::CFGMIDI_DIALOG_MAX_LEN);
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
    update_window_title(true);
}


void PlayerWindow::on_quit(wxCommandEvent &event)
{
    if (pre_close_check(event)) {
        Close(true);
    }
}


void PlayerWindow::on_about(wxCommandEvent &event)
{
    static_cast<void>(event);
    wxMessageBox(fmt::format(
        L"BachBot MIDI player for Schlicker Organs \"{}\" edition:\n\n"
         "BachBot is a MIDI player intended Schlicker Pipe Organs or other "
         "Organs using the Syndyne Console Control system.\n"
         "Written By Andrew Buettner for Zion Lutheran Church and School "
         "Hartland, WI\n"
         "https://www.github.com/KB3HNS/BachBot"
         "\n\nImage by rawpixel.com on Freepik.com", EDITION),
        wxT("About BachBot"), wxOK | wxICON_INFORMATION);
}


void PlayerWindow::on_thread_tick(wxThreadEvent &event)
{
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

    if (0U != m_next_song_id.first) {
        m_song_labels[m_next_song_id.first]->reset_status();
    }
}


void PlayerWindow::on_device_changed(const uint32_t device_id)
{
    m_current_device_id = device_id;
}


void PlayerWindow::on_bank_changed(wxThreadEvent &event)
{
    m_current_config = BankConfig(event.GetInt());
    update_config_ui(false);
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
        m_next_song_id.second = false;
        set_next_song(song_data->get_sequence().second);
        m_current_song_event_count = song_data->get_song_events().size();
        event_count->SetRange(int(m_current_song_event_count));
        m_playing_label.set_label_text(song_data->get_filename());
        song_data->set_playing();
        scroll_to_widget(song_data);
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
    if ((nullptr != m_selected_control) && !select_multi_menu->IsChecked()) {
        const auto sequence = m_selected_control->get_sequence();
        if (0U != sequence.second) {
            on_move_event(m_selected_control->get_song_id(),
                          m_selected_control, false);
        }
    }
}


void PlayerWindow::on_accel_up_event(wxCommandEvent &event)
{
    static_cast<void>(event);
    if (nullptr != m_selected_control && !select_multi_menu->IsChecked()) {
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
    if (nullptr != m_selected_control && !select_multi_menu->IsChecked()) {
        set_next_song(m_selected_control->get_song_id(), true);
        scroll_to_widget(m_selected_control);
    }
}


void PlayerWindow::on_timer_tick(wxTimerEvent &event)
{
    static_cast<void>(event);
    m_up_next_label.animate_tick();
    m_playing_label.animate_tick();
    BankConfig next_config;
    auto box = next_song_box_sizer->GetStaticBox();
    if (m_player_thread.get() != nullptr) {
        next_config = m_player_thread->get_desired_config();
        box->SetLabelText("Desired Config");
    } else if (m_next_song_id.first > 0U) {
        const auto &song = m_song_labels.at(m_next_song_id.first);
        next_config = song->get_starting_registration();
        box->SetLabelText("Next Song Config");
    } else {
        box->SetLabelText("Current / Next Song");
    }

    next_memory_label->SetLabelText(wxString::Format(wxT("%d"),
                                                     next_config.memory));
    next_mode_label->SetLabelText(wxString::Format(wxT("%d"),
                                                   next_config.mode));

    const auto color = (next_config != m_current_config ?
                        *wxRED : GetBackgroundColour());
    if (next_song_panel->GetBackgroundColour() != color) {
        next_song_panel->SetBackgroundColour(color);
        next_song_panel->Refresh();
    }

    for (auto &[song_id, control]: m_song_labels) {
        control->update_color_state(song_id == m_next_song_id.first);
    }

    if (m_sync_config) {
        m_current_config = next_config;
        update_config_ui();
        m_sync_config = false;
    }
}


void PlayerWindow::on_move_event(const uint32_t song_id,
                                 PlaylistEntryControl *control,
                                 const bool direction)
{
    const auto sequence = control->get_sequence();
    const auto next_song_id = (direction ? sequence.first : sequence.second);
    auto other_control = m_song_labels[next_song_id].get();
    // scroll_to_widget(other_control);
    control->swap(other_control);
    std::swap(m_song_labels[song_id], m_song_labels[next_song_id]);
    if (direction) {
        std::swap(control, other_control);
    }

    //  order is now prev -> control -> other_control -> next
    const auto prev_sequence = control->get_sequence().first;
    const auto next_sequence = other_control->get_sequence().second;
    if (0U != prev_sequence) {
        m_song_labels[prev_sequence]->set_sequence(-1,
                                                   int(control->get_song_id()));
    } else {
        m_song_list.first = control->get_song_id();
    }

    if (0U != next_sequence) {
        m_song_labels[next_sequence]->set_sequence(
            int(other_control->get_song_id()));
    } else {
        m_song_list.second = other_control->get_song_id();
    }

    if (0U != m_current_song_id) {
        const auto cur_sequence = m_song_labels[m_current_song_id]->get_sequence();
        if ((cur_sequence.second != m_next_song_id.first) && !m_next_song_id.second) {
            if (0U != m_next_song_id.first) {
                m_song_labels[m_next_song_id.first]->reset_status();
            }
            set_next_song(cur_sequence.second);
        }
    }
}


void PlayerWindow::on_checkbox_event(const uint32_t song_id, const bool checked)
{
    if (song_id == m_current_song_id && m_player_thread.get() != nullptr) {
        if (checked && (0U != m_next_song_id.first)) {
            auto control = m_song_labels[m_next_song_id.first].get();
            control->set_next();
            m_player_thread->enqueue_next_song(control->get_song_events());
        } else {
            m_player_thread->enqueue_next_song({});
            if (0U != m_next_song_id.first &&
                m_current_song_id != m_next_song_id.first)
            {
                m_song_labels[m_next_song_id.first]->reset_status();
            }
        }
    }

    update_window_title(true);
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
    loader.set_on_success_callback([=](std::list<PlayListEntry> playlist) {
        if (playlist.size() > 0U) {
            std::for_each(playlist.begin(), playlist.end(),
                          [=](const PlayListEntry &i) {
                              add_playlist_entry(i);
                          });
            layout_scroll_panel();
            update_window_title(true);
        }
    });

    if (loader.ShowModal() != wxID_OK) {
        wxMessageBox(fmt::format(L"Error with import:\n"
                                 "Error reported was: {}",
                                 loader.get_error_text().value()));
        return;
    }
}


void PlayerWindow::on_mode_up_button_clicked(wxCommandEvent &event)
{
    static_cast<void>(event);
    if (m_current_config.mode < 8U) {
        ++m_current_config.mode;
    }
    update_config_ui();
}


void PlayerWindow::on_mode_down_button_clicked(wxCommandEvent &event)
{
    static_cast<void>(event);
    if (m_current_config.mode > 1U) {
        --m_current_config.mode;
    }
    update_config_ui();
}


void PlayerWindow::next_buttonOnButtonClick(wxCommandEvent &event)
{
    on_manual_advance(event);
}


void PlayerWindow::prev_buttonOnButtonClick(wxCommandEvent &event)
{
    on_manual_prev(event);
}


void PlayerWindow::cancel_buttonOnButtonClick(wxCommandEvent &event)
{
    on_manual_cancel(event);
}


void PlayerWindow::on_sync_button_clicked(wxCommandEvent &event)
{
    static_cast<void>(event);
    m_sync_config = true;
}


void PlayerWindow::on_select_multi(wxCommandEvent &event)
{
    static_cast<void>(event);
    if (!select_multi_menu->IsChecked()) {
        const auto sid = (nullptr != m_selected_control ?
                          m_selected_control->get_song_id() : 0U);
        for (auto &[song_id, control] : m_song_labels) {
            if (sid != song_id) {
                control->select(false);
            }
        }
    }
}


void PlayerWindow::on_clear_selection(wxCommandEvent &event)
{
    static_cast<void>(event);
    if (select_multi_menu->IsChecked()) {
        for (auto &[_, control] : m_song_labels) {
            control->select(false);
        }
    } else if (nullptr != m_selected_control) {
        m_selected_control->select(false);
    }
    m_selected_control = nullptr;
}


void PlayerWindow::on_shift_up(wxCommandEvent &event)
{
    static_cast<void>(event);
    if (select_multi_menu->IsChecked()) {
        auto song_id = m_song_list.first;
        if (m_song_labels[song_id]->is_selected()) {
            //  Can't continue first selected is at top of playlist
            return;
        }

        auto change_made = false;
        do {
            auto control = m_song_labels[song_id].get();
            const auto sequence = control->get_sequence();
            if (control->is_selected()) {
                on_move_event(song_id, control, true);  //  Move up
                change_made = true;
            }
            song_id = sequence.second;
        } while (song_id > 0U);

        if (change_made) {
            update_window_title(true);
        }
    }
}


void PlayerWindow::on_shift_down(wxCommandEvent &event)
{
    static_cast<void>(event);
    if (select_multi_menu->IsChecked()) {
        auto song_id = m_song_list.second;
        if (m_song_labels[song_id]->is_selected()) {
            //  Can't continue last selected is at bottom of playlist
            return;
        }

        auto change_made = false;
        do {
            auto control = m_song_labels[song_id].get();
            const auto sequence = control->get_sequence();
            if (control->is_selected()) {
                on_move_event(song_id, control, false);  //  Move down
                change_made = true;
            }
            song_id = sequence.first;
        } while (song_id > 0U);

        if (change_made) {
            update_window_title(true);
        }
    }
}


void PlayerWindow::on_group_edit(wxCommandEvent &event)
{
    if (select_multi_menu->IsChecked()) {
        static_cast<void>(event);
        GroupEditMidiDialog dialog(this);
        const auto result = dialog.ShowModal();
        if (wxID_CANCEL == result) {
            return;
        }

        auto song_id = m_song_list.first;
        while (song_id > 0U) {
            auto control = m_song_labels[song_id].get();
            const auto sequence = control->get_sequence();
            if (control->is_selected()) {
                if (!control->apply_group_dialog(dialog)) {
                    break;
                }
            }
            song_id = sequence.second;
        }

        update_window_title(true);
    }
}


void PlayerWindow::on_delete_selected(wxCommandEvent &event)
{
    static_cast<void>(event);
    auto song_id = m_song_list.first;
    m_selected_control = nullptr;
    while (song_id > 0U) {
        auto control = m_song_labels[song_id].get();
        const auto sequence = control->get_sequence();
        if (control->is_selected()) {
            control->select(false);
            remove_song(control, song_id);
        }

        song_id = sequence.second;
    }
}


void PlayerWindow::on_memory_up_button_clicked(wxCommandEvent &event)
{
    static_cast<void>(event);
    if (m_current_config.memory < 100U) {
        ++m_current_config.memory;
    }
    update_config_ui();
}


void PlayerWindow::on_memory_down_button_clicked(wxCommandEvent &event)
{
    static_cast<void>(event);
    if (m_current_config.memory > 1U) {
        --m_current_config.memory;
    }
    update_config_ui();
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
    m_next_song_id = std::make_pair(0U, false);
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

    static_cast<void>(playlist_container->Add(p_label.get(), 1,
                                              wxALL|wxEXPAND, 5));
    header_container->Show(true);
    p_label->set_sequence(int(m_song_list.second));
    m_song_list.second = song.song_id;

    p_label->set_callback([=](const PlaylistEntryEventId reason,
                              uint32_t song_id,
                              PlaylistEntryControl *control,
                              bool flag) {
        switch (reason) {
        case PlaylistEntryEventId::ENTRY_CHECKBOX_EVENT:
            on_checkbox_event(song_id, flag);
            break;

        case PlaylistEntryEventId::ENTRY_MOVED_EVENT:
            on_move_event(song_id, control, flag);
            update_window_title(true);
            break;

        case PlaylistEntryEventId::ENTRY_SET_NEXT_EVENT:
            set_next_song(song_id, true);
            scroll_to_widget(control);
            break;

        case PlaylistEntryEventId::ENTRY_SELECTED_EVENT:
            if (flag) {
                on_control_selected(song_id, control);
            }
            break;

        case PlaylistEntryEventId::ENTRY_DELETED_EVENT:
            remove_song(control, song_id);
            break;
        }
    });

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


void PlayerWindow::set_next_song(uint32_t song_id, bool priority)
{
    if (0U == song_id) {
        song_id = m_song_list.first;
    }
    if (!m_next_song_id.second || priority) {
        if (0U != m_next_song_id.first) {
            auto control = m_song_labels[m_next_song_id.first].get();
            control->reset_status();
            if (m_next_song_id.first == m_current_song_id) {
                control->set_playing();
            }
        }
        m_next_song_id = std::make_pair(song_id, priority);
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
            } else if (song_id != m_current_song_id) {
                next_song->reset_status();
            }
        } else {
            m_up_next_label.set_label_text(wxT(""));
        }
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
    m_player_thread->set_bank_config(m_current_config.memory,
                                     m_current_config.mode);

    if (0U != m_next_song_id.first) {
        auto control = m_song_labels[m_next_song_id.first].get();
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


void PlayerWindow::scroll_to_widget(const PlaylistEntryControl *const widget)
{
    int x = -1;
    int y = -1;
    playlist_panel->GetScrollPixelsPerUnit(&x, &y);
    auto position = widget->GetPosition();
    const auto size = widget->GetSize();
    const auto start = playlist_panel->GetViewStart();
    position.y += start.y * y;
    position.y -= size.y;
    if (position.y < 0) {
        position.y = 0;
    } else {
        position.y /= y;
    }
    position.x = -1;
    playlist_panel->Scroll(position);
}


void PlayerWindow::update_config_ui(const bool send_update)
{
    if (m_current_config.mode >= 8U) {
        m_current_config.mode = 8U;
        static_cast<void>(mode_up_button->Enable(false));
    } else {
        static_cast<void>(mode_up_button->Enable());
    }
    static_cast<void>(mode_down_button->Enable((m_current_config.mode > 1U)));

    if (m_current_config.memory >= 100U) {
        m_current_config.memory = 100U;
        static_cast<void>(memory_up_button->Enable(false));
    } else {
        static_cast<void>(memory_up_button->Enable());
    }
    static_cast<void>(memory_down_button->Enable((m_current_config.memory > 1U)));

    memory_label->SetLabelText(wxString::Format(wxT("%d"),
                                                m_current_config.memory));
    mode_label->SetLabelText(wxString::Format(wxT("%u"),
                                              m_current_config.mode));
    if ((m_player_thread.get() != nullptr) && send_update) {
        m_player_thread->set_bank_config(m_current_config.memory,
                                         m_current_config.mode);
    }
}


void PlayerWindow::update_window_title(const bool playlist_changed)
{
    m_playlist_changed = playlist_changed;
    if (!s_window_title.has_value()) {
        s_window_title = GetTitle();
    }

    const auto &title = s_window_title.value();
    if (m_playlist_name.has_value()) {
        const auto change_flag = (m_playlist_changed ? L"*"sv : L""sv);
        SetTitle(fmt::format(L"{} - {}{}",
                             title, m_playlist_name.value(),
                             change_flag));
    } else {
        SetTitle(title);
    }
}


void PlayerWindow::remove_song(PlaylistEntryControl *const widget,
                               const uint32_t song_id)
{
    const auto sequence = widget->get_sequence();

    PlaylistEntryControl *prev_song = nullptr;

    assert(song_id != m_current_song_id);
    if (sequence.first > 0U) {
        prev_song = m_song_labels[sequence.first].get();
        prev_song->set_sequence(-1, int(sequence.second));
    } else {
        //  Removing first entry in playlist
        assert(song_id == m_song_list.first);
        m_song_list.first = sequence.second;
    }

    if (sequence.second > 0U) {
        auto next_song = m_song_labels[sequence.second].get();
        next_song->set_sequence(int(sequence.first));
    } else {
        //  Removing last entry in playlist
        assert(song_id == m_song_list.second);
        m_song_list.second = sequence.first;
    }

    if (song_id == m_next_song_id.first) {
        set_next_song(sequence.second, true);
    }

    if (widget == m_selected_control) {
        widget->select(false);
        m_selected_control = nullptr;
    }

    playlist_container->Detach(widget);
    layout_scroll_panel();
    update_window_title(true);
    m_song_labels.erase(song_id);
}


void PlayerWindow::on_control_selected(const uint32_t song_id,
                                       PlaylistEntryControl *const widget)
{
    if (!select_multi_menu->IsChecked()) {
        for (auto &[sid, entry]: m_song_labels) {
            if (sid != song_id) {
                entry->select(false);
            }
        }
    } else if ((nullptr != m_selected_control) &&
               (widget != m_selected_control) &&
               wxGetKeyState(WXK_SHIFT)) {
        auto select = false;
        const auto range = std::make_pair(
            m_selected_control->get_song_id(),
            widget->get_song_id());

        auto next_song_id = m_song_list.first;
        do {
            auto control = m_song_labels[next_song_id].get();
            const auto control_song_id = control->get_song_id();
            if (control_song_id == range.first || control_song_id == range.second) {
                if (select) {
                    //  Reached the end, done
                    break;
                }
                select = true;
            }

            if (select) {
                control->select();
            }
            const auto sequence = control->get_sequence();
            next_song_id = sequence.second;
        } while (next_song_id > 0U);
    }
    m_selected_control = widget;
}


PlayerWindow::~PlayerWindow()
{
    static_cast<void>(PopEventHandler());

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
    EVT_MENU(PlayerWindowEvents::PLAY_ACTIVATE_EVENT, PlayerWindow::on_play_advance)
    EVT_TIMER(PlayerWindowEvents::UI_ANIMATE_TICK, PlayerWindow::on_timer_tick)
wxEND_EVENT_TABLE()

}  //  end ui
}  //  end bach_bot
