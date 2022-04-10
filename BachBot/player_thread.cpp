/**
 * @file player_thread.cpp
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
 */


//  system includes
#include <exception>  //  std::runtime_error

//  module includes
// -none-

//  local includes
#include "player_thread.h"  //   local include


namespace {
const auto TICKS_PER_UI_REFRESH = 500U;
}

PlayerThread::PlayerThread(PlayerWindow* const frame, const uint32_t port_id) :
    wxThread(wxTHREAD_JOINABLE),
    m_mutex(),
    m_midi_event_queue(),
    m_bank_number{0U},
    m_mode_number{0U},
    m_frame{frame},
    m_midi_out(frame->m_midi_out),
    m_waiting{nullptr},
    m_current_time(),
    m_bank_change_delay()
{
    m_midi_out.openPort(port_id);
    m_bank_change_delay.Start();
}


wxThread::ExitCode PlayerThread::Entry()
{
    wxMutexLocker lock(m_mutex);
    [[maybe_unused]] const auto start_result = timeBeginPeriod(1U);

    auto timer_callback = [](UINT, UINT, DWORD_PTR dwUser, DWORD_PTR, DWORD_PTR) {
        auto *const ptr = reinterpret_cast<PlayerThread*>(dwUser);
        ptr->post_tick();
    };
    const auto timer_id = timeSetEvent(
        1U, 1U, timer_callback, DWORD_PTR(this), TIME_PERIODIC);

    auto run = true;
    auto i = 0U;
    while (run && (m_midi_event_queue.size() > 0U)) {
        auto message = wait_for_message();
        switch (message.first)
        {
        case MessageId::ADVANCE_MESSAGE:
            force_advance();
            process_notes();
            break;

        case MessageId::STOP_MESSAGE:
            run = false;
            break;

        case MessageId::TICK_MESSAGE:
            if (++i >= TICKS_PER_UI_REFRESH) {
                i = 0U;
                wxThreadEvent tick_event(wxEVT_THREAD, PlayerEvents::TICK_EVENT);
                tick_event.SetInt(int((m_mode_number * 8U) + m_bank_number));
                wxQueueEvent(m_frame, tick_event.Clone());
            }
            process_notes();
            break;

        default:
            break;
        }
    }

    timeKillEvent(timer_id);
    const auto end_result = timeEndPeriod(1U);

    wxThreadEvent exit_event(wxEVT_THREAD, PlayerEvents::EXIT_EVENT);
    exit_event.SetInt(int(end_result));
    wxQueueEvent(m_frame, exit_event.Clone());

    return nullptr;
}


PlayerThread::Message PlayerThread::wait_for_message()
{
    if (m_event_queue.size() == 0U) {
        wxCondition signal(m_mutex);
        m_waiting = &signal;
        static_cast<void>(signal.Wait());
        m_waiting = nullptr;
    }

    if (m_event_queue.size() == 0U) {
        throw std::runtime_error("Event queue not posted.");
    }
    const auto message = m_event_queue.front();
    m_event_queue.pop_front();

    //  To reduce overhead, do the mode check while locked.
    if (MessageId::TICK_MESSAGE == message.first &&
        m_midi_event_queue.size() > 0U &&
        m_bank_change_delay.Time() > MINIMUM_BANK_CHANGE_INTERVAL_MS) {
            do_mode_check();
    }

    return message;
}


void PlayerThread::post_message(const MessageId msg_id, const uintptr_t value)
{
    wxMutexLocker lock(m_mutex);
    m_event_queue.push_back({ msg_id, value });
    if (nullptr != m_waiting) {
        m_waiting->Signal();
    }
}


void PlayerThread::play(const std::list<OrganMidiEvent>& event_list)
{
    m_midi_event_queue = event_list;
    if (Create() != wxTHREAD_NO_ERROR) {
        wxLogError(wxT("Can't create thread!"));
        return;
    }

    Run();
}


void PlayerThread::process_notes()
{
    const auto time_now = m_current_time.TimeInMicro();
    do {
        const auto &midi_event = m_midi_event_queue.front();
        const auto timestamp = midi_event.get_us();
        if (timestamp > time_now) {
            break;
        }

        midi_event.send_event(m_midi_out);
        m_midi_event_queue.pop_front();
    } while (m_midi_event_queue.size() > 0U);
}


void PlayerThread::force_advance()
{
    const auto &current_event = m_midi_event_queue.front();
    const auto ms = current_event.get_us().GetValue();
    m_current_time.Start(long(ms / 1000LL));
}


size_t PlayerThread::get_events_remaining()
{
    wxMutexLocker lock(m_mutex);
    return m_midi_event_queue.size();
}


void PlayerThread::set_bank_config(const uint8_t current_bank,
                                   const uint32_t current_mode)
{
    wxMutexLocker lock(m_mutex);
    m_bank_number = current_bank;
    m_mode_number = current_mode;
    m_bank_change_delay.Start();
}


void PlayerThread::do_mode_check()
{
    const auto desired_mode = m_midi_event_queue.front().get_bank_config();
    if ((desired_mode.second < m_mode_number && m_bank_number > 0U) || 
        (desired_mode.second == m_mode_number && 0U == desired_mode.first))
    {
        //  The desired piston mode is *lower* than the current state:  We
        // can take a shortcut and use CLEAR to get to the start of this 
        // piston mode.
        m_frame->send_manual_message(SyndyneBankCommands::GENERAL_CANCEL);
        m_bank_number = 0U;
        m_bank_change_delay.Start();
    } else if (desired_mode.second < m_mode_number || 
               desired_mode.first < m_bank_number) {
        //  Either at the bottom of this piston mode and need to step down to
        // the top of the last one, or we just need to walk down to the desired
        // bank.
        m_frame->send_manual_message(SyndyneBankCommands::PREV_BANK);
        if (0U == m_bank_number) {
            --m_mode_number;
            m_bank_number = 8U;
        }
        --m_bank_number;
        m_bank_change_delay.Start();
    } else if (desired_mode.second > m_mode_number ||
               desired_mode.first > m_bank_number) {
        //  We need to go up, no shortcuts available.
        m_frame->send_manual_message(SyndyneBankCommands::NEXT_BANK);
        ++m_bank_number;
        if (m_bank_number >= 8U) {
            m_bank_number = 0U;
            ++m_mode_number;
        }
        m_bank_change_delay.Start();
    }

}


PlayerThread::~PlayerThread()
{
    m_midi_out.closePort();
}
