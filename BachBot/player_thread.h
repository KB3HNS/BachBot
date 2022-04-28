/**
 * @file player_thread.h
 * @brief Player thread
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
 * Organ event generation in a separate thread.  The thread is timed using the
 * windows multimedia timer interface as, under windows, it provides the most
 * accurate and deterministic event time source outside of a busy loop.  If
 * porting to another platform is desired, the multimedia timer _should_ be
 * able to be replaced with another thread that uses an empty select to post
 * the tick event to this thread.
 */

#pragma once

//  system includes
#include <cstdint>  //  uint32_t, uintptr_t, etc
#include <list>  //  std::list
#include <utility>  //  std::pair
#include <RtMidi.h>  //  RtMidiOut
#include <wx/wx.h>  //  wxCondition, wxThread, etc

//  module includes
// -none-

//  local includes
#include "player_window.h"
#include "common_defs.h"
#include "organ_midi_event.h"

namespace bach_bot {

/**
 * @brief wx Events issues by this class
 */
enum PlayerEvents : int
{
    /**
     * @brief Periodic message sent to UI to refresh screen.
     * @note
     * "Int" value is formatted as `(mode << 3) | bank`
     */
    TICK_EVENT = 1001,
    EXIT_EVENT  ///< On thread exit message "Int" is return code.
};


/**
 * @brief wxThread representing the real-time midi player
 */
class PlayerThread : public wxThread
{
    /**
     * @brief Internal messages used between threads
     */
    enum MessageId : uint32_t
    {
        TICK_MESSAGE,
        STOP_MESSAGE,
        START_MESSAGE,
        ADVANCE_MESSAGE
    };

    /**
     * @brief Internal message format for sending messages to the worker
     *        thread.
     */
    using Message = std::pair<MessageId, uintptr_t>;

public:
    /**
     * @brief Constructor
     * @param frame reference to main window 
     * @param port_id MIDI port to open to send events to
     */
    PlayerThread(ui::PlayerWindow *const frame, const uint32_t port_id);

    /**
     * @brief Thread-safe call to send MIDI stop to.
      */
    void signal_stop()
    {
        post_message(MessageId::STOP_MESSAGE);
    }

    /**
     * @brief Thread-safe call to send "Advance State" to.
     * @section DESCRIPTION
     * There is a use-case where music may want to sustain indefinately until
     * a certain point - this is mostly used during the singing of certain
     * psalms and canticles where a section of the music is held as a single,
     * untimed chord followed by a brief section of standard music progression.
     * This allows such a sequence to be easily generated in any standard
     * "sequencer" application and would require the "player" to click the
     * `Play/Advance` button which will force the Midi to the next state.
     */
    void signal_advance()
    {
        post_message(MessageId::ADVANCE_MESSAGE);
    }

    /**
     * @brief Play music, upon completion `EXIT_EVENT` will be issued.
     * @param event_list Parsed MIDI event list
     * @note 
     * Currently there is no check to prevent this from re-triggering.
     * Therefore, external logic should prevent this from being called until
     * after the `EXIT_EVENT` has been issued.
     * @sa PlayerEvents
     */
    void play(const std::list<OrganMidiEvent> &event_list);

    /**
     * @brief Thread-safe get the number of events remaining in the current
     *        MIDI event list.
     * @returns number of events remaining
     * @note this is intended to drive a simple progress bar in the UI.
     */
    size_t get_events_remaining();

    /**
     * @brief Set the current state of the organ bank externally.
     * @param current_bank current bank (1-8)
     * @param current_mode current piston mode (1-100)
     */
    void set_bank_config(const uint8_t current_bank, 
                         const uint32_t current_mode);

    virtual ~PlayerThread() override;

protected:
    virtual ExitCode Entry() override;

private:

    /**
     * @brief Callback to post timer tick events.
     */
    void post_tick()
    {
        post_message(MessageId::TICK_MESSAGE);
    }

    /**
     * @brief General message posting API
     * @param msg_id Message to be posted
     * @param value extra message data - meaning may be message specific.
     */
    void post_message(const MessageId msg_id, const uintptr_t value = 0U);
    
    /**
     * @brief Thread call: Block and wait for a message event
     * @returns next message to process
     */
    Message wait_for_message();
    
    /**
     * @brief Process midi notes continuouly until state >= now.
     */
    void process_notes();
    
    /**
     * @brief Advance logic to reset internal player time to the next event's
     *        time.
     */
    void force_advance();

    /**
     * @brief Internal logic to check the mode and (possibly) change it.
     */
    void do_mode_check();

    /**
     * @brief Generate the test pattern as a sequence to be played.
    */
    void generate_test_pattern();

    double generate_test_pattern(const SyndyneKeyboards keyboard, 
                                 double start_time);

    wxMutex m_mutex;  ///< Shared data are protected by mutex

    std::list<Message> m_event_queue;  ///< Current Thread-Safe event queue
    std::list<OrganMidiEvent> m_midi_event_queue;  ///< List of midi events

    bool m_playing_test_pattern;  ///<  Are we playing the test pattern?

    /**
     *  @brief Value of the current bank registration number that the organ
     *         _should_ be at.  Range 1-8 (reported as 0-7)
     */
    uint8_t m_bank_number;
    
    /**
     * @brief Value of the piston mode that organ _should_ be at.  Range 1-100.
     */
    uint32_t m_mode_number;

    ui::PlayerWindow *const m_frame;  ///<  Pointer to parent window
    RtMidiOut &m_midi_out;  ///<  Reference to MIDI port
    
    /**
     * @brief Thread signal that player pends on.  Set `nullptr_t` if thread is
     *        not waiting (prevents false signals)
     */
    wxCondition *m_waiting;

    wxStopWatch m_current_time;  ///<  Current time and event time measurement.
    wxStopWatch m_bank_change_delay;  ///<  Holdoff delay between bank changes
};

}  //  end bach_bot
