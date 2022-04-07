#pragma once

#include <cstdint>
#include <list>
#include <utility>
#include <RtMidi.h>
#include <wx/wx.h>

#include "player_window.h"
#include "common_defs.h"


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

public:
    PlayerThread(PlayerWindow *const frame, const uint32_t port_id);

    void signal_stop()
    {
        post_message(MessageId::STOP_MESSAGE);
    }

    void signal_advance()
    {
        post_message(MessageId::ADVANCE_MESSAGE);
    }

    void play(const std::list<OrganMidiEvent> &event_list);
    size_t get_events_remaining();

    virtual ~PlayerThread() override;

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
    RtMidiOut &m_midi_out;
    wxCondition *m_waiting;
    wxStopWatch m_current_time;
};

