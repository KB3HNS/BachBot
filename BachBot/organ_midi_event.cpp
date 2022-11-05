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
#include <limits>  //  std::numeric_limits
#include <stdexcept>  //  std::runtime_error

//  module includes
// -none-

//  local includes
#include "organ_midi_event.h"  //  Local include


namespace bach_bot {

OrganMidiEvent::OrganMidiEvent(const smf::MidiEvent& midi_event,
                               const SyndyneKeyboards channel) :
    m_event_code{make_midi_command_byte(channel, MidiCommands::SPECIAL)},
    m_mode_change_event{false},
    m_desired_memory{1U},
    m_desired_mode_number{1U},
    m_seconds{midi_event.seconds},
    m_delta_time{0.0},
    m_byte1(),
    m_byte2(),
    m_metadata(),
    m_midi_time{midi_event.tick},
    m_delta{0},
    m_partner{nullptr},
    m_song_id{std::numeric_limits<uint32_t>::max()}
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


OrganMidiEvent::OrganMidiEvent(const MidiCommands command,
                               const SyndyneKeyboards channel,
                               const int8_t byte1,
                               const int8_t byte2) :
    m_event_code{make_midi_command_byte(channel, command)},
    m_mode_change_event{(MidiCommands::CONTROL_CHANGE == command)},
    m_desired_memory{1U},
    m_desired_mode_number{1U},
    m_seconds{0.0},
    m_delta_time{0.0},
    m_byte1(),
    m_byte2(),
    m_metadata(),
    m_midi_time{0},
    m_delta{0},
    m_partner{nullptr},
    m_song_id{std::numeric_limits<uint32_t>::max()}
{
    if (byte1 >= 0) {
        m_byte1 = uint8_t(byte1);
    }
    if (byte2 >= 0) {
        m_byte2 = uint8_t(byte2);
    }
}


OrganMidiEvent::OrganMidiEvent(const int metadata_value,
                               const OrganMidiEvent *const src) :
    m_event_code{make_midi_command_byte(0U, MidiCommands::SPECIAL)},
    m_mode_change_event{false},
    m_desired_memory{1U},
    m_desired_mode_number{1U},
    m_seconds{0.0},
    m_delta_time{0.0},
    m_byte1(),
    m_byte2(),
    m_metadata(metadata_value),
    m_midi_time{0},
    m_delta{0},
    m_partner{nullptr},
    m_song_id{std::numeric_limits<uint32_t>::max()}
{
    if (nullptr != src) {
        set_bank_config(src->get_bank_config());
        m_seconds = src->m_seconds;
        m_delta_time = src->m_delta_time;
        m_midi_time = src->m_midi_time;
        m_delta = src->m_delta;
        m_song_id = src->m_song_id;
    }
}


OrganMidiEvent::OrganMidiEvent(const smf::MidiEvent &midi_event,
                               const BankConfig &cfg) :
    m_event_code{make_midi_command_byte(uint8_t(midi_event.getChannel()),
                                        MidiCommands::SPECIAL)},
    m_mode_change_event{true},
    m_desired_memory{cfg.memory},
    m_desired_mode_number{cfg.mode},
    m_seconds{midi_event.seconds},
    m_delta_time{0.0},
    m_byte1(),
    m_byte2(),
    m_metadata(),
    m_midi_time{midi_event.tick},
    m_delta{0},
    m_partner{nullptr},
    m_song_id{std::numeric_limits<uint32_t>::max()}
{
}


wxLongLong OrganMidiEvent::get_us() const
{
    const auto us_time = m_seconds * 1000000.0;
    return wxLongLong(us_time + 0.5);
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
                midi_message[msg_size++] = m_byte2.value();
            }
        }

        player.sendMessage(midi_message.data(), msg_size);
    }
}


void OrganMidiEvent::set_bank_config(const BankConfig& cfg)
{
    m_desired_memory = cfg.memory;
    m_desired_mode_number = cfg.mode;
}


BankConfig OrganMidiEvent::get_bank_config() const
{
    return {m_desired_memory, m_desired_mode_number};
}


bool OrganMidiEvent::is_mode_change_event() const
{
    return m_mode_change_event;
}


void OrganMidiEvent::calculate_delta(const OrganMidiEvent& rhs)
{
    m_delta_time = m_seconds - rhs.m_seconds;
    m_delta = m_midi_time - rhs.m_midi_time;
}


void OrganMidiEvent::offset_time(const double seconds, const int ticks)
{
    m_seconds -= seconds;
    m_midi_time -= ticks;
}


OrganMidiEvent::~OrganMidiEvent()
{
    if (nullptr != m_partner) {
        m_partner->m_partner = nullptr;
        m_partner = nullptr;
    }
}


OrganNote::OrganNote(OrganMidiEvent *const ptr) :
    std::shared_ptr<OrganMidiEvent>(ptr)
{
}


OrganNote::OrganNote(const OrganMidiEvent &rhs) :
    OrganNote(new OrganMidiEvent(rhs))
{
    get()->m_partner = nullptr;
}


void OrganNote::link(OrganNote &rhs) const
{
    auto *const this_inst = get();
    if (nullptr == this_inst || nullptr == rhs.get()) {
        throw std::runtime_error("link() on null instance");
    }
    this_inst->m_partner = rhs.get();
    rhs->m_partner = this_inst;
}


bool OrganNote::operator< (const OrganNote& rhs) const
{
    const auto *const this_event = get();
    if (nullptr == this_event || nullptr == rhs.get()) {
        throw std::runtime_error("operator< on null instance");
    }

    const auto time_compare = (this_event->m_seconds < rhs->m_seconds);
    if (rhs->m_midi_time == this_event->m_midi_time) {
        if (this_event->is_mode_change_event() == rhs->is_mode_change_event()) {
            return time_compare;
        }
        return rhs->is_mode_change_event();
    }
    return time_compare;
}


OrganMidiEvent OrganNote::clone() const
{
    auto cloned_event = *get();
    cloned_event.m_partner = nullptr;
    return cloned_event;
}


BankConfig::BankConfig(int msgdata) :
    BankConfig()
{
    const auto current_setting = uint32_t(msgdata);
    mode = uint8_t(current_setting & 0xFF);
    memory = current_setting >> 8U;
}


BankConfig::operator int() const
{
    const auto mem = memory << 8U;
    return int(mem) + int(mode);
}

}  //  end bach_bot
