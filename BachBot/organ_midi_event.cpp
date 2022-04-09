/**
 * @file organ_midi_event.cpp
 * @brief Individual MIDI event sent to organ.
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
// -none-

//  module includes
// -none-

//  local includes
#include "organ_midi_event.h"  //  Local include


OrganMidiEvent::OrganMidiEvent(const smf::MidiEvent& midi_event, const SyndineKeyboards channel) :
    m_event_code{make_midi_command_byte(channel, MidiCommands::SPECIAL)},
    m_mode_change_event{false},
    m_desired_bank_number{0U},
    m_desired_mode_number{0U},
    m_seconds{midi_event.seconds},
    m_delta_time{0.0},
    m_byte1(),
    m_byte2(),
    m_midi_time{midi_event.tick},
    m_delta{0}
{
    if (midi_event.isNoteOn()) {
        m_event_code = make_midi_command_byte(channel, MidiCommands::NOTE_ON);
    } else if (midi_event.isNoteOff()) {
        m_event_code = make_midi_command_byte(channel, MidiCommands::NOTE_OFF);
    }
    if (midi_event.size() > 1) {
        m_byte1 = uint8_t(midi_event[1]);
        if (midi_event.size() > 2) {
            m_byte2 = uint8_t(midi_event[2]);
        } else {
            m_byte2.reset();
        }
    } else {
        m_byte1.reset();
        m_byte2.reset();
    }
}


OrganMidiEvent::OrganMidiEvent(const smf::MidiEvent &midi_event,
                               const BankConfig& cfg) :
    m_event_code{make_midi_command_byte(uint8_t(midi_event.getChannel()),
                                        MidiCommands::SPECIAL)},
    m_seconds{midi_event.seconds},
    m_mode_change_event{true},
    m_desired_bank_number{cfg.first},
    m_desired_mode_number{cfg.second},
    m_delta_time{0.0},
    m_byte1(),
    m_byte2(),
    m_midi_time{midi_event.tick},
    m_delta{0}
{
}


wxLongLong OrganMidiEvent::get_us() const
{
    const auto ms_time = m_seconds * 1000000.0;
    return wxLongLong(ms_time + 0.5);
}


void OrganMidiEvent::send_event(RtMidiOut& player) const
{
    std::array<uint8_t, MIDI_MESSAGE_SIZE> midi_message;
    size_t msg_size = 0U;
    if (m_event_code < make_midi_command_byte(0U, SPECIAL)) {
        midi_message[msg_size++] = m_event_code;

        if (m_byte1.has_value()) {
            midi_message[msg_size++] = m_byte1.value();

            if (m_byte2.has_value()) {
                midi_message[msg_size] = m_byte2.value();
                msg_size++;
            }
        }

        player.sendMessage(midi_message.data(), msg_size);
    }
}


void OrganMidiEvent::set_bank_config(const BankConfig& cfg)
{
    m_desired_bank_number = cfg.first;
    m_desired_mode_number = cfg.second;
}


BankConfig OrganMidiEvent::get_bank_config() const
{
    return {m_desired_bank_number, m_desired_mode_number};
}


bool OrganMidiEvent::is_mode_change_event() const
{
    return m_mode_change_event;
}


OrganMidiEvent& OrganMidiEvent::operator-=(const OrganMidiEvent &rhs)
{
    m_seconds -= rhs.m_seconds;
    m_midi_time -= rhs.m_midi_time;
    return *this;
}


void OrganMidiEvent::calculate_delta(const OrganMidiEvent& rhs)
{
    m_delta_time = m_seconds - rhs.m_seconds;
    m_delta = m_midi_time - rhs.m_midi_time;
}