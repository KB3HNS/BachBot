/**
 * @file rt_timer.h
 * @brief Realtime Timer interface
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
 * The Real Time event class is responsible for generatingthe PlayerThread's
 * `post_tick` event which is responsible for all MIDI timing.  This class is
 * also responsible for the platform specific power management control.
 */


#pragma once

//  system includes
// -none-

//  module includes
// -none-

//  local includes
#include "player_thread.h"  //  PlayerThread


namespace bach_bot {

/**
 * @brief Abstract interface of a real-time timer event control.
 * @note Disabling power management is expected for the lifetime of this
 * object.  The player thread stores this in a unique pointer and will be
 * responsible for it's destruction prior to thread exit.  All APIs will be
 * called exlusively from the timer thread class (including the constructor),
 * the player thread's `post_tick` may be safely called from any thread.
 */
class RTTimer
{
public:
    RTTimer(PlayerThread *const player) :
        m_player_thread{player}
    {
    }

    virtual void start_timer() = 0;

    virtual void stop_timer() = 0;

    virtual ~RTTimer() = default;

protected:
    
    /**
     * @brief Internal tick event - implementations call this to send the
     *        signal to the player thread instance.
     */
    void tick()
    {
        m_player_thread->post_tick();
    }

private:
    PlayerThread *const m_player_thread;
};


/**
 * @brief Create an instance of the timer.
 * @note individual ports are responsible for defining this function.
 * @param player pointer to player thread
 * @returns port-specific RTTimer instance
 */
extern RTTimer* create_timer(PlayerThread *const player);


}  //  end bach_bot
