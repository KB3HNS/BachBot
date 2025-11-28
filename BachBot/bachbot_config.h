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
 *
 * @section DESCRIPTION
 * These are the specializations of the ThreadLoader for specific use-cases.
 */

#pragma once

//  system includes
#include <wx/wx.h>  //  Must be first!
#include <wx/config.h>  //  wxConfig API
#include <wx/valnum.h>  //  wxFloatingPointValidator

//  module includes
// -none-

//  local includes
#include "main_window.h"  //  AppConfigDialog

namespace bach_bot {
namespace ui  {

class BachbotConfigDialog : public AppConfigDialog
{
    static constexpr const auto m_precision = 2;
public:
    BachbotConfigDialog(MainWindow *parent);

    void run();

private:
    wxConfigBase *const m_config;
    double m_gap;
    double m_mul;
    wxFloatingPointValidator<double> m_gap_validator;
    wxFloatingPointValidator<double> m_mul_validator;
};

}  // ui
}  //  bach_bot
