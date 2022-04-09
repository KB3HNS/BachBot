
#include <iostream>  //  Delete me
#include <cmath>  //  abs

#include "midi_note_tracker.h"


MidiNoteTracker::MidiNoteTracker() :
    m_on_now{false},
    m_last_event_was_on{false},
    m_midi_ticks_on_time{-1},
    m_last_midi_off_time{-1},
    m_note_nesting_count{0U},
    m_note_on(),
    m_note_off(),
    m_keyboard{SyndineKeyboards::MANUAL1_SWELL},
    m_event_list()
{
}


void MidiNoteTracker::add_event(const smf::MidiEvent &ev)
{
    auto organ_event = std::make_shared<OrganMidiEvent>(ev, m_keyboard);
    //if (ev[1] == 0x34) {
    //    organ_event->m_delta_time = 0.0;
    //}

    if (ev.isNoteOn() && !m_on_now) {
        process_new_note_on_event(organ_event);
    } else if (ev.isNoteOff() && m_last_event_was_on){
        process_new_note_off_event(organ_event);
    } else if (ev.isNoteOn() && is_same_time(ev, m_midi_ticks_on_time)) {
        ++m_note_nesting_count;
    } else if (ev.isNoteOff() && 
               is_same_time(ev, m_last_midi_off_time) && 
               (m_note_nesting_count > 0U)) {
        --m_note_nesting_count;
    } else if (ev.isNoteOn() && m_on_now) {
        insert_off_event(organ_event);
        process_new_note_on_event(organ_event);
    } else if (ev.isNoteOff() && !m_on_now && (m_note_nesting_count > 0U)) {
        backfill_on_event(organ_event);
        process_new_note_off_event(organ_event);
    }

    m_last_event_was_on = ev.isNoteOn();
}


void MidiNoteTracker::append_events(std::list<OrganMidiEvent> &event_list) const
{
    //if (m_event_list.size() > 0U) {
    //    std::cout << "\n";
   // }

    for (const auto &i: m_event_list) {
        if (i.second->m_seconds - i.first->m_seconds > MINIMUM_NOTE_LENGTH_S) {
            event_list.push_back(*(i.first));
            event_list.push_back(*(i.second));
        }
    }
}


void MidiNoteTracker::set_keyboard(const SyndineKeyboards keyboard_id)
{
    m_keyboard = keyboard_id;
}


bool MidiNoteTracker::is_same_time(const smf::MidiEvent &ev,
                                   const int organ_time) const
{
    return (abs(ev.tick - organ_time) <= 1);
}


void MidiNoteTracker::process_new_note_on_event(OrganNote &organ_ev)
{
    auto last_off_time = -1.0;
    if (nullptr != m_note_off.get()) {
        last_off_time = m_note_off->m_seconds;
    }

    if (organ_ev->m_seconds - last_off_time < MINIMUM_NOTE_GAP_S) {
        const auto delta = MINIMUM_NOTE_GAP_S / 2.0;
        m_note_off->m_seconds -= delta;
        organ_ev->m_seconds += delta;
    }

    m_note_on = organ_ev;
    m_midi_ticks_on_time = organ_ev->m_midi_time;
    m_on_now = true;
    ++m_note_nesting_count;
    organ_ev->m_byte2 = SYNDINE_NOTE_ON_VELOCITY;
}


void MidiNoteTracker::process_new_note_off_event(OrganNote &organ_ev)
{
    m_note_off = organ_ev;
    --m_note_nesting_count;
    m_on_now = false;
    m_last_midi_off_time = organ_ev->m_midi_time;
    organ_ev->m_byte2 = 0U;
    m_event_list.push_back({m_note_on, m_note_off});
}


void MidiNoteTracker::insert_off_event(OrganNote &organ_ev)
{
    auto organ_event = std::make_shared<OrganMidiEvent>(*organ_ev);
    organ_event->m_event_code = make_midi_command_byte(m_keyboard, 
                                                       MidiCommands::NOTE_OFF);
    process_new_note_off_event(organ_event);
    ++m_note_nesting_count;
}


void MidiNoteTracker::backfill_on_event(OrganNote &organ_ev)
{
    auto organ_event = std::make_shared<OrganMidiEvent>(*m_note_off);
    organ_event->m_event_code = make_midi_command_byte(m_keyboard, 
                                                       MidiCommands::NOTE_ON);
    process_new_note_on_event(organ_event);
    --m_note_nesting_count;
}