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
#include <list>  //  std::list
#include <variant>  //  std::variant
#include <vector>  //  std::vector
#include <wx/config.h>  //  wxConfig API
#include <wx/wx.h>  //  wxThread, etc

 //  module includes
 // -none-

 //  local includes
#include "play_list.h"  //  PlayList, PlayListEntry
#include "playlist_parser.h"  //  PlaylistParser
#include "thread_loader.h"  //  ThreadLoader


namespace bach_bot {
namespace ui {

/**
 * @brief Load playlist file.
 */
class PlaylistXmlLoader : public ThreadLoader
{
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
    PlaylistParser m_parser;  ///<  Parser storage
    const wxString m_filename;  ///<  Filename
    std::vector<PlaylistParser::SongNode> m_entries;  ///<  Pasrsed entries
};

/**
 * @brief Playlist loading from Drag'N'Drop event
 */
class PlaylistDndLoader : public ThreadLoader
{
    /**
     * @brief Simplify playlist entries:  Song node and filename index
     */
    using PlaylistEntry = std::pair<PlaylistParser::SongNode, size_t>;
    
    /**
     * @brief Simplify the entry type
     * @note Either the filename (load a MIDI file directly), or 
     */
    using SongEntry = std::variant<wxString, PlaylistEntry>;

    /**
     * @brief Callable object for handling Song entries
     */
    class PlaylistAccess
    {
    public:
        /**
         * @brief Constructor
         * @param parent parent object
         * @param entry Song entry (output)
         * @param song_number song number to load (overrides parsed value)
         */
        PlaylistAccess(PlaylistDndLoader &parent,
                       PlayListEntry &entry,
                       const uint32_t song_number);

        /**
         * @brief Visitor access for standard midi files
         * @param entry filename
         */
        void operator()(const wxString &entry);

        /**
         * @brief Visitor access for playlist entries
         * @param entry Playlist entry data
         */
        void operator()(const PlaylistEntry &entry);

    private:
        PlaylistDndLoader &m_parent;  ///<  Parent
        PlayListEntry &m_entry;  ///<  Entry output
        const uint32_t m_song_number;  ///< Song number to assign to this entry
        const wxConfigBase *const m_config;  ///<  Global configuration
    };

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
    std::list<PlaylistParser> m_parsers;
    std::vector<wxString> m_files_dropped;
    std::vector<SongEntry> m_files_to_load;
    const uint32_t m_first_song_id;  ///<  Starting song ID (offset)
};

}  //  end ui
}  // end bach_bot
