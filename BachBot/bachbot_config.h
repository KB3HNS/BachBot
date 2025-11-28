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

/**
 * @brief Configuration dialog with appropriate text validation
 */
class BachbotConfigDialog : public AppConfigDialog
{
    /// @brief Shorten this type
    using Validator = wxFloatingPointValidator<double>;

public:
    /**
    * @brief Fixed precision to use with the validator
    * @note There's a bit of a tradeoff since the text parser enforces _this_
    * many digits after the decimal and truncation.
    */
    static constexpr const auto precision = 2;

    /**
     * @brief Constructor
     * @param parent Main window parent
     */
    BachbotConfigDialog(MainWindow *parent);

    /**
     * @brief Run this dialog as modal.  Update storage back-end if requtested.
     */
    void run();

private:
    wxConfigBase *const m_config;  ///<  Global configuration manager
    double m_gap;  ///<  Intra-song Gap storage back-end
    double m_mul; ///<  Last note extension storage back-end
    Validator m_gap_validator;  ///<  Validator for gap
    Validator m_mul_validator;  ///<  Validator for last note
};

}  // ui
}  //  bach_bot
