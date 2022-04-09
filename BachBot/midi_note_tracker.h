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
 *
 * @section DESCRIPTION
 * Normal realtime midi playback devices implement a "5-phase" playback system 
 * when playing back a note:
 *   1. Silence
 *   2. Pre-strike
 *   3. Sustain
 *   4. Fade-out
 *   5. Silence
 * With this, the note-on event traversed states 1-3 and the note state will
 * hold at state 3 until the note off occurs and then the note will progress
 * through the ramining 2 states.  The organ does not work this way.  Things
 * like sequentially played notes will be played as a single sustained note.
 * Also, in MIDI it is technically illegal to have any individual note on the
 * same track be turned on more than once (with the caveat of a velocity
 * change).  This logic introduces some specific gaps and applies overlap logic
 * to handle when dealing with the fact that multiple midi tracks are
 * on-the-fly recombined into a single track for each of the keyboards (also
 * including out-of-band note shifting).  This enforces some experimentally and
 * musically determined guards to replicate what a real-time synth would do.
 */

#pragma once

//  system includes
#include <list>  //  std::list
#include <memory>  //  std::shared_ptr
#include <utility>  //  std::pari

//  module includes
#include "MidiEvent.h"  //  smf::MidiEven

//  local includes
#include "common_defs.h"  //  Orgain timing "magic numbers"
#include "organ_midi_event.h"  //  OrganMidiEvent


/**
 * @brief Midi note tracking.
 * @note Although not enforced, this is indended to represent a single
 *       keyboard-note combination.
 */
class MidiNoteTracker
{
    /**
     * @brief Simplify this type.  Organ event storage.
     */
    using OrganNote = std::shared_ptr<OrganMidiEvent>;

public:
    /**
     * @brief Constructor
     */
    MidiNoteTracker();

    /**
     * @brief Add a single event to this tracking logic
     * @param ev midi event
     * @note ev must be either a NoteOn or NoteOff event.
     */
    void add_event(const smf::MidiEvent &ev);

    /**
     * @brief Append our events to the list
     * @param[out] event_list current list of midi events
    */
    void append_events(std::list<OrganMidiEvent> &event_list) const;
    
    /**
     * @brief Set the keyboard that we will use.
     * @param keyboard_id keyboard to route all events to.
    */
    void set_keyboard(const SyndineKeyboards keyboard_id);

private:
    /**
     * @brief Logic for a new note-on event in the event list.
     * @param organ_ev event to process
     * @note this occurs *any* time that the note is turned on including 
     *       restrikes.
     */
    void process_new_note_on_event(OrganNote &organ_ev);
    
    /**
     * @brief Logic for a new not-off event in the event list.
     * @param organ_ev event to process
     * @note this will always append a complete (note-on+note-off pair).
     */
    void process_new_note_off_event(OrganNote &organ_ev);
    
    /**
     * @brief Insert a simulated note-off event in the case of a restrike.
     * @param organ_ev note-on event that caused the restrike.
    */
    void insert_off_event(OrganNote &organ_ev);
    
    /**
     * @brief Insert a simulated note-on event from last note-off.
     * @param organ_ev note-off event that necessitated the backfill.
     * @note
     * This occurs when multiple note-on events are active and the 
     * corresponding note-off events occur at different times.  In this event, 
     * the first note-off, which would have turned the channel off, was 
     * actually supposed to be a restrike and this note-off is the end of that
     * restrike event.
     */
    void backfill_on_event(OrganNote &organ_ev);

    bool m_on_now;  ///< Current state, note is on now
    bool m_last_event_was_on;  ///< The last event processed was a note-on

    /**
     * @brief MIDI time that the most recent"on" time occured
     * @note We use MIDI time here and not real time because the real-time may
     *       be shifted during the processing logic.
     */
    int m_midi_ticks_on_time;
    int m_last_midi_off_time;  ///< MIDI time of the most recent "note-off"

    uint32_t m_note_nesting_count;  ///<  Number of concurrent note-on events
    OrganNote m_note_on;  ///<  Shared pointer to the last "note-on" event
    OrganNote m_note_off;  ///<  Shared pointer to the last "note-off" event
    SyndineKeyboards m_keyboard;  ///<  Keyboard that events shall be routed to

    /**
     * @brief Event list for this note
     * @note
     *   - `first` is the note-on event,
     *   - `second` is the matching note-off event
     */
    std::list<std::pair<OrganNote, OrganNote>> m_event_list;
};
