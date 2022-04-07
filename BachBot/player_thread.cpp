#include <vector>
#include <string>
#include <memory>
#include <exception>
#include <array>

#include "player_thread.h"


PlayerThread::PlayerThread(PlayerWindow* const frame, const uint32_t port_id) :
    wxThread(wxTHREAD_JOINABLE),
    m_mutex(),
    m_midi_event_queue(),
    m_bank_number{0U},
    m_mode_number{0U},
    m_frame{frame},
    m_midi_out(frame->m_midi_out),
    m_waiting{nullptr},
    m_current_time()
{
    m_midi_out.openPort(port_id);
}


wxThread::ExitCode PlayerThread::Entry()
{
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
            if (++i >= 500U) {
                if (++m_bank_number >= 8U) {
                    m_bank_number = 0U;
                    ++m_mode_number;
                }
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

    wxThreadEvent exit_event(wxEVT_THREAD, PlayerEvents::EXIT_EVENT);
    exit_event.SetInt(0);
    wxQueueEvent(m_frame, exit_event.Clone());
    timeKillEvent(timer_id);

    [[maybe_unused]] const auto end_result = timeEndPeriod(1U);
    return nullptr;
}


PlayerThread::Message PlayerThread::wait_for_message()
{
    m_mutex.Lock();
    if (m_event_queue.size() == 0U) {
        wxCondition signal(m_mutex);
        m_waiting = &signal;
        signal.Wait();
        m_waiting = nullptr;
    }

    if (m_event_queue.size() == 0U) {
        throw std::runtime_error("Event queue not posted.");
    }
    const auto message = m_event_queue.front();
    m_event_queue.pop_front();

    m_mutex.Unlock();
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
    std::array<uint8_t, MIDI_MESSAGE_SIZE> midi_message;
    const auto time_now = m_current_time.TimeInMicro();
    do {
        const auto &midi_event = m_midi_event_queue.front();
        const auto timestamp = midi_event.get_us();
        if (timestamp > time_now) {
            break;
        }

        size_t msg_size = 0U;
        if (midi_event.event_code < 0xF0) {
            midi_message[msg_size++] = midi_event.event_code;

            if (midi_event.byte1.has_value()) {
                midi_message[msg_size++] = midi_event.byte1.value();
                
                if (midi_event.byte2.has_value()) {
                    midi_message[msg_size++] = midi_event.byte2.value();
                }
            }

            m_midi_out.sendMessage(midi_message.data(), msg_size);
        }
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


PlayerThread::~PlayerThread()
{
    m_midi_out.closePort();
}
