/**
 * @file rt_timer_win.cpp
 * @brief Realtime Timer interface (POSIX platform port)
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
#include <cassert>  //  assert
#include <atomic>  //  std::atomic
#include <iostream>  //  std::cout
#include <sys/socket.h>  //  select, fd_set, etc

#ifdef __APPLE__
#include <wx/power.h>  //  wxPowerResourceBlocker - not supported in Linux
#endif

//  local includes
#include "rt_timer.h"  //  RTTimer


namespace {

constexpr const auto US_PER_MS = 1000ULL;  //  Seed at 1000uS (1ms)

/**
 * @brief Timer implementation using an empty `select` loop.
 *
 */
class PosixTimer : public bach_bot::RTTimer, wxThread
{
public:
    PosixTimer(bach_bot::PlayerThread *const player) :
        RTTimer(player),
        wxThread(wxTHREAD_JOINABLE),
        m_mutex(),
        m_current_time(),
#ifdef __APPLE__
        m_power_control(wxPOWER_RESOURCE_SYSTEM, "BachBot Playing"),
        m_screen_control(wxPOWER_RESOURCE_SCREEN, "BachBot Playing"),
#endif
        m_signal_stop{false},
        m_running{false}
    {
    }

    virtual void start_timer() override
    {
        assert(!m_running);
        m_running = true;
        Run();
    }

    virtual void stop_timer() override
    {
        assert(m_running);
        request_stop();
        Wait();
        m_running = false;
    }

    virtual ~PosixTimer() override
    {
        if (m_running) {
            request_stop();
            Wait();
        }
    }

protected:
    virtual ExitCode Entry() override
    {
        fd_set tx_set, rx_set, err_set;
        m_current_time.Start();
        auto loop_iter = 0;
        auto loop_count = 0U;
        uint64_t delay_us = US_PER_MS;

        auto start_time = m_current_time.TimeInMicro();
        while (true) {
            FD_ZERO(&tx_set);
            FD_ZERO(&rx_set);
            FD_ZERO(&err_set);
            auto tv = timeval();
            tv.tv_usec = suseconds_t(delay_us);
            static_cast<void>(select(1, &rx_set, &tx_set, &err_set, &tv));
            const auto end_time = m_current_time.TimeInMicro();

            tick();

            //  Calculate a ratio to adjust delay by to improve the sleep by.
            const auto start = uint64_t(start_time.GetValue());
            const auto end = uint64_t(end_time.GetValue());
            auto time = delay_us * US_PER_MS;
            time /= end - start;

            //  Apply a 1/64 average
            delay_us *= 63ULL;
            delay_us += time;
            delay_us >>= 6U;
            if (0ULL == delay_us) {
                delay_us = 1ULL;
            }

            ++loop_iter;
            if (loop_iter > 1000) {
                loop_iter = 0;
                ++loop_count;
                std::cout << "Loop: " << loop_count
                          << " Delay (est): " << delay_us
                          << " Diff: " << end - start
                          << std::endl;
            }

            if (check_stop()) {
                break;
            }

            start_time = end_time;
        }

        return nullptr;
    }

    void request_stop()
    {
        wxMutexLocker lock(m_mutex);
        m_signal_stop = true;
    }

    bool check_stop()
    {
        wxMutexLocker lock(m_mutex);
        const auto signal_stop = bool(m_signal_stop);
        m_signal_stop = false;
        return signal_stop;
    }

private:
    wxMutex m_mutex;
    wxStopWatch m_current_time;

#ifdef __APPLE__
    wxPowerResourceBlocker m_power_control;  ///<  Prevent low power mode
    wxPowerResourceBlocker m_screen_control;  ///<  Prevent screen blanking
#endif

    std::atomic<bool> m_signal_stop;
    std::atomic<bool> m_running;
};

}  //  end anonymous namespace


namespace bach_bot {

RTTimer* create_timer(PlayerThread *const player)
{
    return new PosixTimer(player);
}

}  //  end bach_bot
