/**
 * @file midi_note_tracker.h
 * @brief Note deduplication logic
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
#include <cmath>  //  abs

//  module includes
// -none-

//  local includes
#include "midi_note_tracker.h"  //  local include


namespace {
/**
 * @brief Simple test to see if a midi event occurs at the same time as a
 *        currently tracked event.
 * @param ev event to test
 * @param organ_time time to test against
 * @retval `true` events occur at the same time
 * @retval `false` events are at different times
 */
bool is_same_time(const smf::MidiEvent &ev, const int organ_time)
{
    return (abs(ev.tick - organ_time) <= 1);
}

}  //  end anonymous namespace.


namespace bach_bot {

MidiNoteTracker::MidiNoteTracker() :
    m_on_now{false},
    m_midi_ticks_on_time{-1},
    m_last_midi_off_time{-1},
    m_note_nesting_count{0U},
    m_note_on(),
    m_note_off(),
    m_keyboard{SyndyneKeyboards::MANUAL2_SWELL},
    m_event_list()
{
}


void MidiNoteTracker::add_event(const smf::MidiEvent &ev,
                                const NoteConfig &note_config)
{
    auto organ_event = OrganNote(new OrganMidiEvent(ev, m_keyboard));

    if (ev.isNoteOn()) {
        if (!m_on_now) {
            //  New note
            process_new_note_on_event(organ_event, note_config.min_gap);
        } else if (ev.isNoteOn() && is_same_time(ev, m_midi_ticks_on_time)) {
            //  Note turned on twice at the same time
            ++m_note_nesting_count;
        } else {
            //  Note turned on during current note
            insert_off_event(organ_event, note_config.min_len);
            process_new_note_on_event(organ_event, note_config.min_gap);
            ++m_note_nesting_count;
        }
    } else if (ev.isNoteOff() && m_on_now) {
        if (0 == m_note_nesting_count) {
            process_new_note_off_event(organ_event, note_config.min_len);
        } else {
            //  Keep note on until all note off events have occurred
            --m_note_nesting_count;
        }
    }
}


void MidiNoteTracker::append_events(std::list<OrganNote> &event_list) const
{
    OrganNote grouped_note_on;
    double grouped_note_len = MINIMUM_NOTE_LENGTH_S;

    auto append_pair = [&](const OrganNote &note_on, const OrganNote &note_off) {
        event_list.emplace_back(note_on);
        auto &on_event = event_list.back();
        event_list.emplace_back(note_off);
        on_event.link(event_list.back());
        grouped_note_on.reset();
        grouped_note_len = MINIMUM_NOTE_LENGTH_S;
    };

    for (auto i = m_event_list.cbegin(); m_event_list.cend() != i; ++i) {
        if (i->off->m_seconds - i->on->m_seconds > i->min_note_len) {
            append_pair(i->on, i->off);
        } else if (grouped_note_on.get() == nullptr) {
            grouped_note_on = i->on;
            grouped_note_len = i->min_note_len;
        } else if (grouped_note_on.get() != nullptr) {
            auto avg_len = i->min_note_len + grouped_note_len;
            avg_len /= 2.0;
            auto grouped_length = i->off->m_seconds - grouped_note_on->m_seconds;
            if (grouped_length > avg_len) {
                append_pair(grouped_note_on, i->off);
            }
        }
    }
}


void MidiNoteTracker::set_keyboard(const SyndyneKeyboards keyboard_id)
{
    m_keyboard = keyboard_id;
}


void MidiNoteTracker::process_new_note_on_event(OrganNote &organ_ev,
                                                double min_gap)
{
    auto last_off_time = -1.0;
    if (nullptr != m_note_off.get()) {
        last_off_time = m_note_off->m_seconds;
    }

    if (organ_ev->m_seconds - last_off_time < min_gap) {
        const auto delta = min_gap / 2.0;
        m_note_off->m_seconds -= delta;
        organ_ev->m_seconds += delta;
    }

    m_note_on = organ_ev;
    m_midi_ticks_on_time = organ_ev->m_midi_time;
    m_on_now = true;
    organ_ev->m_byte2 = SYNDYNE_NOTE_ON_VELOCITY;
}


void MidiNoteTracker::process_new_note_off_event(OrganNote &organ_ev,
                                                 double min_length)
{
    m_note_off = organ_ev;
    m_on_now = false;
    m_last_midi_off_time = organ_ev->m_midi_time;
    organ_ev->m_byte2 = uint8_t(0U);
    m_event_list.push_back({m_note_on, m_note_off, min_length});
}


void MidiNoteTracker::insert_off_event(OrganNote &organ_ev, double min_length)
{
    OrganNote organ_event(*organ_ev);
    organ_event->m_event_code = make_midi_command_byte(m_keyboard,
                                                       MidiCommands::NOTE_OFF);
    process_new_note_off_event(organ_event, min_length);
}

}  //  end bach_bot
