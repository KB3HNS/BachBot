/**
 * @file playlist_parser.cpp
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
 */


#include "playlist_parser.h"

namespace bach_bot {


PlaylistParser::PlaylistParser() :
    m_playlist_doc(), 
    m_playlist_root{nullptr},
    m_filename()
{
}


void PlaylistParser::load(const wxString &filename)
{
    if (!m_filename.empty()) {
        throw PlaylistParseError(wxT("Instance previously initialzied"));
    }

    m_filename = filename;
    if (!m_playlist_doc.Load(filename)) {
        throw PlaylistParseError(wxT("Invalid file format"));
    }

    m_playlist_root = m_playlist_doc.GetRoot();
    if (m_playlist_root->GetName() != L"BachBot_Playlist") {
        throw PlaylistParseError(wxT("File format not recognized."));
    }
}


std::vector<PlaylistParser::SongNode> PlaylistParser::get_children() const
{
    if (nullptr == m_playlist_root) {
        throw PlaylistParseError(wxT("No file loaded."));
    }

    std::vector<std::pair<uint32_t, SongNode>> children;
    const auto *child = m_playlist_root->GetChildren();
    while (nullptr != child) {
        if (child->GetName() == L"song" &&
            child->GetType() == wxXML_ELEMENT_NODE)
        {
            const auto &order_text = child->GetAttribute(wxT("order"));
            auto order = std::numeric_limits<long>::min();
            const auto ok = order_text.ToCLong(&order);
            if (!ok || (order < 1)) {
                throw PlaylistParseError(
                    wxString::Format(wxT("Invalid song data in %s:%i"),
                                     m_filename,
                                     child->GetLineNumber()));
            }

            children.push_back({uint32_t(order), child});
        }

        child = child->GetNext();
    }

    std::sort(
        children.begin(),
        children.end(),
        [](const auto &a, const auto &b) {
            return a.first < b.first;
        });

    std::vector<SongNode> result;
    result.reserve(children.size());
    for (auto i : children) {
        result.push_back(i.second);
    }

    return result;
}


PlaylistParseError::PlaylistParseError(const wxString &error_text) :
    m_what{error_text}
{
}


const char *PlaylistParseError::what() const
{
    return m_what.c_str().AsChar();
}

}  //  bach_bot
