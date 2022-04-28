/**
 * @file main.cpp
 * @brief Application entry point.
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
#include <wx/wxprec.h>  //  Pre-compiled header (VS thing?)
#include <wx/wx.h>  //  wxApp

//  module includes
// -none-

//  local includes
#include "player_window.h"  //  Local include


/**
 * @brief Application main class
 */
class BachBot : public wxApp
{
public:
    virtual bool OnInit() override final
    {
        m_window = new bach_bot::ui::PlayerWindow();
        m_window->Show(true);
        return true;
    }

private:
    bach_bot::ui::PlayerWindow *m_window = nullptr;
};


wxIMPLEMENT_APP(BachBot);
