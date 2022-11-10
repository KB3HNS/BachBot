/**
 * @file common_defs.h
 * @brief Common definitions used throughout application
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
 * These are the values that define aspects of either the MIDI standard,
 * the Syndyne computer used in the organ console, or the "magic" timing values
 * used for note-deduplication and retriggering.
 */

#pragma once

//  system includes
#include <cstdint>  //  uintXX_t
#include <cstdlib>  //  size_t
#include <array>  //  std::array

//  module includes
// -none-

//  local includes
// -none-

namespace bach_bot {

/**
 * @brief Bank control command values sent to "General Controller 1"
 */
enum SyndyneBankCommands : uint8_t
{
    GENERAL_CANCEL = 0U,
    PREV_BANK = 1U,
    NEXT_BANK = 2U
};

/**
 * @brief MIDI Channels (tracks) associated with each keyboard.
 */
enum SyndyneKeyboards : uint8_t
{
    MANUAL1_GREAT = 1U,  ///<  Bottom keyboard
    MANUAL2_SWELL,  ///<  Top keyboard
    PETAL
};

/** Number of keyboards in the console */
constexpr const size_t NUM_SYNDYNE_KEYBOARDS = 3U;

/** Organ "controller" `general controller 1` */
constexpr const uint8_t SYNDYNE_CONTROLLER_ID = 16U;

/** Optimal Note-On velocity to use */
constexpr const uint8_t SYNDYNE_NOTE_ON_VELOCITY = 127U;

/** Message size of [nearly] all midi events */
constexpr const size_t MIDI_MESSAGE_SIZE = 3U;

/** Tempo to derive beat timing from if tempo does not exist in song. */
constexpr const auto DEFAULT_NO_TEMPO = 120;

/**
 * @brief Adding or subtracting this from the note yields a note 1 octave up
 *        or down from the current note.
 */
constexpr const auto MIDI_NOTES_IN_OCTAVE = 12;

/**
 * @brief Simple definition of a complete event tracking table.
 * @tparam T Type associated with each keyboard-note combination.
 */
template <typename T>
using SyndyneMidiEventTable = std::array<std::array<T, 127U>,
                                         NUM_SYNDYNE_KEYBOARDS>;

/**
 * @brief The official command set for General Midi
 */
enum MidiCommands : uint8_t
{
    NOTE_OFF = 0x8U,
    NOTE_ON = 0x9U,
    AFTERTOUCH = 0xAU,
    CONTROL_CHANGE = 0xBU,
    PATCH_CHANGE = 0xCU,
    CHANNEL_PRESSURE = 0xDU,
    PITCH_BLEND = 0xEU,
    SPECIAL = 0xFU
};

/**
 * @brief Helper function to make a MIDI command byte from a command and a
 *        corresponding channel.
 * @param chan MIDI tack/channel (0-15)
 * @param command MIDI command
 */
constexpr uint8_t make_midi_command_byte(const uint8_t chan,
                                         const MidiCommands command)
{
    return uint8_t(command << 4U) | (chan & 0x0FU);
}


/*! @addtogroup magic_constants
 * Midi timing magic constants
 * @{
 */

/** Minimum gap between notes */
constexpr const auto MINIMUM_NOTE_GAP_S = 0.09;
/** Minimum length of one note */
constexpr const auto MINIMUM_NOTE_LENGTH_S = 0.045;

/**
 * @brief Minimum delay between consecutive bank-change commands.
 * @note sending bank change too rapidly can cause permanent damage to the
 *       console.  Don't reduce this!  Period!  I don't care what it sound
 *       like, just don't do it!
 */
constexpr const auto MINIMUM_BANK_CHANGE_INTERVAL_MS = 250L;
/*! @} */


/*! @addtogroup metadata events
 * Global set of metadata constants.  These should be able to slot in with
 * the user space of signal integers.  Most classes will have their own set
 * starting at 1001.  Negative values are OK for events that don't generate a
 * signal event.
 * @{
 */

 /** Meta event added to set first-note initial delay. */
constexpr const auto EMPTY_FIRST_META_EVENT = -900;

/** Last duration in a song set by the syndyne importer */
constexpr const auto LAST_NOTE_META_CODE = -901;
/**
 * @brief Meta code for an event generated that indicates sequence is that of
 *        the test pattern.
 */
constexpr const auto TEST_PATTERN_META_CODE = -902;

/*! @} */

}  //  end bach_bot
