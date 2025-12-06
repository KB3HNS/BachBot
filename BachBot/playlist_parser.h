/**
 * @file playlist_parser.h
 * @brief Playlist parsing
 * @copyright
 * 2025 Andrew Buettner (ABi)
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
 * RAII memory management for playlists.
 */


#pragma once

#include <exception>  //  std::exception
#include <utility>  //  std::pair
#include <vector>  //  std::vector
#include <wx/xml/xml.h>  //  wxXml API

#ifdef WIN32
//  MSW C++ is **WRONG** and **NOT CONFORMANT**
#define NOEXCEPT
#else
#define NOEXCEPT noexcept
#endif


namespace bach_bot {

/**
 * @brief Parse exceptions
 */
class PlaylistParseError : public std::exception
{
public:
    /**
     * @brief Constructor
     * @param error_text **Translated** text of what occurred
    */
    PlaylistParseError(const wxString& error_text);

    const char* what() const NOEXCEPT override;

private:
    wxString m_what;  ///<  Persistent storage
};


/**
 * @brief Memory managed XML playlist parsing and storage.
 * @note Uses exception model (above exception).  Pointers returned by this
 * object's calls have the lifetime of this object.
 */
class PlaylistParser
{
public:
   
   /**
    * @brief Shorten song entry type
    */
   using SongNode = const wxXmlNode*;

    /**
     * @brief Constructor
     */
    PlaylistParser();

    /**
     * @brief Load file
     * @param filename file to load
     * @note can only be called once
     * @throws PlaylistParseError
     */
    void load(const wxString &filename);

    /**
     * @brief Get _parsed_ children
     * @return Children
     * @throws PlaylistParseError
     */
    std::vector<const wxXmlNode*> get_children() const;

private:
    wxXmlDocument m_playlist_doc;  ///<  Document (managed)
    SongNode m_playlist_root;  ///<  Root node
    wxString m_filename;  ///<  Filename loaded

};

}  //  bach_bot