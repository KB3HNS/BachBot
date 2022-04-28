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
#include <algorithm>  //  std::foreach, std::clamp
#include <array>  //  std::array
#include <exception>  //  std::runtime_exception
#include <limits>  //  std::numeric_limits
#include <unordered_map>  //  std::unordered_map
#include <optional>  //  std::optional
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
    using namespace bach_bot;
    std::list<OrganMidiEvent> file_events;

    /** Map of keyboard index to keyboard channel */
    const std::array<SyndyneKeyboards,
                     NUM_SYNDYNE_KEYBOARDS> keyboard_indexes = {
        SyndyneKeyboards::MANUAL1_GREAT, 
        SyndyneKeyboards::MANUAL2_SWELL, 
        SyndyneKeyboards::PETAL
    };

    /** Map drums to commands */
    const std::unordered_map<uint8_t, SyndyneBankCommands> drum_map = {
        // Drumkit
        {36U, SyndyneBankCommands::PREV_BANK},
        {35U, SyndyneBankCommands::PREV_BANK},
        {38U, SyndyneBankCommands::PREV_BANK},
        {40U, SyndyneBankCommands::PREV_BANK},
        {37U, SyndyneBankCommands::PREV_BANK},

        // Hi-hat
        {42U, SyndyneBankCommands::GENERAL_CANCEL},
        {46U, SyndyneBankCommands::GENERAL_CANCEL},
        {44U, SyndyneBankCommands::GENERAL_CANCEL},

        // Cymbal
        {49U, SyndyneBankCommands::NEXT_BANK},
        {57U, SyndyneBankCommands::NEXT_BANK},
        {55U, SyndyneBankCommands::NEXT_BANK},
        {52U, SyndyneBankCommands::NEXT_BANK},
        {51U, SyndyneBankCommands::NEXT_BANK},
        {59U, SyndyneBankCommands::NEXT_BANK},
        {53U, SyndyneBankCommands::NEXT_BANK},

        // Toms
        {41U, SyndyneBankCommands::PREV_BANK},
        {43U, SyndyneBankCommands::PREV_BANK},
        {45U, SyndyneBankCommands::PREV_BANK},
        {47U, SyndyneBankCommands::PREV_BANK},
        {48U, SyndyneBankCommands::PREV_BANK},
        {50U, SyndyneBankCommands::PREV_BANK},

        // African
        {76U, SyndyneBankCommands::GENERAL_CANCEL},
        {77U, SyndyneBankCommands::GENERAL_CANCEL},
        {69U, SyndyneBankCommands::GENERAL_CANCEL},
        {67U, SyndyneBankCommands::GENERAL_CANCEL},
        {68U, SyndyneBankCommands::GENERAL_CANCEL},
        {58U, SyndyneBankCommands::GENERAL_CANCEL},
        {62U, SyndyneBankCommands::GENERAL_CANCEL},
        {63U, SyndyneBankCommands::GENERAL_CANCEL},
        {64U, SyndyneBankCommands::GENERAL_CANCEL},

        // Latin
        {73U, SyndyneBankCommands::NEXT_BANK},
        {74U, SyndyneBankCommands::NEXT_BANK},
        {75U, SyndyneBankCommands::NEXT_BANK},
        {78U, SyndyneBankCommands::NEXT_BANK},
        {79U, SyndyneBankCommands::NEXT_BANK},
        {70U, SyndyneBankCommands::NEXT_BANK},
        {56U, SyndyneBankCommands::NEXT_BANK},
        {60U, SyndyneBankCommands::NEXT_BANK},
        {61U, SyndyneBankCommands::NEXT_BANK},
        {85U, SyndyneBankCommands::NEXT_BANK},
        {86U, SyndyneBankCommands::NEXT_BANK},
        {87U, SyndyneBankCommands::NEXT_BANK},

        // Others
        {54U, SyndyneBankCommands::PREV_BANK},
        {65U, SyndyneBankCommands::PREV_BANK},
        {66U, SyndyneBankCommands::PREV_BANK},
        {71U, SyndyneBankCommands::PREV_BANK},
        {72U, SyndyneBankCommands::PREV_BANK},
        {80U, SyndyneBankCommands::PREV_BANK},
        {81U, SyndyneBankCommands::PREV_BANK},
        {82U, SyndyneBankCommands::PREV_BANK},
        {83U, SyndyneBankCommands::PREV_BANK},
        {84U, SyndyneBankCommands::PREV_BANK},
        {31U, SyndyneBankCommands::PREV_BANK},

        // Sound Effects
        {34U, SyndyneBankCommands::NEXT_BANK},
        {33U, SyndyneBankCommands::NEXT_BANK},
        {32U, SyndyneBankCommands::NEXT_BANK},
        {30U, SyndyneBankCommands::NEXT_BANK},
        {29U, SyndyneBankCommands::NEXT_BANK},
        {28U, SyndyneBankCommands::NEXT_BANK},
        {27U, SyndyneBankCommands::NEXT_BANK},
        {39U, SyndyneBankCommands::NEXT_BANK}
    };

    /** Map MIDI channel to keyboard / special event */
    const std::array<uint8_t, 16U> channel_mapping = {
        SyndyneKeyboards::MANUAL2_SWELL, SyndyneKeyboards::MANUAL2_SWELL, SyndyneKeyboards::MANUAL2_SWELL,
        SyndyneKeyboards::MANUAL1_GREAT, SyndyneKeyboards::MANUAL1_GREAT, SyndyneKeyboards::MANUAL1_GREAT,
        SyndyneKeyboards::PETAL, SyndyneKeyboards::PETAL, SyndyneKeyboards::PETAL,
        0xFFU,  //  (9) Drums - used for control
        SyndyneKeyboards::MANUAL2_SWELL, SyndyneKeyboards::MANUAL1_GREAT, SyndyneKeyboards::PETAL,
        SyndyneKeyboards::MANUAL2_SWELL, SyndyneKeyboards::MANUAL1_GREAT, SyndyneKeyboards::PETAL
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
    uint8_t remap_note(const int note, const SyndyneKeyboards keyboard)
    {
        uint8_t low_limit = 36U;  //  Standard range of organ keys.
        uint8_t high_limit = 96U;
        if (SyndyneKeyboards::PETAL == keyboard) {
            high_limit = 67U;  //  Petal only goes up to G above middle-C
        }

        auto mapped_note = uint8_t(note);
        while (mapped_note < low_limit) {
            mapped_note += 12U;  //  Bump 1 octave
        }
        while (mapped_note > high_limit) {
            mapped_note -= 12U;  //  Bump 1 octave down
        }
        return mapped_note;
    }

    /**
     * @brief Update the current state of the configuration based on a drum
     *        channel note.
     * @param note note ID
     * @param current_state[in/out] state to update
     */
    void get_bank_event(const int note, BankConfig &current_state)
    {
        const auto bank_event = drum_map.find(uint8_t(note));
        if (drum_map.end() != bank_event) {
            switch (bank_event->second) {
            case SyndyneBankCommands::GENERAL_CANCEL:
                current_state.first = 0U;
                break;

            case SyndyneBankCommands::PREV_BANK:
                if (0U == current_state.first) {
                    current_state.first = 8U;
                    --current_state.second;
                }
                --current_state.first;
                break;

            case SyndyneBankCommands::NEXT_BANK:
                ++current_state.first;
                if (current_state.first >= 8U) {
                    current_state.first = 0U;
                    ++current_state.second;
                }
                break;

            default:
                break;
            }
        }

        current_state.first = std::clamp(current_state.first, 
                                         uint8_t(0U), 
                                         uint8_t(7U));
        current_state.second = std::clamp(current_state.second,  0U,  99U);
    }
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

    smf::MidiFile midifile;
    midifile.read(open_dialog.GetPath().ToStdString());
    midifile.doTimeAnalysis();
    midifile.linkNotePairs();
    midifile.joinTracks();
    auto tempo = 120.0;
    for (auto i = 0; i < midifile[0].size(); ++i) {
        const auto &evt = midifile[0][i];
        if (evt.isTempo()) {
            tempo = evt.getTempoBPM();
            break;
        }
    }

    LoadMidiDialog import_dialog(this);
    import_dialog.file_name_label->SetLabel(open_dialog.GetPath());
    import_dialog.tempo_label->SetLabel(fmt::format(L"{}bpm", tempo));
    import_dialog.select_tempo->SetValue(int(tempo));
    if (import_dialog.ShowModal() == wxID_CANCEL) {
        return;
    }

    build_syndyne_sequence(midifile[0]);
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


void PlayerWindow::build_syndyne_sequence(const smf::MidiEventList& event_list) const
{
    std::list<OrganMidiEvent> events;
    SyndyneMidiEventTable<MidiNoteTracker> current_state;
    for (auto i = 0U; i < current_state.size(); ++i) {
        const auto keyboard_id = keyboard_indexes[i];
        for (auto j = 0U; j < current_state[i].size(); ++j) {
            current_state[i][j].set_keyboard(keyboard_id);
        }
    }

    //  1st pass: Process all events
    auto config = BankConfig{0U, 0U};
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
            } else if (midi_event.isNoteOn()) {
                //  Treat as control event
                get_bank_event(midi_event.getKeyNumber(), config);
                events.emplace_back(midi_event, config);
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
