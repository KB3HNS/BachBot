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

LabelAnimator::LabelAnimator(wxStaticText *const label, const int padding) :
    m_label{label},
    m_label_text{label->GetLabelText()},
    m_padding{padding},
    m_state{0U},
    m_pix_per_char()
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
    if (m_label_text.empty()) {
        return;
    }

    if (!m_pix_per_char.has_value()) {
        auto len = m_label_text.Len() - 1U;

        m_label->SetLabel(m_label_text.substr(0U, len));
        auto label_size_new = m_label->GetSize();

        m_label->SetLabel(m_label_text);
        auto label_size_orig = m_label->GetSize();

        m_pix_per_char = double(label_size_orig.x - label_size_new.x);
        return;
    }

    const auto size = m_label->GetContainingSizer()->GetSize();
    const auto pix_per_char = m_pix_per_char.value();
    const auto label_width = double(size.x - m_padding);
    auto max_len = int(label_width / pix_per_char);
    const auto trim_elipsis = "..."sv;
    max_len = std::max(int(trim_elipsis.size() * 2U) + 1,
                       max_len);

    if (m_label_text.size() <= size_t(max_len)) {
        m_label->SetLabelText(m_label_text);
        m_state = 0;
    } else {
        auto start_elipsis = ""sv;
        auto end_elipsis = ""sv;

        if (m_state > 0) {
            start_elipsis = trim_elipsis;
        }

        auto final_ticks = 0U;
        auto start = (m_state < 0 ? 0 : m_state);
        auto end = start + max_len;

        if (end < int(m_label_text.size())) {
            end_elipsis = trim_elipsis;
        } else {
            start = int(m_label_text.size() - 1U) - max_len;
            end = m_label_text.size();
            final_ticks = uint32_t(m_state - start);
        }

        start += int(start_elipsis.size());
        end -= int(end_elipsis.size());

        m_label->SetLabelText(wxString::Format("%s%s%s",
            start_elipsis.data(),
            m_label_text.SubString(size_t(start), size_t(end)),
            end_elipsis.data()));

        if (final_ticks > START_END_TICKS) {
            m_state = -START_END_TICKS;
        } else {
            ++m_state;
        }
    }
}


}  //  end ui
}  // end bach_bot
