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
#include <wx/stdpaths.h>  //  wxStandardPaths::Get
#include <wx/filename.h>  //  wxFileName
//  module includes
// -none-

//  local includes
#include "bitmap_painter.h"  //  local include


namespace bach_bot {
namespace ui {

BitmapPainter::BitmapPainter(const wxString &filename) :
    wxEvtHandler(),
    m_bitmap()
{
    const auto &std = wxStandardPaths::Get();
    wxFileName exe_name(std.GetExecutablePath());
    const auto path = exe_name.GetPath() + exe_name.GetPathSeparator();
    assert(m_bitmap.LoadFile(path + filename, wxBITMAP_TYPE_JPEG));
}


bool BitmapPainter::ProcessEvent(wxEvent &e)
{
    if (e.GetEventType() == wxEVT_ERASE_BACKGROUND) {
        auto &erase_event = dynamic_cast<wxEraseEvent&>(e);
        auto *const dc = erase_event.GetDC();
        wxMemoryDC mdc(m_bitmap);
        dc->StretchBlit(wxPoint(0, 0), dc->GetSize(),
                        &mdc, wxPoint(0, 0), mdc.GetSize(), 
                        wxCOPY,  true);

        // const auto size = m_bitmap.GetSize();
        // const auto dc_size = dc->GetSize();
        // for (auto x = 0; x < dc_size.x; x += size.x) {
        //     for (auto y = 0; y < dc_size.y; y += size.y) {
        //         dc->DrawBitmap(m_bitmap, 0, 0, false);
        //     }
        // }
        return true;
    } else {
        return wxEvtHandler::ProcessEvent(e);
    }
}

}  //  end ui
}  //  end bach_bot