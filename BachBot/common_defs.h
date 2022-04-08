#pragma once


#include <cstdint>  //  uintXX_t
#include <cstdlib>  //  size_t
#include <array>  //  std::array


enum SyndyneBankCommands : uint8_t
{
    GENERAL_CANCEL = 0U,
    PREV_BANK = 1U,
    NEXT_BANK = 2U
};

enum SyndineKeyboards : size_t
{
    MANUAL1_SWELL = 2U,
    MANUAL2_GREAT = 1U,
    PETAL = 3U
};
constexpr const size_t NUM_SYNDINE_KEYBOARDS = 3U;
constexpr const uint8_t SYNDINE_CONTROLLER_ID = 16U;


constexpr const size_t MIDI_MESSAGE_SIZE = 3U;


template <typename T>
using SyndineMidiEventTable = std::array<std::array<T, 127U>, 
                                         NUM_SYNDINE_KEYBOARDS>;


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


constexpr uint8_t make_midi_command_byte(const uint8_t chan, 
                                         const MidiCommands command)
{
    return (command << 4U) | (chan & 0x0FU);
}

/* Midi timing magic constants */
constexpr const auto MINIMUM_NOTE_GAP_S = 0.02;
constexpr const auto MINIMUM_NOTE_LENGTH_S = 0.08;
constexpr const auto MINIMUM_BANK_CHANGE_INTERVAL_S = 0.1;
