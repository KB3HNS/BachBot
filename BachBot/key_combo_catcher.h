/**
 * @file key_combo_catcher.h
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
 *
 * @section DESCRIPTION
 * There doesn't appear to be a simple utility to trigger an event when a
 * certain key combination occurs out-of-the-box in wxWidgets.  This is a
 * simple class that monitors all key-up and key-down events for the 
 * combination that it's looking for and then raises an event when it occurs.
 * The event is guaranteed to only fire once for each unique key-press.
 */

#pragma once

//  system includes
#include <functional>  //  std::function
#include <vector>  //  std::vector
#include <unordered_set>  //  std::unordered_set
#include <wx/wx.h>  //  wxKeyEvent
#include <wx/dnd.h>

//  module includes
// -none-

//  local includes
// -none-


namespace bach_bot {

class KeyComboCatcher
{
    using CallBack = std::function<void(wxKeyEvent&)>;
public:
    KeyComboCatcher(const std::vector<int> combination, CallBack callback);

    bool on_key_event(wxKeyEvent &event, const bool key_down);

private:
    std::unordered_set<int> m_current_state;
    const std::vector<int> m_match_test;
    CallBack m_callback;
};


class MidiFileDrop : public wxFileDropTarget
{
public:
    MidiFileDrop(wxFrame *const parent);

    virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString &filenames) override;

private:
    wxFrame *const m_parent;
};


}  //  end bach_bot
