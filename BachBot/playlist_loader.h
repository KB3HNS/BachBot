/**
 * @file playlist_loader.h
 * @brief Playlist loading
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
 * Because playlists can cause the loading of several "large" songs, this
 * process can potentially take several seconds on an under-powered machine.
 * Rather than bottling up the UI, do most of the loading in a separate thread
 * and force a top-level modal dialog box during the process.
 */

#pragma once

 //  system includes
#include <wx/xml/xml.h>  //  wxXml API
#include <wx/wx.h>  //  wxThread

 //  module includes
 // -none-

 //  local includes
#include "play_list.h"  //  PlayList
#include "main_window.h"  //  LoadingPopup


namespace bach_bot {
namespace ui {


class PlaylistLoader : public LoadingPopup, wxThread
{
public:
    PlaylistLoader(wxFrame *const parent, 
                   PlayList &playlist,
                   const wxXmlNode *const p_playlist);

    int ShowModal() override;

protected:
    virtual ExitCode Entry() override;

private:
    enum LoaderEvents : int
    {
        TICK_EVENT = 1001,
        EXIT_EVENT
    };

    void on_tick_event(wxThreadEvent &event);
    void on_close_event(wxThreadEvent &event);

    wxMutex m_mutex;
    PlayList &m_playlist;
    wxString m_error_text;
    const wxXmlNode *const m_playlist_root;

    wxDECLARE_EVENT_TABLE();
};

}  //  end ui
}  // end bach_bot
