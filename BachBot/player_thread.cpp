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
#include <memory>  //  std::unique_ptr
#include <wx/config.h>  //  wxConfig API

//  module includes
// -none-

//  local includes
#include "player_thread.h"  //   local include
#include "player_window.h"  //  PlayerWindowEvents
#include "rt_timer.h"  //  RTTimer


namespace {
constexpr const auto TICKS_PER_UI_REFRESH = 500U;
constexpr std::array g_chan_map = {
    bach_bot::SyndyneKeyboards::MANUAL1_GREAT,
    bach_bot::SyndyneKeyboards::MANUAL2_SWELL,
    bach_bot::SyndyneKeyboards::PEDAL
};


/**
 * @brief Get the "default" voice from the global configuration manager.
 * @return MIDI instrument to use
 * @note As a function so this can be called during construction
*/
uint8_t get_default_voice()
{
    auto config = wxConfig::Get();
    return uint8_t(config->ReadLong(L"voice/default_voice", 0));
}
}


namespace bach_bot {

void send_bank_change_message(RtMidiOut &midi_out,
                              const SyndyneBankCommands value)
{
    std::array<uint8_t, MIDI_MESSAGE_SIZE> midi_message;
    midi_message[0] = make_midi_command_byte(0U, MidiCommands::CONTROL_CHANGE);
    midi_message[1] = SYNDYNE_CONTROLLER_ID;
    midi_message[2] = uint8_t(value);

    if (!midi_out.isPortOpen()) {
        throw std::runtime_error("Sending MIDI message on closed port");
    }

    midi_out.sendMessage(midi_message.data(), MIDI_MESSAGE_SIZE);
}


PlayerThread::PlayerThread(wxFrame* const frame, RtMidiOut &intf) :
    wxThread(wxTHREAD_JOINABLE),
    m_mutex(),
    m_event_queue(),
    m_midi_event_queue(),
    m_precache(),
    m_test_precache{false},
    m_playing_test_pattern{false},
    m_memory_number{1U},
    m_mode_number{1U},
    m_desired_config(),
    m_frame{frame},
    m_midi_out(intf),
    m_waiting{nullptr},
    m_current_time(),
    m_bank_change_delay(),
    m_last_message{MessageId::NO_MESSAGE},
    m_first_match{false},
    m_desired_config_shared(),
    m_notes_on(),
    m_playback_voice{get_default_voice()}
{
    m_desired_config_shared = int(m_desired_config);
    m_bank_change_delay.Start(MINIMUM_BANK_CHANGE_INTERVAL_MS);
    for (auto &i : m_notes_on) {
        i.fill(0U);
    }
}


wxThread::ExitCode PlayerThread::Entry()
{
    std::unique_ptr<RTTimer> timer(create_timer(this));
    
    timer->start_timer();
    init_default_instrument();
    while (load_next_song()) {
        if (!run_song()) {
            break;
        }
    }

    timer->stop_timer();
    wxThreadEvent exit_event(wxEVT_THREAD,
                             ui::PlayerWindowEvents::EXIT_EVENT);
    exit_event.SetInt(0);
    wxQueueEvent(m_frame, exit_event.Clone());

    return nullptr;
}


bool PlayerThread::run_song()
{
    auto run = true;
    auto i = 0U;
    m_current_time.Start();
    m_test_precache = false;
    m_playing_test_pattern = false;

    const auto song_id = m_midi_event_queue.front().m_song_id;
    wxThreadEvent start_event(wxEVT_THREAD,
                              ui::PlayerWindowEvents::SONG_START_EVENT);
    start_event.SetInt(int(song_id));
    wxQueueEvent(m_frame, start_event.Clone());

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

            if (m_first_match) {
                process_notes();
            }
            break;

        default:
            break;
        }

        m_last_message = message.first;
    }

    wxThreadEvent end_event(wxEVT_THREAD,
                            ui::PlayerWindowEvents::SONG_END_EVENT);
    end_event.SetInt(int(run));
    wxQueueEvent(m_frame, end_event.Clone());
    return run;
}


PlayerThread::Message PlayerThread::wait_for_message()
{
    wxMutexLocker lock(m_mutex);

    //  To reduce overhead, do the mode check while locked.  Do before blocking
    // in order to reduce transition delays.
    if ((MessageId::TICK_MESSAGE == m_last_message) && 
        !m_playing_test_pattern &&
        (m_midi_event_queue.size() > 0U) &&
        (m_bank_change_delay.Time() > MINIMUM_BANK_CHANGE_INTERVAL_MS)) {
        do_mode_check();
    }

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


void PlayerThread::play()
{
    if (Create() != wxTHREAD_NO_ERROR) {
        wxLogError(wxT("Can't create thread!"));
        return;
    }

    Run();
}


void PlayerThread::enqueue_next_song(std::deque<OrganMidiEvent> song_events)
{
    wxMutexLocker lock(m_mutex);
    m_test_precache = true;
    m_precache = std::move(song_events);
}


BankConfig PlayerThread::get_desired_config() const
{
    const auto desired_config = int(m_desired_config_shared);
    return BankConfig(desired_config);
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

        if (!midi_event.is_mode_change_event()) {
            midi_event.send_event(m_midi_out);
            update_event_table(midi_event);
        }

        if (m_playing_test_pattern) {
            const BankConfig msg{uint32_t(midi_event.m_byte1.value()),
                                 uint8_t(midi_event.m_event_code & 0x0FU)};
            wxThreadEvent bank_event(wxEVT_THREAD,
                                     ui::PlayerWindowEvents::BANK_CHANGE_EVENT);
            bank_event.SetInt(int(msg));
            wxQueueEvent(m_frame, bank_event.Clone());
        } else {
            m_desired_config = midi_event.get_bank_config();
        }

        if (midi_event.m_metadata.has_value()) {
            handle_meta_event(midi_event.m_metadata.value());
        }

        m_midi_event_queue.pop_front();
    } while (m_midi_event_queue.size() > 0U);

    m_desired_config_shared = int(m_desired_config);
}


void PlayerThread::force_advance()
{
    const auto &current_event = m_midi_event_queue.front();
    const auto ms = current_event.get_us().GetValue();
    m_current_time.Start(long(ms / 1000LL));
    m_first_match = true;
}


void PlayerThread::set_bank_config(const uint32_t current_memory,
                                   const uint8_t current_mode)
{
    wxMutexLocker lock(m_mutex);
    m_memory_number = current_memory;
    m_mode_number = current_mode;
    m_bank_change_delay.Start();
}


void PlayerThread::do_mode_check()
{
    wxMutexLocker lock(m_mutex);

    if (m_desired_config.memory == m_memory_number &&
        m_desired_config.mode == m_mode_number)
    {
        //  Nothing to do.
        if (!m_first_match) {
            m_current_time.Start(0L);
            m_first_match = true;
        }
        return;
    }

    auto send_change = [&](const SyndyneBankCommands value) {
        send_bank_change_message(m_midi_out, value);
        wxThreadEvent bank_event(wxEVT_THREAD,
                                 ui::PlayerWindowEvents::BANK_CHANGE_EVENT);
        BankConfig config{m_memory_number, m_mode_number};
        bank_event.SetInt(int(config));
        wxQueueEvent(m_frame, bank_event.Clone());
        m_bank_change_delay.Start();
    };

    auto step_down = [=]() {
        if (0U == m_mode_number) {
            --m_memory_number;
            m_mode_number = 8U;
        } else {
            --m_mode_number;
        }

        send_change(SyndyneBankCommands::PREV_BANK);
    };

    if ((m_desired_config.memory < m_memory_number && m_mode_number > 0U) || 
        (m_desired_config.memory == m_memory_number &&
         1U == m_desired_config.mode && m_mode_number > 1U))
    {
        //  The desired memory is *lower* than the current state:  We can take
        // a shortcut and use CLEAR to get to the start of this piston mode.
        m_mode_number = 0U;
        send_change(SyndyneBankCommands::GENERAL_CANCEL);
    } else if (m_desired_config.memory < m_memory_number) {
        //  At the bottom of the piston position and need to step down to the
        // top of the last one.
        step_down();
    } else if (m_desired_config.memory > m_memory_number ||
               m_desired_config.mode > m_mode_number) {
        //  We need to go up, no shortcuts available.
        ++m_mode_number;
        if (m_mode_number > 8U) {
            m_mode_number = 1U;
            ++m_memory_number;
        }
        send_change(SyndyneBankCommands::NEXT_BANK);
    } else if (m_desired_config.mode < m_mode_number) {
        // We just need to walk down to the desired bank.
        step_down();
    }
}


void PlayerThread::precache_next_song(const uint32_t song_id)
{
    //  Nothing done here anymore.  This is just a stub in case I want to do
    // something else before the last note.
    static_cast<void>(song_id);
}


bool PlayerThread::load_next_song()
{
    wxMutexLocker lock(m_mutex);
    const auto song_size = m_precache.size();
    if (song_size > 0U) {
        m_desired_config = m_precache.front().get_bank_config();
        m_desired_config_shared = int(m_desired_config);
        m_midi_event_queue = std::move(m_precache);
    }

    return (song_size > 0U);
}


void PlayerThread::handle_meta_event(const int meta_event_id)
{
    switch (meta_event_id) {
    case LAST_NOTE_META_CODE:
        precache_next_song(m_midi_event_queue.front().m_song_id);
        break;

    case TEST_PATTERN_META_CODE:
        m_playing_test_pattern = true;
        break;

    default:
        if (meta_event_id > 0) {
            wxThreadEvent meta_event(wxEVT_THREAD,
                                     ui::PlayerWindowEvents::SONG_META_EVENT);
            meta_event.SetInt(meta_event_id);
            wxQueueEvent(m_frame, meta_event.Clone());
        }
    }
}


void PlayerThread::update_event_table(const OrganMidiEvent &event)
{
    if (!event.m_byte1.has_value() || !event.m_byte2.has_value()) {
        return;
    }

    auto note = event.m_byte1.value();
    auto vel = event.m_byte2.value();
    auto index = std::numeric_limits<size_t>::max();
    switch (event.m_event_code & 0x0F) {
    case SyndyneKeyboards::MANUAL1_GREAT:
        index = 0U;
        break;
    
    case SyndyneKeyboards::MANUAL2_SWELL:
        index = 1U;
        break;

    case SyndyneKeyboards::PEDAL:
        index = 2U;
        break;

    default:
        return;
    }

    m_notes_on[index][note] = vel;
}


void PlayerThread::init_default_instrument()
{
    std::array<uint8_t, MIDI_MESSAGE_SIZE> message;
    message.fill(0U);
    
    message[1U] = m_playback_voice;
    message[0U] = make_midi_command_byte(SyndyneKeyboards::MANUAL1_GREAT,
                                         MidiCommands::PATCH_CHANGE);
    m_midi_out.sendMessage(message.data(), message.size());

    message[0U] = make_midi_command_byte(SyndyneKeyboards::MANUAL2_SWELL,
                                         MidiCommands::PATCH_CHANGE);
    m_midi_out.sendMessage(message.data(), message.size());

    message[0U] = make_midi_command_byte(SyndyneKeyboards::PEDAL,
                                         MidiCommands::PATCH_CHANGE);
    m_midi_out.sendMessage(message.data(), message.size());
}


PlayerThread::~PlayerThread()
{
    std::array<uint8_t, MIDI_MESSAGE_SIZE> midi_message;
    midi_message[2U] = 0U;  //  All commands require the 3rd byte to be 0

    for (auto i = 0U; i < m_notes_on.size(); ++i) {
        auto &bank = m_notes_on[i];
        midi_message[0U] = make_midi_command_byte(g_chan_map[i],
                                                  MidiCommands::NOTE_OFF);

        for (auto note = 0U; note < bank.size(); ++note) {
            if (bank[note] > 0U) {
                midi_message[1U] = note;
                m_midi_out.sendMessage(midi_message.data(), MIDI_MESSAGE_SIZE);
            }
        }
    }

    //  This is modeled after Windows using a packet dump
    for (uint8_t i = 0U; i < 16U; ++i) {
        midi_message[0U] = make_midi_command_byte(
            i, MidiCommands::CONTROL_CHANGE);

        midi_message[1U] = 0x7B;  //  All notes off
        m_midi_out.sendMessage(midi_message.data(), MIDI_MESSAGE_SIZE);

        midi_message[1U] = 0x79;  //  All controllers off
        m_midi_out.sendMessage(midi_message.data(), MIDI_MESSAGE_SIZE);
    }
   
    m_midi_out.closePort();
}

}  //  end bach_bot
