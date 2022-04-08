#include "midi_note_tracker.h"


MidiNoteTracker::MidiNoteTracker() :
    m_on_now{false},
    m_midi_ticks_on_time{-1},
    m_last_midi_off_time{-1},
    m_note_nesting_count{0U},
    m_note_on{nullptr},
    m_note_off{nullptr},
    m_keyboard{MANUAL1_SWELL},
    m_event_list()
{
}


void MidiNoteTracker::add_event(const smf::MidiEvent& ev)
{
    OrganMidiEvent organ_event(ev, m_keyboard);
    //if (ev.isNoteOn() && !m_on_now) {

    //}

    m_event_list.push_back(organ_event);
}


void MidiNoteTracker::append_events(std::list<OrganMidiEvent>& event_list) const
{
    for (const auto& i : m_event_list) {
        event_list.push_back(i);
    }
}


void MidiNoteTracker::set_keyboard(const SyndineKeyboards keyboard_id)
{
    m_keyboard = keyboard_id;
}
