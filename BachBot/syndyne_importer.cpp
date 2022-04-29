/**
* @file syndyne_importer.cpp
* @brief MIDI -> Syndyne Sequence conversion.
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
#include <algorithm>  //  std::clamp
#include <array>  //  std::array
#include <utility>  //  std::pair
#include <fmt/format.h>  //  fmt::format

//  module includes
// -none-

//  local includes
#include "syndyne_importer.h"  //  local include


namespace {
    using bach_bot::SyndyneKeyboards;
    using bach_bot::SyndyneBankCommands;

/** Map of keyboard index to keyboard channel */
const std::array<SyndyneKeyboards,
                 bach_bot::NUM_SYNDYNE_KEYBOARDS> g_keyboard_indexes = {
    SyndyneKeyboards::MANUAL1_GREAT, 
    SyndyneKeyboards::MANUAL2_SWELL, 
    SyndyneKeyboards::PETAL
};

/** Map drums to commands */
const std::pair<uint8_t, SyndyneBankCommands> g_drum_map[] = {
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
const std::array<uint8_t, 16U> g_channel_mapping = {
    SyndyneKeyboards::MANUAL2_SWELL, SyndyneKeyboards::MANUAL2_SWELL, SyndyneKeyboards::MANUAL2_SWELL,
    SyndyneKeyboards::MANUAL1_GREAT, SyndyneKeyboards::MANUAL1_GREAT, SyndyneKeyboards::MANUAL1_GREAT,
    SyndyneKeyboards::PETAL, SyndyneKeyboards::PETAL, SyndyneKeyboards::PETAL,
    std::numeric_limits<uint8_t>::max(),  //  (9) Drums - used for control
    SyndyneKeyboards::MANUAL2_SWELL, SyndyneKeyboards::MANUAL1_GREAT, SyndyneKeyboards::PETAL,
    SyndyneKeyboards::MANUAL2_SWELL, SyndyneKeyboards::MANUAL1_GREAT, SyndyneKeyboards::PETAL
};

/** Tempo to derive beat timing from if tempo does not exist in song. */
constexpr const auto DEFAULT_NO_TEMPO = 120.0;

}  //  end anonymous namespace


namespace bach_bot {

SyndineImporter::SyndineImporter(const std::string &file_name, 
                                 const uint32_t song_id) :
    m_midifile(),
    m_file_events(),
    m_current_state(),
    m_song_id{song_id},
    m_tempo_detected(),
    m_current_config{0U, 0U},
    m_time_scaling_factor{1.0},
    m_note_offset{0},
    m_drum_map()
{
    m_midifile.read(file_name);
    m_midifile.doTimeAnalysis();
    m_midifile.linkNotePairs();
    m_midifile.joinTracks();

    for (auto i = 0U; i < m_current_state.size(); ++i) {
        const auto keyboard_id = g_keyboard_indexes[i];
        for (auto j = 0U; j < m_current_state[i].size(); ++j) {
            m_current_state[i][j].set_keyboard(keyboard_id);
        }
    }

    for (const auto &i: g_drum_map) {
        m_drum_map[i.first] = i.second;
    }
}


void SyndineImporter::adjust_tempo(const int new_tempo)
{
    if (!m_tempo_detected.has_value()) {
        static_cast<void>(get_tempo());
    }

    if (m_tempo_detected.has_value()) {
        //  Current events are in time:
        // time(s) * current tempo (b/s) = beats
        // beats * 1/new tempo (s/b) = time
        // Therefore input time * (old tempo / new tempo) = new time
        // Ingore 60s/1m as that will cross out.
        //  Yeay, I still remember jr-high algebra 30 years later.
        m_time_scaling_factor = m_tempo_detected.value() / double(new_tempo);
    }
}


void SyndineImporter::set_bank_config(const uint8_t initial_bank,
                                      const uint32_t initial_mode)
{
    const auto clamped_bank = std::clamp<uint8_t>(initial_bank, 0U, 7U);
    const auto clamped_mode = std::clamp(initial_mode, 0U, 99U);
    m_current_config = std::make_pair(clamped_bank, clamped_mode);
}


size_t SyndineImporter::get_control_index(const int channel) const
{
    if (channel < 0 || channel > int(g_channel_mapping.size())) {
        throw std::runtime_error(fmt::format("Index out of range: {}",
                                             channel));
    }

    const auto keyboard = g_channel_mapping[size_t(channel)];
    auto index = std::numeric_limits<size_t>::max();
    for (auto i = 0U; i < g_keyboard_indexes.size(); ++i) {
        if (keyboard == g_keyboard_indexes[i]) {
            index = i;
            break;
        }
    }

    return index;
}


uint8_t SyndineImporter::remap_note(const int note, 
                                    const SyndyneKeyboards keyboard) const
{
    uint8_t low_limit = 36U;  //  Standard range of organ keys.
    uint8_t high_limit = 96U;
    if (SyndyneKeyboards::PETAL == keyboard) {
        high_limit = 67U;  //  Petal only goes up to G above middle-C
    }

    auto mapped_note = uint8_t(note + m_note_offset);
    while (mapped_note < low_limit) {
        mapped_note += MIDI_NOTES_IN_OCTAVE;  //  Bump 1 octave
    }
    while (mapped_note > high_limit) {
        mapped_note -= MIDI_NOTES_IN_OCTAVE;  //  Bump 1 octave down
    }
    return mapped_note;
}


void SyndineImporter::update_bank_event(const int note)
{
    const auto bank_event = m_drum_map.find(uint8_t(note));
    if (m_drum_map.end() != bank_event) {
        switch (bank_event->second) {
        case SyndyneBankCommands::GENERAL_CANCEL:
            m_current_config.first = 0U;
            break;

        case SyndyneBankCommands::PREV_BANK:
            if (0U == m_current_config.first) {
                m_current_config.first = 8U;
                --m_current_config.second;
            }
            --m_current_config.first;
            break;

        case SyndyneBankCommands::NEXT_BANK:
            ++m_current_config.first;
            if (m_current_config.first >= 8U) {
                m_current_config.first = 0U;
                ++m_current_config.second;
            }
            break;

        default:
            break;
        }
    }

    m_current_config.first = std::clamp(m_current_config.first, 
                                        uint8_t(0U), 
                                        uint8_t(7U));
    m_current_config.second = std::clamp(m_current_config.second,  0U,  99U);
}


void SyndineImporter::build_syndyne_sequence(const smf::MidiEventList &event_list)
{
    std::list<OrganMidiEvent> events;

    //  1st pass: Process all events
    for (auto i = 0; i < event_list.size(); ++i) {
        auto midi_event = event_list[i];
        midi_event.seconds *= m_time_scaling_factor;
        if (midi_event.isNote()) {
            const auto channel_id = get_control_index(midi_event.getChannel());
            if (channel_id < m_current_state.size()) {
                const auto note = remap_note(midi_event.getKeyNumber(), 
                                             g_keyboard_indexes[channel_id]);
                midi_event[2] = note;
                m_current_state[channel_id][note].add_event(midi_event);
            } else if (midi_event.isNoteOn()) {
                //  Treat as control event
                update_bank_event(midi_event.getKeyNumber());
                events.emplace_back(midi_event, m_current_config);
            }
        }
    }

    //  2nd pass: append all de-duplicated events
    for (const auto &i: m_current_state) {
        for (const auto &j: i) {
            j.append_events(events);
        }
    }

    //  3rd pass: sort by time
    events.sort();

    //  4th pass: update bank config, build output events
    m_file_events.clear();
    auto current_config = events.front().get_bank_config();
    for (auto &i: events) {
        if (i.is_mode_change_event()) {
            current_config = i.get_bank_config();
        } else {
            i.set_bank_config(current_config);
            m_file_events.push_back(i);
        }
    }

    //  5th pass: remove start dead time from song, assign song id, add start 
    // silence
    auto &initial_delay = m_file_events.front();
    auto last_element = initial_delay;
    for (auto &i : m_file_events) {
        i.m_song_id = m_song_id;
        i -= initial_delay;
        i.calculate_delta(last_element);
        last_element = i;
    }
}


void SyndineImporter::adjust_key(int offset_steps)
{
    m_note_offset = uint8_t(std::clamp(offset_steps, 
                                       -MIDI_NOTES_IN_OCTAVE, 
                                       MIDI_NOTES_IN_OCTAVE));
}


std::optional<int> SyndineImporter::get_tempo()
{
    if (!m_tempo_detected.has_value()) {
        for (auto i = 0; i < m_midifile[0].size(); ++i) {
            const auto &evt = m_midifile[0][i];
            if (evt.isTempo()) {
                m_tempo_detected = evt.getTempoBPM();
                break;
            }
        }
    }

    std::optional<int> tempo;
    if (m_tempo_detected.has_value()) {
        tempo = int(m_tempo_detected.value() + 0.5);
    }

    return tempo;
}


std::list<OrganMidiEvent> SyndineImporter::get_events(
    const double initial_delay_beats, const double extend_final_duration)
{
    build_syndyne_sequence(m_midifile[0]);
    return std::move(m_file_events);
}

}
