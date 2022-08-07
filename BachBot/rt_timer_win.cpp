/**
 * @file rt_timer_win.cpp
 * @brief Realtime Timer interface (Windows platform port)
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
 */


//  system includes
#include <cassert>  //  assert
#include <optional>  //  std::optional
#include <wx/power.h>  //  wxPowerResourceBlocker
//  I don't know why, but it looks like I'm getting access to the  WinMM timer
// interface through this as well.  There doesn't seem to be a better way of
// including it that doesn't cause compilation issues elsewhere.

//  module includes
// -none-

//  local includes
#include "rt_timer.h"  //  RTTimer


namespace {

//  Keep this around for debug purposes.
[[maybe_unused]] MMRESULT s_last_exit_code;

class WindowsTimer : public bach_bot::RTTimer
{
public:
    WindowsTimer(bach_bot::PlayerThread *const player) :
        RTTimer(player),
        m_start_result{timeBeginPeriod(1U)},
        m_timer_id(),
        m_power_control(wxPOWER_RESOURCE_SYSTEM, "BachBot Playing"),
        m_screen_control(wxPOWER_RESOURCE_SCREEN, "BachBot Playing")

    {
    }

    virtual void start_timer() override
    {
        assert(!m_timer_id.has_value());
        auto timer_callback = [](UINT, UINT, DWORD_PTR dwUser, DWORD_PTR, DWORD_PTR) {
            auto *const ptr = reinterpret_cast<WindowsTimer*>(dwUser);
            ptr->tick();
        };
        m_timer_id = timeSetEvent(1U, 1U,
                                  timer_callback, DWORD_PTR(this),
                                  TIME_PERIODIC);
    }

    virtual void stop_timer() override
    {
        assert(m_timer_id.has_value());
        timeKillEvent(m_timer_id.value());
        m_timer_id.reset();
    }

    virtual ~WindowsTimer() override
    {
        s_last_exit_code = timeEndPeriod(1U);
    }

private:
    const MMRESULT m_start_result;
    std::optional<MMRESULT> m_timer_id;
    wxPowerResourceBlocker m_power_control;  ///<  Prevent low power mode
    wxPowerResourceBlocker m_screen_control;  ///<  Prevent screen blanking
};

}  //  end anonymous namespace


namespace bach_bot {

RTTimer* create_timer(PlayerThread *const player)
{
    return new WindowsTimer(player);
}

}  //  end bach_bot
