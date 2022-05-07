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
#include <stdexcept>  //  std::runtime_error

//  module includes
// -none-

//  local includes
#include "player_thread.h"  //   local include


namespace {
constexpr const auto TICKS_PER_UI_REFRESH = 500U;
}


namespace bach_bot {

void send_bank_change_message(RtMidiOut &midi_out,
                              const SyndyneBankCommands value)
{
    std::array<uint8_t, MIDI_MESSAGE_SIZE> midi_message;
    midi_message[0] = make_midi_command_byte(0U, MidiCommands::CONTROL_CHANGE);
    midi_message[1] = SYNDYNE_CONTROLLER_ID;
    midi_message[2] = value;

    if (!midi_out.isPortOpen()) {
        throw std::runtime_error("Sending MIDI message on closed port");
    }

    midi_out.sendMessage(midi_message.data(), MIDI_MESSAGE_SIZE);
}


PlayerThread::PlayerThread(ui::PlayerWindow* const frame, const uint32_t port_id) :
    wxThread(wxTHREAD_JOINABLE),
    m_mutex(),
    m_event_queue(),
    m_midi_event_queue(),
    m_precache(),
    m_test_precache{false},
    m_first_song_id{0U},
    m_playing_test_pattern{false},
    m_bank_number{0U},
    m_mode_number{0U},
    m_frame{frame},
    m_midi_out(frame->m_midi_out),
    m_waiting{nullptr},
    m_current_time(),
    m_bank_change_delay(),
    m_power_control(wxPOWER_RESOURCE_SYSTEM, "BachBot Playing"),
    m_screen_control(wxPOWER_RESOURCE_SCREEN, "BachBot Playing")
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

    if (m_playing_test_pattern) {
        generate_test_pattern();
        wxThreadEvent start_event(wxEVT_THREAD,
                                  ui::PlayerWindowEvents::SONG_START_EVENT);
        start_event.SetInt(0);
        wxQueueEvent(m_frame, start_event.Clone());
        static_cast<void>(run_song());
    } else {
        run_playlist();
    }

    timeKillEvent(timer_id);
    const auto end_result = timeEndPeriod(1U);

    wxThreadEvent exit_event(wxEVT_THREAD,
                             ui::PlayerWindowEvents::EXIT_EVENT);
    exit_event.SetInt(int(end_result));
    wxQueueEvent(m_frame, exit_event.Clone());

    return nullptr;
}


bool PlayerThread::run_song()
{
    auto run = true;
    auto i = 0U;
    m_current_time.Start();
    m_test_precache = false;
    while (run && (m_midi_event_queue.size() > 0U)) {
        auto message = wait_for_message();
        switch (message.first) {
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
                wxThreadEvent tick_event(wxEVT_THREAD,
                                         ui::PlayerWindowEvents::TICK_EVENT);
                tick_event.SetInt(int(m_midi_event_queue.size()));
                wxQueueEvent(m_frame, tick_event.Clone());
            }
            process_notes();
            break;

        default:
            break;
        }
    }

    wxThreadEvent end_event(wxEVT_THREAD,
                            ui::PlayerWindowEvents::SONG_END_EVENT);
    end_event.SetInt(int(run));
    wxQueueEvent(m_frame, end_event.Clone());
    return run;
}


void PlayerThread::run_playlist()
{
    auto song_id = m_first_song_id;
    auto &playlist = m_frame->m_playlist;
    while (song_id > 0U) {
        if (m_precache.size() > 0U) {
            const auto midi_event = m_precache.front();
            if (midi_event->m_song_id == song_id) {
                m_midi_event_queue = std::move(m_precache);
            }
        }
        if (m_midi_event_queue.size() == 0U) {
            //  Pre-cached data was invalid, slow copy-construct
            m_midi_event_queue = playlist.get_song_events(song_id);
        }

        wxThreadEvent tick_event(wxEVT_THREAD,
                                 ui::PlayerWindowEvents::SONG_START_EVENT);
        tick_event.SetInt(int(song_id));
        wxQueueEvent(m_frame, tick_event.Clone());

        if (!run_song()) {
            break;
        }

        //  Need to re-check song config here in case UI changed the playlist.
        auto lock  = playlist.lock();
        const auto &song = playlist.get_playlist_entry(song_id);
        song_id = (song.play_next ? song.next_song_id : 0U);
    }
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
    if ((MessageId::TICK_MESSAGE == message.first) && 
        !m_playing_test_pattern &&
        (m_midi_event_queue.size() > 0U) &&
        (m_bank_change_delay.Time() > MINIMUM_BANK_CHANGE_INTERVAL_MS)) {
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


void PlayerThread::play(const uint32_t song_id)
{
    if (0U == song_id) {
        m_playing_test_pattern = true;
    } else {
        m_first_song_id = song_id;
        m_playing_test_pattern = false;
    }
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
        const auto timestamp = midi_event->get_us();
        if ((midi_event->m_metadata.has_value()) &&
            (midi_event->m_metadata.value() == LAST_NOTE_META_CODE) &&
            !m_test_precache) {
                precache_next_song(midi_event->m_song_id);
        }
        if (timestamp > time_now) {
            break;
        }

        midi_event->send_event(m_midi_out);
        if (m_playing_test_pattern) {
            m_bank_number = (midi_event->m_event_code & 0x0FU) - 1U;
            m_mode_number = midi_event->m_byte1.value() - 1U;
        }
        m_midi_event_queue.pop_front();
    } while (m_midi_event_queue.size() > 0U);
}


void PlayerThread::force_advance()
{
    const auto &current_event = m_midi_event_queue.front();
    const auto ms = current_event->get_us().GetValue();
    m_current_time.Start(long(ms / 1000LL));
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
    auto send_change = [&](const SyndyneBankCommands value) {
        send_bank_change_message(m_midi_out, value);
        wxThreadEvent tick_event(wxEVT_THREAD,
                                 ui::PlayerWindowEvents::BANK_CHANGE_EVENT);
        tick_event.SetInt(int((m_mode_number * 8U) + m_bank_number));
        wxQueueEvent(m_frame, tick_event.Clone());
        m_bank_change_delay.Start();
    };

    const auto desired_mode = m_midi_event_queue.front()->get_bank_config();
    if ((desired_mode.second < m_mode_number && m_bank_number > 0U) || 
        (desired_mode.second == m_mode_number && 0U == desired_mode.first))
    {
        //  The desired piston mode is *lower* than the current state:  We
        // can take a shortcut and use CLEAR to get to the start of this 
        // piston mode.
        m_bank_number = 0U;
        send_change(SyndyneBankCommands::GENERAL_CANCEL);
    } else if (desired_mode.second < m_mode_number || 
               desired_mode.first < m_bank_number) {
        //  Either at the bottom of this piston mode and need to step down to
        // the top of the last one, or we just need to walk down to the desired
        // bank.
        if (0U == m_bank_number) {
            --m_mode_number;
            m_bank_number = 8U;
        }
        --m_bank_number;
        send_change(SyndyneBankCommands::PREV_BANK);
    } else if (desired_mode.second > m_mode_number ||
               desired_mode.first > m_bank_number) {
        //  We need to go up, no shortcuts available.
        ++m_bank_number;
        if (m_bank_number >= 8U) {
            m_bank_number = 0U;
            ++m_mode_number;
        }
        send_change(SyndyneBankCommands::NEXT_BANK);
    }
}


void PlayerThread::generate_test_pattern()
{
    auto midi_time = 0.0;
    midi_time = generate_test_pattern(SyndyneKeyboards::PETAL, midi_time);
    midi_time = generate_test_pattern(SyndyneKeyboards::MANUAL1_GREAT, 
                                      midi_time);
    generate_test_pattern(SyndyneKeyboards::MANUAL2_SWELL, midi_time);
}


double PlayerThread::generate_test_pattern(const SyndyneKeyboards keyboard, 
                                           double start_time)
{
    for (uint8_t i = 1U; i <= 127U; ++i) {
        m_midi_event_queue.emplace_back(
            new OrganMidiEvent(MidiCommands::NOTE_ON, keyboard,
                               i, SYNDYNE_NOTE_ON_VELOCITY));
        m_midi_event_queue.back()->m_seconds = start_time;
        start_time += 1.0;
        m_midi_event_queue.emplace_back(
            new OrganMidiEvent(MidiCommands::NOTE_OFF, keyboard, i, 0U));
        m_midi_event_queue.back()->m_seconds = start_time;
    }

    return start_time;
}


void PlayerThread::precache_next_song(const uint32_t song_id)
{
    m_test_precache = true;
    m_precache.clear();

    auto lock  = m_frame->m_playlist.lock();
    const auto &song = m_frame->m_playlist.get_playlist_entry(song_id);
    const auto next_song_id = song.next_song_id;
    lock.reset();
    if (0U != next_song_id) {
        m_precache = m_frame->m_playlist.get_song_events(next_song_id);
    }
}


PlayerThread::~PlayerThread()
{
    m_midi_out.closePort();
}

}  //  end bach_bot
