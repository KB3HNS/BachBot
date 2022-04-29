/**
* @file syndyne_importer.h
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
*
* @section DESCRIPTION
* This is the top-level container class for converting raw MIDI to a sequence
* understood and that can be played by the organ.  Importing requires multiple
* passes through the MIDI sequence applying different transforms at each step
* with the final result being a list of OrganMidiEvents with appropriate timing
* information that can be sent to the organ using a timer.  This application
* uses the Windows Multimedia Timer (or an empty select loop in the case of
* Linux) as the timing generator.  Timing should be accurate to < +/- 1ms or 
* better with an ideal timing accuracy of +/- 100nS.
*/


#pragma once

//  system includes
#include <cstdint>
#include <string>  //  std::string
#include <optional>  //  std::optional
#include <list>  //  std::list
#include <unordered_map>  //  std::unordered_map

//  module includes
#include "MidiFile.h"  //  smf::MidiFile

//  local includes
#include "midi_note_tracker.h"  //  MidiNoteTracker
#include "organ_midi_event.h"  //  OrganMidiEvent
#include "common_defs.h"  //  MIDI Event definitions

namespace bach_bot {

/**
 * @brief Class that contains all logic to translate a MIDI file into an
 *        organ MIDI event sequence.
*/
class SyndineImporter
{
public:
    /**
     * @brief Constructor
     * @param file_name read midi data from file path
     * @param song_id assign song ID to events
     */
    SyndineImporter(const std::string &file_name, const uint32_t song_id);

    /**
     * @brief Adjust the tempo to increase / decrease playback speed
     * @param new_tempo adjust to tempo
     * @note This function has no effect if `get_tempo` returns false.
     */
    void adjust_tempo(const int new_tempo);

    /**
     * @brief Set the starting registration bank and piston position
     * @param initial_bank registration bank that song starts on
     * @param initial_mode desired piston mode to begin song
     */
    void set_bank_config(const uint8_t initial_bank, 
                         const uint32_t initial_mode);

    /**
     * @brief Adjust song up or down `offset_steps` half-steps
     * @param offset_steps half steps to adjust pitch (value is clamped at +/-
     *        1 octave)
     */
    void adjust_key(int offset_steps);

    /**
     * @brief Get the "reported" initial tempo of the song.
     * @returns tempo
     * @retval false song does not contain tempo information
     */
    std::optional<int> get_tempo();

    /**
     * @brief Get the sequence of timed organ midi events from song.
     * @param initial_delay_beats a number of empty "rest" beats before first 
                                  note-on event
     * @param extend_final_duration multiply the length of the final note
     *                              on duration by this value to extend the
     *                              final chord.
     * @returns timed event list to send to player 
     */
    std::list<OrganMidiEvent> get_events(const double initial_delay_beats,
                                         const double extend_final_duration);

private:
    /** 
    * @brief Get the the appropriate index of the channel event map based on
    *        the reported MIDI channel of a MIDI event.
    * @param channel midi event channel
    * @returns array index
    * @retval size_t::max Channel is a control channel
    */
    size_t get_control_index(const int channel) const;

    /**
    * @brief Remap a MIDI note to a note that exists on the appropriate
    *        keyboard.
    * @param note MIDI note ID
    * @param keyboard note is being assigned to
    * @retval MIDI note that can be corectly represented on the keyboard
    */
    uint8_t remap_note(const int note, const SyndyneKeyboards keyboard) const;

    /**
    * @brief Update the current state of the configuration based on a drum
    *        channel note.
    * @param note note ID
    */
    void update_bank_event(const int note);

    /**
    * @brief Logic to build an appropriate midi sequence to send to the organ
    * @param value message to send
    */
    void build_syndyne_sequence(const smf::MidiEventList &event_list);

    smf::MidiFile m_midifile;  ///< parsed midi events
    std::list<OrganMidiEvent> m_file_events;  ///< intermediate events
    /** Array of tracks & notes */
    SyndyneMidiEventTable<MidiNoteTracker> m_current_state;
    const uint32_t m_song_id;  ///< Requested song ID
    std::optional<double> m_tempo_detected;  ///< detected song tempo
    BankConfig m_current_config;   ///< current bank/piston setting
    double m_time_scaling_factor;  ///< calculated tempo time skew
    int8_t m_note_offset;  ///< key adjustment
    /** Drum track -> Bank Change Event translation */
    std::unordered_map<uint8_t, SyndyneBankCommands> m_drum_map;
};

}  //  end bach_bot
