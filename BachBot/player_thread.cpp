#include <vector>
#include <string>
#include <memory>
#include <exception>
#include <array>

#include "player_thread.h"

PlayerThread::PlayerThread(wxFrame* const frame) :
    wxThread(wxTHREAD_JOINABLE),
    m_mutex(),
    m_midi_event_queue(),
    m_bank_number{0U},
    m_mode_number{0U},
    m_frame{frame},
    m_midi_out(),
    m_waiting{nullptr},
    m_current_time{TimeReference::now()},
    m_current_ms{0ULL}
{

}


wxThread::ExitCode PlayerThread::Entry()
{
    // std::vector<uint8_t> midi_message(3);
    std::vector<std::string> port_names;
    [[maybe_unused]] const auto start_result = timeBeginPeriod(1U);

    auto timer_callback = [](UINT, UINT, DWORD_PTR dwUser, DWORD_PTR, DWORD_PTR) {
        auto *const ptr = reinterpret_cast<PlayerThread*>(dwUser);
        ptr->post_tick();
    };
    const auto timer_id = timeSetEvent(
        1U, 1U, timer_callback, DWORD_PTR(this), TIME_PERIODIC);

    for (auto i = 0U; i < m_midi_out.getPortCount(); ++i) {
        port_names.push_back(m_midi_out.getPortName(i));
    }
    // m_midi_out.openPort(1U);
    m_midi_out.openPort();

    // midi_message[0U] = 192U;
    // midi_message[1U] = 68U;
    // m_midi_out.sendMessage(midi_message.data(), 2U);
    auto run = true;

    auto note = 0U;
    /*auto next_note = [&]() {
        if (note > 0U) {
            midi_message[2] = 0U;
            m_midi_out.sendMessage(midi_message.data(), midi_message.size());
        }

        ++note;
        midi_message[1] = uint8_t(note & 0x7FU);
        midi_message[2] = 90U;
        m_midi_out.sendMessage(midi_message.data(), midi_message.size());
    };*/

    auto i = 0U;
    // midi_message[0] = 0x90;
    while (run && (m_midi_event_queue.size() > 0U)) {
        auto message = wait_for_message();
        switch (message.first)
        {
        case MessageId::ADVANCE_MESSAGE:
            // next_note();
            force_advance();
            process_notes();
            break;

        case MessageId::STOP_MESSAGE:
            run = false;
            break;

        case MessageId::TICK_MESSAGE:
            if (++i >= 500U) {
                i = 0U;
            //     next_note();
                wxThreadEvent tick_event(wxEVT_THREAD, PlayerEvents::TICK_EVENT);
                tick_event.SetInt(int(++note));
                wxQueueEvent(m_frame, tick_event.Clone());
            }
            process_notes();
            break;

        default:
            break;
        }
    }

    // midi_message[2] = 0U;
    // m_midi_out.sendMessage(midi_message.data(), midi_message.size());

    wxThreadEvent exit_event(wxEVT_THREAD, PlayerEvents::EXIT_EVENT);
    exit_event.SetInt(0);
    wxQueueEvent(m_frame, exit_event.Clone());
    // m_midi_out.closePort();
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
    std::array<uint8_t, 3U> midi_message;
    ++m_current_ms;
    do {
        const auto &midi_event = m_midi_event_queue.front();
        const auto timestamp = midi_event.get_ms();
        if (timestamp > m_current_ms) {
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
    m_current_ms = current_event.get_ms() - 1U;
}
