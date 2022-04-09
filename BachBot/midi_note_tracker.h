#pragma once

#include <list>
#include <memory>
#include <utility>

#include "MidiEvent.h"

#include "common_defs.h"
#include "organ_midi_event.h"


class MidiNoteTracker
{
    using OrganNote = std::shared_ptr<OrganMidiEvent>;
public:
    MidiNoteTracker();

    void add_event(const smf::MidiEvent &ev);

    void append_events(std::list<OrganMidiEvent> &event_list) const;
    void set_keyboard(const SyndineKeyboards keyboard_id);

private:
    void process_new_note_on_event(OrganNote &organ_ev);
    void process_new_note_off_event(OrganNote &organ_ev);
    void insert_off_event(OrganNote &organ_ev);
    void backfill_on_event(OrganNote &organ_ev);

    bool is_same_time(const smf::MidiEvent &ev, 
                      const int organ_time) const;
        
    bool m_on_now;
    bool m_last_event_was_on;
    int m_midi_ticks_on_time;
    int m_last_midi_off_time;
    uint32_t m_note_nesting_count;
    OrganNote m_note_on;
    OrganNote m_note_off;
    SyndineKeyboards m_keyboard;

    std::list<std::pair<OrganNote, OrganNote>> m_event_list;
};
