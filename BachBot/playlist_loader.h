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
#include <vector>  //  std::vector
#include <utility>  //  std::pair
#include <wx/xml/xml.h>  //  wxXml API
#include <wx/wx.h>  //  wxThread, etc

 //  module includes
 // -none-

 //  local includes
#include "play_list.h"  //  PlayList, PlayListEntry
#include "main_window.h"  //  LoadingPopup


namespace bach_bot {
namespace ui {

/**
 * @brief Loaylist loading in another thread logic.
 */
class PlaylistLoader : public LoadingPopup, wxThread
{
    using SongNode = std::pair<uint32_t, const wxXmlNode*>;
public:
    /**
     * @brief Constructor
     * @param parent Parent window (main window)
     * @param playlist reference to playlist container
     * @param filename file to load
     */
    PlaylistLoader(wxFrame *const parent,
                   PlayList &playlist,
                   const wxString &filename);

    int ShowModal() override;

    /**
     * @brief Get error text
     * @return error text (valid only if ShowModal doesn't return wxID_OK)
     */
    const wxString& get_error_text() const;

    /**
     * @brief Get error text
     * @return error text (valid only if ShowModal doesn't return wxID_OK)
     */
    wxString get_error_text();

protected:
    virtual ExitCode Entry() override;

private:
    enum LoaderEvents : int
    {
        START_EVENT = 1001,
        TICK_EVENT,
        EXIT_EVENT
    };

    /**
     * @brief Parse the playlist tree
     * @param playlist_root pointer to root node
     * @throws std::out_of_range on load error
     */
    void parse_playlist(const wxXmlNode *const playlist_root);

    /**
     * @brief Count children (ie songs) in XML tree and sorts them into the
     *        internal vector (sends start event)
     * @param playlist_root root note of playlist
     * @return number of children
     * @retval <= 0 indicates error
     */
    int count_children(const wxXmlNode *const playlist_root);

    /**
     * @brief Load the playlist configuration into the song_entry structure
     * @param song_entry[in/out] update with song entry data
     * @param playlist_node XML node for data
     * @retval `true` data loaded
     * @retval `false` parse error
     */
    bool load_playlist_config(PlayListEntry &song_entry, 
                              const wxXmlNode *const playlist_node) const;
    
    /**
     * @brief Load MIDI file and import events
     * @param song_entry[in/out] update with events
     * @retval `false` song not loaded
     * @retval `true` song loaded successfully
     */
    bool import_midi(PlayListEntry &song_entry) const;

    //  Events in event table
    void on_start_event(wxThreadEvent &event);
    void on_tick_event(wxThreadEvent &event);
    void on_close_event(wxThreadEvent &event);

    wxMutex m_mutex;
    PlayList &m_playlist;
    wxString m_error_text;
    wxXmlDocument m_playlist_doc;
    const wxString m_filename;
    std::vector<SongNode> m_entries;

    wxDECLARE_EVENT_TABLE();
};

}  //  end ui
}  // end bach_bot
