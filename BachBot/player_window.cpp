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
#include <fmt/xchar.h>  //  fmt::format(L

//  module includes
// -none-

//  local includes
#include "player_window.h"  //  local include
#include "player_thread.h"  //  PlayerThread
#include "organ_midi_event.h"  //  OrganMidiEvent
#include "syndyne_importer.h"  //  SyndineImporter

namespace {
    std::list<bach_bot::OrganMidiEvent> file_events;

}  //  end anonymous namespace


namespace bach_bot {
namespace ui {

PlayerWindow::PlayerWindow() :
    MainWindow(nullptr),
    wxLog(),
    m_counter{0U},
    m_player_thread{nullptr},
    m_midi_devices(),
    m_midi_out(),
    m_current_device_id{0U}
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
        event_count->SetValue(0);
        event_count->SetRange(int(file_events.size()));
        m_player_thread = new PlayerThread(this, m_current_device_id);
        m_player_thread->play(file_events);
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
    wxMessageBox("Load Playlist",
                 "load playlist", wxOK | wxICON_INFORMATION);
}


void PlayerWindow::on_save_playlist(wxCommandEvent &event)
{
    wxMessageBox("Save Playlist",
                 "Save current playlist", wxOK | wxICON_INFORMATION);
}


void PlayerWindow::on_open_midi(wxCommandEvent &event)
{
    wxFileDialog open_dialog(this, "Open MIDI File", "", "", 
                             "MIDI Files|*.mid", 
                             wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (open_dialog.ShowModal() == wxID_CANCEL) {
        return;
    }

    auto importer = std::make_unique<SyndineImporter>(
    open_dialog.GetPath().ToStdString(), 0U);

    auto tempo = importer->get_tempo();
    if (!tempo.has_value()) {
        tempo = 120;
    }

    LoadMidiDialog import_dialog(this);
    import_dialog.file_name_label->SetLabel(open_dialog.GetPath());
    import_dialog.tempo_label->SetLabel(fmt::format(L"{}bpm", tempo.value()));
    import_dialog.select_tempo->SetValue(tempo.value());
    if (import_dialog.ShowModal() == wxID_CANCEL) {
        return;
    }

    file_events = importer->get_events(0.0, 0.0);
}


void PlayerWindow::on_quit(wxCommandEvent &event)
{
    Close(true);
}


void PlayerWindow::on_about(wxCommandEvent &event)
{
    wxMessageBox("This is a wxWidgets' Hello world sample",
                 "About Hello World", wxOK | wxICON_INFORMATION);
}


void PlayerWindow::on_thread_tick(wxThreadEvent &event)
{
    ++m_counter;
    const auto current_setting = uint32_t(event.GetInt());
    const auto bank = current_setting % 8U;
    const auto mode = current_setting / 8U;
    bank_label->SetLabel(wxString(fmt::format(L"{}", bank + 1U)));
    mode_label->SetLabel(wxString(fmt::format(L"{}", mode + 1U)));
    size_t events_complete = 0U;
    if (file_events.size() > 0U) {
        events_complete = file_events.size() - 
                          m_player_thread->get_events_remaining();
    }
    event_count->SetValue(int(events_complete));
}


void PlayerWindow::on_thread_exit(wxThreadEvent &event)
{
    if (nullptr != m_player_thread) {
        delete m_player_thread;
        m_player_thread = nullptr;
    }

    event_count->SetValue(0);
    std::for_each(m_midi_devices.begin(), m_midi_devices.end(),
                  [](wxMenuItem& i) { i.Enable(); });
}


void PlayerWindow::on_device_changed(const uint32_t device_id)
{
    m_current_device_id = device_id;
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
    std::array<uint8_t, MIDI_MESSAGE_SIZE> midi_message;
    midi_message[0] = make_midi_command_byte(0U, MidiCommands::CONTROL_CHANGE);
    midi_message[1] = SYNDYNE_CONTROLLER_ID;
    midi_message[2] = value;

    const auto port_open = m_midi_out.isPortOpen();
    if (!port_open) {
        m_midi_out.openPort(m_current_device_id);
    }

    m_midi_out.sendMessage(midi_message.data(), MIDI_MESSAGE_SIZE);

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
    EVT_THREAD(PlayerEvents::EXIT_EVENT, PlayerWindow::on_thread_exit)
wxEND_EVENT_TABLE()

}  //  end ui
}  //  end bach_bot
