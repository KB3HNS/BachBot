/**
 * @file label_animator.cpp
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
 */


//  system includes
#include <string_view>  //  sv
#include <fmt/xchar.h>  //  fmt::format(L

//  module includes
// -none-

//  local includes
#include "label_animator.h"  //  local include


namespace {
    constexpr const auto START_END_TICKS = 10;
}

namespace bach_bot {
namespace ui {

using namespace std::literals::string_view_literals;

LabelAnimator::LabelAnimator(wxStaticText *const label, const uint32_t max_len) :
    m_label{label},
    m_label_text{label->GetLabelText()},
    m_max_len{max_len},
    m_state{0U}
{
}


void LabelAnimator::set_label_text(const wxString &text)
{
    if (text != m_label_text) {
        m_state = 0U;
        m_label_text = text;
        animate_tick();
    }
}


void LabelAnimator::animate_tick()
{
    if (m_label_text.size() <= m_max_len) {
        m_label->SetLabelText(m_label_text);
        m_state = 0;
    } else {
        auto start_elipsis = L""sv;
        auto end_elipsis = L""sv;

        if (m_state > 0) {
            start_elipsis = L"..."sv;
        }


        auto final_ticks = 0U;
        auto start = (m_state < 0 ? 0 : m_state);
        auto end = start + int(m_max_len);

        if (end < m_label_text.size()) {
            end_elipsis = L"..."sv;
        } else {
            start = int(m_label_text.size() - m_max_len);
            end = m_label_text.size();
            final_ticks = uint32_t(m_state - start);
        }

        start += int(start_elipsis.size());
        end -= int(end_elipsis.size());

        m_label->SetLabelText(fmt::format(
            L"{}{}{}",
            start_elipsis,
            m_label_text.SubString(size_t(start), size_t(end)),
            end_elipsis));

        if (final_ticks > START_END_TICKS) {
            m_state = -START_END_TICKS;
        } else {
            ++m_state;
        }
    }
}


}  //  end ui
}  // end bach_bot
