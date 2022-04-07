#pragma once


#include <cstdint>  //  uintXX_t
#include <cstdlib>  //  size_t
#include <optional>  //  std::optional
#include <wx/wx.h>  //  wxLongLong


enum SyndyneBankCommands : uint8_t
{
    GENERAL_CANCEL = 0U,
    PREV_BANK = 1U,
    NEXT_BANK = 2U
};

constexpr const uint8_t SYNDINE_CONTROLLER_ID = 16U;


constexpr const size_t MIDI_MESSAGE_SIZE = 3U;


enum MidiCommands : uint8_t
{
    NOTE_OFF = 0x8U,
    NOTE_ON = 0x9U,
    AFTERTOUCH = 0xAU,
    CONTROL_CHANGE = 0xBU,
    PATCH_CHANGE = 0xCU,
    CHANNEL_PRESSURE = 0xDU,
    PITCH_BLEND = 0xEU,
    SPECIAL = 0xFU
};


struct OrganMidiEvent
{
    uint8_t event_code;
    double seconds;
    double delta_time;
    std::optional<uint8_t> byte1;
    std::optional<uint8_t> byte2;
    uint8_t desired_bank_number;
    uint32_t desired_mode_number;
    int delay;
    int delta;

    wxLongLong get_us() const
    {
        const auto ms_time = seconds * 1000000.0;
        return wxLongLong(ms_time + 0.5);
    }

    constexpr bool operator< (const OrganMidiEvent& rhs) const
    {
        return (seconds < rhs.seconds);
    }
};


constexpr uint8_t make_midi_command_byte(const uint8_t chan, 
                                         const MidiCommands command)
{
    return (command << 4U) | (chan & 0x0FU);
}

/* Midi timing magic constants */
constexpr const auto MINIMUM_NOTE_GAP_S = 0.05;
constexpr const auto MINIMUM_BANK_CHANGE_INTERVAL_S = 0.1;
