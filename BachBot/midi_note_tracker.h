#pragma once

#include <list>

#include "MidiEvent.h"

#include "common_defs.h"
#include "organ_midi_event.h"


class MidiNoteTracker
{
public:
    MidiNoteTracker();

    void add_event(const smf::MidiEvent &ev);

    void append_events(std::list<OrganMidiEvent> &event_list) const;
    void set_keyboard(const SyndineKeyboards keyboard_id);

private:
    bool m_on_now;
    int m_midi_ticks_on_time;
    int m_last_midi_off_time;
    uint32_t m_note_nesting_count;
    OrganMidiEvent *m_note_on;
    OrganMidiEvent* m_note_off;
    SyndineKeyboards m_keyboard;

    std::list<OrganMidiEvent> m_event_list;
};
