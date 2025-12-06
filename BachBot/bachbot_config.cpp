/**
 * @file bachbot_config.h
 * @brief Persistent system configuration
 * @copyright
 * 2025 Andrew Buettner (ABi)
 *
 * @section LICENSE
 *
 * BachBot - A hymn Midi player for Schlicker organs
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//  system includes
#include <array>
#include <string_view>
#include <vector>

//  module includes
// -none-

//  local includes
#include "bachbot_config.h"  //  local include

namespace {
using namespace std::literals::string_view_literals;
const std::array s_midi_instruments{
    "Acoustic Grand Piano"sv,
    "Bright Acoustic Piano"sv,
    "Electric Grand Piano"sv,
    "Honky-tonk Piano"sv,
    "Rhodes Piano"sv,
    "Chorused Piano"sv,
    "Harpsichord"sv,
    "Clavinet"sv,
    //  Chromatic Percussion:
    "Celesta"sv,
    "Glockenspiel"sv,
    "Music Box"sv,
    "Vibraphone"sv,
    "Marimba"sv,
    "Xylophone"sv,
    "Tubular Bells"sv,
    "Dulcimer"sv,
    //  Organ Timbres:
    "Hammond Organ"sv,
    "Percussive Organ"sv,
    "Rock Organ"sv,
    "Church Organ"sv,
    "Reed Organ"sv,
    "Accordion"sv,
    "Harmonica"sv,
    "Tango Accordion"sv,
    //  Guitar Timbres:
    "Acoustic Nylon Guitar"sv,
    "Acoustic Steel Guitar"sv,
    "Electric Jazz Guitar"sv,
    "Electric Clean Guitar"sv,
    "Electric Muted Guitar"sv,
    "Overdriven Guitar"sv,
    "Distortion Guitar"sv,
    "Guitar Harmonics"sv,
    //  Bass Timbres:
    "Acoustic Bass"sv,
    "Fingered Electric Bass"sv,
    "Plucked Electric Bass"sv,
    "Fretless Bass"sv,
    "Slap Bass 1"sv,
    "Slap Bass 2"sv,
    "Synth Bass 1"sv,
    "Synth Bass 2"sv,
    //  String Timbres:
    "Violin"sv,
    "Viola"sv,
    "Cello"sv,
    "Contrabass"sv,
    "Tremolo Strings"sv,
    "Pizzicato Strings"sv,
    "Orchestral Harp"sv,
    "Timpani"sv,
    //  Ensemble Timbres:
    "String Ensemble 1"sv,
    "String Ensemble 2"sv,
    "Synth Strings 1"sv,
    "Synth Strings 2"sv,
    "Choir 'Aah'"sv,
    "Choir 'Ooh'"sv,
    "Synth Voice"sv,
    "Orchestral Hit"sv,
    //  Brass Timbres:
    "Trumpet"sv,
    "Trombone"sv,
    "Tuba"sv,
    "Muted Trumpet"sv,
    "French Horn"sv,
    "Brass Section"sv,
    "Synth Brass 1"sv,
    "Synth Brass 2"sv,
    //  Reed Timbres:
    "Soprano Sax"sv,
    "Alto Sax"sv,
    "Tenor Sax"sv,
    "Baritone Sax"sv,
    "Oboe"sv,
    "English Horn"sv,
    "Bassoon"sv,
    "Clarinet"sv,
    //  Pipe Timbres:
    "Piccolo"sv,
    "Flute"sv,
    "Recorder"sv,
    "Pan Flute"sv,
    "Bottle Blow"sv,
    "Shakuhachi"sv,
    "Whistle"sv,
    "Ocarina"sv,
    //  Synth Lead:
    "Square Wave Lead"sv,
    "Sawtooth Wave Lead"sv,
    "Calliope Lead"sv,
    "Chiff Lead"sv,
    "Charang Lead"sv,
    "Voice Lead"sv,
    "Fifths Lead"sv,
    "Bass Lead"sv,
    //  Synth Pad:
    "New Age Pad"sv,
    "Warm Pad"sv,
    "Polysynth Pad"sv,
    "Choir Pad"sv,
    "Bowed Pad"sv,
    "Metallic Pad"sv,
    "Halo Pad"sv,
    "Sweep Pad"sv,
    //  Synth Effects:
    "Rain Effect"sv,
    "Soundtrack Effect"sv,
    "Crystal Effect"sv,
    "Atmosphere Effect"sv,
    "Brightness Effect"sv,
    "Goblins Effect"sv,
    "Echoes Effect"sv,
    "Sci-Fi Effect"sv,
    //  Ethnic Timbres:
    "Sitar"sv,
    "Banjo"sv,
    "Shamisen"sv,
    "Koto"sv,
    "Kalimba"sv,
    "Bagpipe"sv,
    "Fiddle"sv,
    "Shanai"sv,
    //  Sound Effects:
    "Tinkle Bell"sv,
    "Agogo"sv,
    "Steel Drums"sv,
    "Woodblock"sv,
    "Taiko Drum"sv,
    "Melodic Tom"sv,
    "Synth Drum"sv,
    "Reverse Cymbal"sv,
    //  Sound Effects:
    "Guitar Fret Noise"sv,
    "Breath Noise"sv,
    "Seashore"sv,
    "Bird Tweet"sv,
    "Telephone Ring"sv,
    "Helicopter"sv,
    "Applause"sv,
    "Gun Shot"sv
};

}

namespace bach_bot {
namespace ui  {

BachbotConfigDialog::BachbotConfigDialog(MainWindow *parent) :
    AppConfigDialog(parent),
    m_config{wxConfig::Get()},
    m_gap{m_config->ReadDouble(L"import/gap", 0.0)},
    m_mul{m_config->ReadDouble(L"import/mul", 1.0)},
    m_gap_validator(precision, &m_gap, wxNUM_VAL_NO_TRAILING_ZEROES),
    m_mul_validator(precision, &m_mul, wxNUM_VAL_NO_TRAILING_ZEROES)
{
    initial_gap_text_box->SetValidator(m_gap_validator);
    extend_ending_textbox->SetValidator(m_mul_validator);

    memory_select->SetValue(int(m_config->ReadLong(L"import/mem", 1)));
    mode_select->SetValue(int(m_config->ReadLong(L"import/mode", 1)));
    pitch_change->SetValue(int(m_config->ReadLong(L"import/pitch", 0)));
    play_next_checkbox->SetValue(m_config->ReadBool(L"import/next", false));

    path_find->SetValue(m_config->Read(L"path/find", L""));
    path_replace->SetValue(m_config->Read(L"path/replace", L""));

    std::vector<wxString> instruments;
    instruments.reserve(s_midi_instruments.size());
    for (auto i : s_midi_instruments) {
        instruments.push_back(_(i.data()));
    }

    m_default_voice->Set(instruments);
    m_default_voice->SetSelection(
        int(m_config->ReadLong(L"voice/default_voice", 0)));

#ifdef __linux__
    auto size = GetSize();
    auto height = double(size.y) * 1.5;
    auto width = double(size.x) * 1.25;
    size.y = int(height);
    size.x = int(width);
    SetSize(size);
#endif
}


void BachbotConfigDialog::run()
{
    auto result = ShowModal();
    if (wxID_OK == result) {
        m_config->Write(L"import/gap", m_gap);
        m_config->Write(L"import/mul", m_mul);
        m_config->Write(L"import/mem", long(memory_select->GetValue()));
        m_config->Write(L"import/mode", long(mode_select->GetValue()));
        m_config->Write(L"import/pitch", long(pitch_change->GetValue()));
        m_config->Write(L"import/next", play_next_checkbox->GetValue());

        m_config->Write(L"path/find", path_find->GetValue());
        m_config->Write(L"path/replace", path_replace->GetValue());

        m_config->Write(L"voice/default_voice",
                        m_default_voice->GetSelection());
    }
}

}  // ui
}  //  bach_bot
