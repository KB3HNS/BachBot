/**
 * @file label_animator.h
 * @brief Label animation
 * @copyright
 * 2022 Andrew Buettner (ABi)
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
 * In many cases, a label containing the song name may be too long for the
 * label to be shown on-screen.  In these cases, this acts as an animation
 * to rotate the text shown in the label like a news scroll at the bottom of
 * the screen during a news broadcast.  The label starts showing the beginning
 * of the text for 10 ticks and then advances 1 character during each tick and 
 * lastly, stops for 10 ticks once the end of the text is reached; then starts
 * again.  Suggested tick time is 100ms
 */

#pragma once

 //  system includes
#include <cstdint>  //  uint32_t
#include <wx/wx.h>  //  wxStaticText, etc

 //  module includes
 // -none-

 //  local includes
 // -none-

namespace bach_bot {
namespace ui {


class LabelAnimator
{
public:
    
    /** Recommmended tick interval */
    static constexpr const auto RECOMMENDED_TICK_MS = 100;

    LabelAnimator(wxStaticText *const label, const uint32_t max_len);

    void set_label_text(const wxString &text);

    void animate_tick();

private:
    wxStaticText *const m_label;
    wxString m_label_text;
    const uint32_t m_max_len;
    int m_state;
};

}  //  end ui
}  // end bach_bot
