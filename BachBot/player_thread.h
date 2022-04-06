#pragma once

#include <cstdint>
#include <list>
#include <utility>
#include <chrono>
#include <optional>
#include <RtMidi.h>
#include <wx/wx.h>


struct OrganMidiEvent
{
    uint8_t event_code;
    double seconds;
    double delta_time;
    std::optional<uint8_t> byte1;
    std::optional<uint8_t> byte2;
    uint8_t desired_bank_number;
    uint32_t desired_mode_number;
    std::chrono::microseconds duration;
    int delay;
    int delta;

    constexpr uint64_t get_ms() const
    {
        const auto ms_time = seconds * 1000.0;
        return uint64_t(ms_time + 0.5);
    }
};


enum PlayerEvents : int
{
    TICK_EVENT = 1001,
    EXIT_EVENT
};


class PlayerThread : public wxThread
{
    enum MessageId : uint32_t
    {
        TICK_MESSAGE,
        STOP_MESSAGE,
        START_MESSAGE,
        ADVANCE_MESSAGE
    };

    using Message = std::pair<MessageId, uintptr_t>;
    using TimeReference = std::chrono::high_resolution_clock;

public:
    PlayerThread(wxFrame *const frame);

    void signal_stop()
    {
        post_message(MessageId::STOP_MESSAGE);
    }
    void signal_advance()
    {
        post_message(MessageId::ADVANCE_MESSAGE);
    }

    void play(const std::list<OrganMidiEvent> &event_list);

protected:
    virtual ExitCode Entry() override;

private:

    void post_tick()
    {
        post_message(MessageId::TICK_MESSAGE);
    }

    void post_message(const MessageId msg_id, const uintptr_t value = 0U);
    Message wait_for_message();
    void process_notes();
    void force_advance();

    wxMutex m_mutex;

    std::list<Message> m_event_queue;
    std::list<OrganMidiEvent> m_midi_event_queue;

    uint8_t m_bank_number;
    uint32_t m_mode_number;
    wxFrame *const m_frame;
    RtMidiOut m_midi_out;
    wxCondition *m_waiting;
    std::chrono::time_point<TimeReference> m_current_time;
    uint64_t m_current_ms;
};

