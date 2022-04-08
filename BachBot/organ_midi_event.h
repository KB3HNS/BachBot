#pragma once

#include <cstdint>
#include <optional>  //  std::optional
#include <utility>  //  std::pair
#include <wx/wx.h>  //  wxLongLong
#include <RtMidi.h>  //  RtMidiOut

#include "MidiEvent.h"  //  smf::MidiEvent

#include "common_defs.h"  //  SyndineKeyboards


using BankConfig = std::pair<uint8_t, uint32_t>;


class OrganMidiEvent
{
public:
    OrganMidiEvent(const smf::MidiEvent& midi_event, 
                   const SyndineKeyboards channel);
    OrganMidiEvent(const smf::MidiEvent &midi_event, const BankConfig& cfg);

    OrganMidiEvent& operator-=(const OrganMidiEvent &rhs);

    void send_event(RtMidiOut &player) const;
    wxLongLong get_us() const;
    void set_bank_config(const BankConfig &cfg);
    BankConfig get_bank_config() const;
    bool is_mode_change_event() const;
    void calculate_delta(const OrganMidiEvent& rhs);

    constexpr bool operator< (const OrganMidiEvent& rhs) const
    {
        return (m_seconds < rhs.m_seconds);
    }

private:
    uint8_t m_event_code;
    bool m_mode_change_event;
    uint8_t m_desired_bank_number;
    uint32_t m_desired_mode_number;
    double m_seconds;
    double m_delta_time;
    std::optional<uint8_t> m_byte1;
    std::optional<uint8_t> m_byte2;
    int m_delay;
    int m_delta;
};
