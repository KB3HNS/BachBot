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
 * These are the specializations of the ThreadLoader for specific use-cases.
 */

#pragma once

//  system includes
#include <vector>  //  std::vector
#include <utility>  //  std::pair
#include <wx/xml/xml.h>  //  wxXml API
#include <wx/wx.h>  //  wxThread, etc

 //  module includes
 // -none-

 //  local includes
#include "play_list.h"  //  PlayList, PlayListEntry
#include "thread_loader.h"  //  ThreadLoader


namespace bach_bot {
namespace ui {

/**
 * @brief Load playlist file.
 */
class PlaylistXmlLoader : public ThreadLoader
{
    using SongNode = std::pair<uint32_t, const wxXmlNode*>;
public:
    /**
     * @brief Constructor
     * @param parent Parent window (main window)
     * @param filename file to load
     */
    PlaylistXmlLoader(wxFrame *const parent,
                      const wxString &filename);

protected:
    virtual int count_children() override;
    virtual void build_playlist_entry(PlayListEntry &song_entry,
                                      const uint32_t song_number) override;

private:
    /**
     * @brief Count children (ie songs) in XML tree and sorts them into the
     *        internal vector (sends start event)
     * @param playlist_root root note of playlist
     * @return number of children
     * @retval <= 0 indicates error
     */
    int _count_children(const wxXmlNode *const playlist_root);

    wxXmlDocument m_playlist_doc;
    const wxString m_filename;
    std::vector<SongNode> m_entries;
};

/**
 * @brief Playlist loading from Drag'N'Drop event
 */
class PlaylistDndLoader : public ThreadLoader
{
public:
    /**
     * @brief Constructor
     * @param parent parent window
     * @param event Drag&Drop event
     * @param first_song_id song ID to use for first song
     */
    PlaylistDndLoader(wxFrame *const parent,
                      const wxDropFilesEvent &event,
                      const uint32_t first_song_id);

protected:
    virtual int count_children() override;
    virtual void build_playlist_entry(PlayListEntry &entry,
                                      const uint32_t song_number) override;

private:
    std::vector<wxString> m_files;
    const uint32_t m_first_song_id;
};


}  //  end ui
}  // end bach_bot
