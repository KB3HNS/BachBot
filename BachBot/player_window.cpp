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
#include <exception>  //  std::runtime_exception
#include <limits>  //  std::numeric_limits
#include <fmt/xchar.h>  //  fmt::format(L
#include <fmt/format.h>  //  fmt::format

//  module includes
// -none-

//  local includes
#include "player_window.h"  //  local include
#include "player_thread.h"  //  PlayerThread
#include "midi_note_tracker.h"  //  MidiNoteTracker
#include "organ_midi_event.h"  //  OrganMidiEvent


namespace {
    std::list<OrganMidiEvent> file_events;

    /** Map of keyboard index to keyboard channel */
    const std::array<SyndineKeyboards,
                     NUM_SYNDINE_KEYBOARDS> keyboard_indexes = {
        MANUAL1_SWELL, MANUAL2_GREAT, PETAL
    };

    /** Map MIDI channel to keyboard / special event */
    const std::array<uint8_t, 16U> channel_mapping = {
        MANUAL1_SWELL, MANUAL1_SWELL, MANUAL1_SWELL,
        MANUAL2_GREAT, MANUAL2_GREAT, MANUAL2_GREAT,
        PETAL, PETAL,
        0xFFU,  //  Drums - used for control
        MANUAL1_SWELL, MANUAL2_GREAT, PETAL,
        MANUAL1_SWELL, MANUAL2_GREAT, PETAL,
        0xFFU
    };

    /** 
     * @brief Get the the appropriate index of the channel event map based on
     *        the reported MIDI channel of a MIDI event.
     * @param channel midi event channel
     * @returns array index
     * @retval size_t::max Channel is a control channel
     */
    size_t get_control_index(const int channel)
    {
        if (channel < 0 || channel > int(channel_mapping.size())) {
            throw std::runtime_error(fmt::format("Index out of range: {}",
                                     channel));
        }

        const auto keyboard = channel_mapping[size_t(channel)];
        auto index = std::numeric_limits<size_t>::max();
        for (auto i = 0U; i < keyboard_indexes.size(); ++i) {
            if (keyboard == keyboard_indexes[i]) {
                index = i;
                break;
            }
        }

        return index;
    }

    /**
     * @brief Remap a MIDI note to a note that exists on the appropriate
     *        keyboard.
     * @param note MIDI note ID
     * @param keyboard note is being assigned to
     * @retval MIDI note that can be corectly represented on the keyboard
     */
    uint8_t remap_note(const int note, const SyndineKeyboards keyboard)
    {
        static_cast<void>(keyboard);
        return uint8_t(note);
    }
}  //  end anonymous namespace


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

    smf::MidiFile midifile;
    midifile.read(open_dialog.GetPath().ToStdString());
    midifile.doTimeAnalysis();
    midifile.linkNotePairs();
    midifile.joinTracks();
    build_syndine_sequence(midifile[0]);
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
    const auto events_complete = file_events.size() - 
                                 m_player_thread->get_events_remaining();
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
    midi_message[1] = SYNDINE_CONTROLLER_ID;
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


void PlayerWindow::build_syndine_sequence(const smf::MidiEventList& event_list) const
{
    std::list<OrganMidiEvent> events;
    SyndineMidiEventTable<MidiNoteTracker> current_state;
    for (auto i = 0U; i < current_state.size(); ++i) {
        const auto keyboard_id = keyboard_indexes[i];
        for (auto j = 0U; j < current_state[i].size(); ++j) {
            current_state[i][j].set_keyboard(keyboard_id);
        }
    }

    //  1st pass: Process all events
    uint8_t current_bank = 0U;
    uint32_t current_mode = 0U;
    for (auto i = 0; i < event_list.size(); ++i) {
        auto midi_event = event_list[i];
        if (midi_event.isNote()) {
            const auto channel_id = get_control_index(midi_event.getChannel());
            if (channel_id < current_state.size()) {
                const auto note = remap_note(midi_event.getKeyNumber(), 
                                             keyboard_indexes[channel_id]);
                midi_event[2] = note;
                current_state[channel_id][note].add_event(midi_event);
            } else {
                //  Treat as control event
                events.emplace_back(midi_event, 
                                    BankConfig{current_bank, current_mode});
            }
        }
    }

    //  2nd pass: append all de-duplicated events
    for (const auto &i: current_state) {
        for (const auto &j: i) {
            j.append_events(events);
        }
    }

    //  3rd pass: sort by time
    events.sort();

    //  4th pass: update bank config, build output events
    file_events.clear();
    auto current_config = events.front().get_bank_config();
    for (auto &i: events) {
        if (i.is_mode_change_event()) {
            current_config = i.get_bank_config();
        } else {
            i.set_bank_config(current_config);
            file_events.push_back(i);
        }
    }

    //  5th pass: remove start dead time
    auto &initial_delay = file_events.front();
    auto last_element = initial_delay;
    for (auto &i : file_events) {
        i -= initial_delay;
        i.calculate_delta(last_element);
        last_element = i;
    }
}

/*
void build_syndine_sequence(const smf::MidiEventList& event_list)
{
    std::list<OrganMidiEvent> events;
    auto current_tick = 0;
    auto current_time = 0.0;
    for (auto i = 0; i < event_list.size(); ++i) {
        OrganMidiEvent ev;
        const auto& midi_event = event_list[i];
        ev.event_code = uint8_t(midi_event[0]);
        if (midi_event.size() > 1) {
            ev.byte1 = uint8_t(midi_event[1]);
            if (midi_event.size() > 2) {
                ev.byte2 = uint8_t(midi_event[2]);
            }
        }

        ev.delay = midi_event.tick;
        ev.delta = ev.delay - current_tick;
        current_tick = ev.delay;

        ev.seconds = midi_event.seconds;
        ev.delta_time = ev.seconds - current_time;
        current_time = ev.seconds;
        events.push_back(ev);
    }

    events.sort();
    file_events = std::move(events);
}
*/

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