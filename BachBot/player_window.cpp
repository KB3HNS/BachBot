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
#include <array>  //  std::array
#include <stdexcept>  //  std::runtime_error
#include <string>  //  std::string
#include <string_view>  //  sv
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
    const auto EDITION = L"Trinity"sv;
}  //  end anonymous namespace


namespace bach_bot {
namespace ui {

PlayerWindow::PlayerWindow() :
    MainWindow(nullptr),
    wxLog(),
    m_counter{0U},
    m_player_thread(),
    m_midi_devices(),
    m_midi_out(),
    m_current_device_id{0U},
    m_playlist(),
    m_current_song_event_count{0U},
    m_current_song_id{0U},
    m_first_song_id{0U},
    m_last_song_id{0U},
    m_song_labels()
{
    for (auto i = 0U; i < m_midi_out.getPortCount(); ++i) {
        m_midi_devices.emplace_back(
            device_select, wxID_ANY, wxString(m_midi_out.getPortName(i)),
            wxEmptyString, wxITEM_RADIO
        );
        device_select->Append(&m_midi_devices.back());
        device_select->Bind(wxEVT_COMMAND_MENU_SELECTED, 
                            [=](wxCommandEvent&) { on_device_changed(i); },
                            m_midi_devices.back().GetId()
        );
    }
    m_midi_devices.front().Check();
}


void PlayerWindow::on_play_advance(wxCommandEvent &event)
{
    if (nullptr == m_player_thread) {
        m_player_thread = std::make_unique<PlayerThread>(this, m_current_device_id);
        m_player_thread->play(m_first_song_id);
        std::for_each(m_midi_devices.begin(), m_midi_devices.end(), 
                      [](wxMenuItem &i) { i.Enable(false); });
    } else {
        m_player_thread->signal_advance();
    }
}


void PlayerWindow::on_stop(wxCommandEvent &event)
{
    if (nullptr != m_player_thread) {
        m_player_thread->signal_stop();
        m_player_thread->Wait();
    }
}


void PlayerWindow::on_new_playlist(wxCommandEvent &event)
{
    wxMessageBox("New Playlist",
                 "create new playlist", wxOK | wxICON_INFORMATION);
}


void PlayerWindow::on_load_playlist(wxCommandEvent &event)
{
    wxFileDialog open_dialog(this, "Open Playlist", "", "", 
                             "BachBot Playlist|*.bbp", 
                             wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (open_dialog.ShowModal() == wxID_CANCEL) {
        return;
    }

    wxXmlDocument playlist;
    playlist.Load(open_dialog.GetPath());

    PlaylistLoader loader(this, m_playlist, playlist.GetRoot());

    static_cast<void>(loader.ShowModal());
}


void PlayerWindow::on_save_playlist(wxCommandEvent &event)
{
    wxFileDialog save_dialog(this, "Save Playlist", "", "", 
                             "BachBot Playlist|*.bbp", 
                             wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (save_dialog.ShowModal() == wxID_CANCEL) {
        return;
    }

    wxXmlDocument playlist_doc;
    auto playlist_root = new wxXmlNode(nullptr, 
                                       wxXML_ELEMENT_NODE,
                                       wxT("BachBot_Playlist"));

    playlist_doc.SetRoot(playlist_root);
    auto song_id = m_first_song_id;
    while (song_id > 0U) {
        auto playlist_entry = new wxXmlNode(playlist_root, 
                                            wxXML_ELEMENT_NODE, 
                                            wxT("song"));

        auto lock = m_playlist.lock();
        const auto &song = m_playlist.get_playlist_entry(song_id);
        if (song.tempo_detected.has_value()) {
            playlist_entry->AddAttribute(
                wxT("tempo_detected"),
                wxString::Format(wxT("%i"), song.tempo_detected.value()));
            playlist_entry->AddAttribute(
                wxT("tempo_requested"),
                wxString::Format(wxT("%i"), song.tempo_requested));
        }
        playlist_entry->AddAttribute(
            wxT("gap"), wxString::FromDouble(song.gap_beats));
        playlist_entry->AddAttribute(
            wxT("pitch"),
            wxString::Format(wxT("%i"), song.delta_pitch));
        playlist_entry->AddAttribute(
            wxT("last_note_multiplier"), 
            wxString::FromDouble(song.last_note_multiplier));
        playlist_entry->AddAttribute(
            wxT("auto_play_next"),
            wxString::Format(wxT("%i"), int(song.play_next)));
        playlist_entry->AddChild(new wxXmlNode(wxXML_TEXT_NODE, 
                                               wxT(""), 
                                               song.file_name));

        song_id = song.next_song_id;
    }

    playlist_doc.Save(save_dialog.GetPath());
}


void PlayerWindow::on_open_midi(wxCommandEvent &event)
{
    wxFileDialog open_dialog(this, "Open MIDI File", "", "", 
                             "MIDI Files|*.mid", 
                             wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (open_dialog.ShowModal() == wxID_CANCEL) {
        return;
    }

    auto lock = m_playlist.lock();
    auto &song_entry = m_playlist.get_playlist_entry();

    auto importer = std::make_unique<SyndineImporter>(
        open_dialog.GetPath().ToStdString(), song_entry.current_song_id);

    song_entry.file_name = open_dialog.GetPath();
    song_entry.tempo_detected = importer->get_tempo();
    auto tempo = DEFAULT_NO_TEMPO;
    if (song_entry.tempo_detected.has_value()) {
        tempo = song_entry.tempo_detected.value();
    }

    LoadMidiDialog import_dialog(this);
    import_dialog.file_name_label->SetLabel(open_dialog.GetPath());
    import_dialog.tempo_label->SetLabel(fmt::format(L"{}bpm", tempo));
    import_dialog.select_tempo->SetValue(tempo);

    auto test = [](double &dest, wxTextCtrl *const box) -> bool {
        auto test_value = std::numeric_limits<double>::max();
        const auto ok = box->GetValue().ToDouble(&test_value);
        if (ok) {
            dest = test_value;
        }

        return ok;
    };

    wxString error_text;
    do {
        if (error_text.size() > 0U) {
            wxMessageBox(fmt::format(L"Error in field: {}", error_text),
                         "Form Error", wxOK | wxICON_INFORMATION);
        }

        if (import_dialog.ShowModal() == wxID_CANCEL) {
            return;
        }

        error_text.clear();
        if (!test(song_entry.last_note_multiplier,
                  import_dialog.extend_ending_textbox)) {
            error_text = import_dialog.extended_ending_label->GetLabelText();
        }

        if (!test(song_entry.gap_beats, import_dialog.initial_gap_text_box)) {
            error_text = import_dialog.initial_gap_label->GetLabelText();
        }
    } while (error_text.size() > 0U);


    song_entry.tempo_requested = import_dialog.select_tempo->GetValue();
    song_entry.starting_config = std::make_pair(
        uint8_t(import_dialog.bank_select->GetValue() - 1), 
        uint32_t(import_dialog.mode_select->GetValue() - 1));
    song_entry.delta_pitch = import_dialog.pitch_change->GetValue();
    song_entry.play_next = import_dialog.play_next_checkbox->IsChecked();

    song_entry.midi_events = importer->get_events(
        song_entry.gap_beats, song_entry.last_note_multiplier);

    if (0U == m_first_song_id) {
        m_first_song_id = song_entry.current_song_id;
        playlist_label->SetLabel(song_entry.file_name);
    } else {
        auto &prev_song = m_playlist.get_playlist_entry(m_last_song_id);
        prev_song.next_song_id = song_entry.current_song_id;
        m_song_labels.emplace_back(playlist_panel, 
                                   wxID_ANY, 
                                   song_entry.file_name);
        auto *const p_label = &m_song_labels.back();
        playlist_container->Add(p_label, 0, wxALL, 5);
    }

    playlist_panel->Layout();
    m_last_song_id = song_entry.current_song_id;
}


void PlayerWindow::on_quit(wxCommandEvent &event)
{
    Close(true);
}


void PlayerWindow::on_about(wxCommandEvent &event)
{
    wxMessageBox(fmt::format(
        L"BachBot MIDI player for Schlicker Organs {} edition\n"
         " or other Organs using the Syndyne Console Control system\n"
         "Written By Andrew Buettner for Zion Lutheran Church and School\n"
         "https://www.github.com/KB3HNS/BachBot", EDITION),
        L"About BachBot", wxOK | wxICON_INFORMATION);
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
    m_player_thread.reset();
    m_current_song_event_count = 0U;
    m_current_song_id = 0U;

    event_count->SetValue(0);
    track_label->SetLabel(L"Not Playing");
    std::for_each(m_midi_devices.begin(), m_midi_devices.end(),
                  [](wxMenuItem& i) { i.Enable(); });
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
    bank_label->SetLabel(wxString(fmt::format(L"{}", bank + 1U)));
    mode_label->SetLabel(wxString(fmt::format(L"{}", mode + 1U)));
}


void PlayerWindow::on_song_starts_playing(wxThreadEvent &event)
{
    event_count->SetValue(0);
    const auto song_id = uint32_t(event.GetInt());
    if (song_id > 0U) {
        auto lock = m_playlist.lock();
        const auto &song_data = m_playlist.get_playlist_entry(song_id);

        m_current_song_event_count = song_data.midi_events.size();
        event_count->SetRange(int(m_current_song_event_count));
        m_current_song_id = song_id;
        track_label->SetLabel(song_data.file_name);
    }
}


void PlayerWindow::on_manual_advance(wxCommandEvent &event)
{
    send_manual_message(SyndyneBankCommands::NEXT_BANK);
}


void PlayerWindow::on_manual_prev(wxCommandEvent &event)
{
    send_manual_message(SyndyneBankCommands::PREV_BANK);
}


void PlayerWindow::on_manual_cancel(wxCommandEvent &event)
{
    send_manual_message(SyndyneBankCommands::GENERAL_CANCEL);
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


PlayerWindow::~PlayerWindow()
{
    wxCommandEvent e;
    on_stop(e);
    std::for_each(m_midi_devices.begin(), m_midi_devices.end(), 
                  [=](wxMenuItem &i) { device_select->Remove(&i); });
}


wxBEGIN_EVENT_TABLE(PlayerWindow, wxFrame)
    EVT_THREAD(PlayerEvents::TICK_EVENT, PlayerWindow::on_thread_tick)
    EVT_THREAD(PlayerEvents::SONG_START_EVENT, 
               PlayerWindow::on_song_starts_playing)
    EVT_THREAD(PlayerEvents::BANK_CHANGE_EVENT, PlayerWindow::on_bank_changed)
    EVT_THREAD(PlayerEvents::EXIT_EVENT, PlayerWindow::on_thread_exit)
wxEND_EVENT_TABLE()

}  //  end ui
}  //  end bach_bot
