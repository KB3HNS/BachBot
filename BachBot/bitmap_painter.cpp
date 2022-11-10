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
 */

//  system includes

//  local includes
#include "bitmap_painter.h"  //  local include


namespace bach_bot {
namespace ui {

BitmapPainter::BitmapPainter(const wxString &filename) :
    wxEvtHandler(),
    m_bitmap()
{
    const auto loaded = load_image(m_bitmap, wxBITMAP_TYPE_PNG, filename);
    assert(loaded);
}


bool BitmapPainter::ProcessEvent(wxEvent &e)
{
#ifndef __linux__
    if (e.GetEventType() == wxEVT_ERASE_BACKGROUND) {
        auto &erase_event = dynamic_cast<wxEraseEvent&>(e);
        auto *const dc = erase_event.GetDC();
        wxMemoryDC mdc(m_bitmap);
        dc->StretchBlit(wxPoint(0, 0), dc->GetSize(),
                        &mdc, wxPoint(0, 0), mdc.GetSize(), 
                        wxCOPY, true);

        return true;
    } else {
#endif // __linux__
        return wxEvtHandler::ProcessEvent(e);
#ifndef __linux__
    }
#endif // __linux__
}

}  //  end ui
}  //  end bach_bot
