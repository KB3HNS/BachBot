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
// -none-

 //  module includes
 // -none-

 //  local includes
#include "bachbot_config.h"  //  local include

namespace bach_bot {
namespace ui  {

BachbotConfigDialog::BachbotConfigDialog(MainWindow *parent) :
    AppConfigDialog(parent),
    m_config{wxConfig::Get()},
    m_gap{m_config->ReadDouble(L"import/gap", 0.0)},
    m_mul{m_config->ReadDouble(L"import/mul", 1.0)},
    m_gap_validator(m_precision, &m_gap, wxNUM_VAL_NO_TRAILING_ZEROES),
    m_mul_validator(m_precision, &m_mul, wxNUM_VAL_NO_TRAILING_ZEROES)
{
    initial_gap_text_box->SetValidator(m_gap_validator);
    extend_ending_textbox->SetValidator(m_mul_validator);

    memory_select->SetValue(int(m_config->ReadLong(L"import/mem", 1)));
    mode_select->SetValue(int(m_config->ReadLong(L"import/mode", 1)));
    pitch_change->SetValue(int(m_config->ReadLong(L"import/pitch", 0)));
    play_next_checkbox->SetValue(m_config->ReadBool(L"import/next", false));

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
    }
}

}  // ui
}  //  bach_bot
