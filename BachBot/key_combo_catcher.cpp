/**
 * @file key_combo_catcher.cpp
 * @brief Utility to simplify key combinations.
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
#include <fmt/xchar.h>  //  fmt::format(L

//  module includes
// -none-

//  local includes
#include "key_combo_catcher.h"

namespace bach_bot {


KeyComboCatcher::KeyComboCatcher(const std::vector<int> combination, CallBack callback) :
    m_current_state(),
    m_match_test{combination},
    m_callback{callback}
{
}


bool KeyComboCatcher::on_key_event(wxKeyEvent &event, const bool key_down)
{
    auto consumed = false;
    const auto key_code = event.GetKeyCode();
    if (key_down) {
        m_current_state.insert(key_code);
        size_t keys_matched = 0U;
        for (const auto i : m_match_test) {
            if (m_current_state.find(i) != m_current_state.end()) {
                ++keys_matched;
            }
        }

        if (m_match_test.size() == keys_matched) {
            consumed = true;
            m_callback(event);
            m_current_state.clear();
        }
    } else {
        m_current_state.erase(key_code);
    }

    return consumed;
}


MidiFileDrop::MidiFileDrop(wxFrame *const parent) :
    m_parent{parent}
{

}


bool MidiFileDrop::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString &filenames)
{
    wxMessageBox(fmt::format(L"PlayerWindow::drop event {} count: {}",
                             filenames.front(), filenames.size()),
                 wxT("Debug"),
                 wxOK | wxICON_INFORMATION);
    return true;
}

}  //  end bach_bot
