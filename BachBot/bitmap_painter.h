/**
 * @file bitmap_painter.h
 * @brief Utility class to scale and draw an image (jpg format) onto a panel
 *        background.
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
 * This is an event handler that can catch window erase events and draw a jpeg
 * image on the background of a window / panel / something else derived from
 * them.
 * Credit to [this post in the WxWidgets forum][1]
 * [1]: <https://forums.wxwidgets.org/viewtopic.php?t=45099>
 */

#pragma once

//  system includes
#include <wx/wx.h>  //  wxEvtHandler, wxBitmap, etc

//  module includes
// -none-

//  local includes
// -none-

namespace bach_bot {
namespace ui {

/**
 * @brief Utility class for drawing panel background in events.
 * @note  To use this, it must be appended to the event handler as follows:
 * @code{.cpp}
 *     auto background = new BitmapPainter(wxT("test.jpg"));
 *     PushEventHandler(background);
 * @endcode
 * The file is assumed to be in the same directory as the executable.
 */
class BitmapPainter : public wxEvtHandler
{
public:
    BitmapPainter(const wxString &filename);

    virtual bool ProcessEvent(wxEvent &e) override;

private:
    wxBitmap m_bitmap;
};

}  //  end ui
}  //  end bach_bot
