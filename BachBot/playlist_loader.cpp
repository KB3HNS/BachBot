/**
 * @file playlist_loader.cpp
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
 */

//  system includes
#include <cstdint>  //  uint32_t
#include <limits>  //  std::numeric_limits
#include <fmt/xchar.h>  //  fmt::format(L

//  module includes
// -none-

//  local includes
#include "playlist_loader.h"  //  local include


namespace bach_bot {
namespace ui {


PlaylistXmlLoader::PlaylistXmlLoader(wxFrame *const parent,
                                     const wxString &filename) :
    ThreadLoader(parent),
    m_playlist_doc(),
    m_filename{filename},
    m_entries()
{
}


int PlaylistXmlLoader::count_children()
{
    auto count = -1;
    if (!m_playlist_doc.Load(m_filename)) {
        set_error_text(wxT("Invalid file format"));
        return count;
    }

    const auto playlist_root = m_playlist_doc.GetRoot();
    if (playlist_root->GetName() != L"BachBot_Playlist") {
        set_error_text(wxT("File format not recognized."));
    } else {
        count = _count_children(playlist_root);
    }

    return count;
}


void PlaylistXmlLoader::build_playlist_entry(PlayListEntry &song_entry,
                                             const uint32_t song_number)
{
    const auto *const child = m_entries[song_number - 1U].second;
    if (!song_entry.load_config(child)) {
        set_error_text(fmt::format(L"Invalid song data line {}", 
                                   child->GetLineNumber()));
    }
}


int PlaylistXmlLoader::_count_children(const wxXmlNode *const playlist_root)
{
    const auto *child = playlist_root->GetChildren();
    while (nullptr != child) {
        if (child->GetName() == L"song" && 
                child->GetType() == wxXML_ELEMENT_NODE)
        {
            const auto &order_text = child->GetAttribute(wxT("order"));
            auto order = std::numeric_limits<long>::min();
            const auto ok = order_text.ToCLong(&order);
            if (!ok || (order < 1)) {
                return -1;
            }

            m_entries.push_back({uint32_t(order), child});
        }
        child = child->GetNext();
    }

    std::sort(m_entries.begin(), m_entries.end(), [](const SongNode &a,
                                                     const SongNode &b) {
        return a.first < b.first;
    });
    return int(m_entries.size());
}


PlaylistDndLoader::PlaylistDndLoader(wxFrame *const parent,
                                     const wxDropFilesEvent &event,
                                     const uint32_t first_song_id) :
    ThreadLoader(parent),
    m_files(size_t(event.GetNumberOfFiles())),
    m_first_song_id{first_song_id}
{
    auto p_filename = event.GetFiles();
    for (auto i = 0; i < event.GetNumberOfFiles(); ++i) {
        m_files[size_t(i)] = *p_filename;
        ++p_filename;
    }
}

int PlaylistDndLoader::count_children()
{
    return int(m_files.size());
}


void PlaylistDndLoader::build_playlist_entry(PlayListEntry &song_entry,
                                             const uint32_t song_number)
{
    const auto song_index = song_number - 1U;
    song_entry.file_name = m_files[song_index];
    song_entry.song_id = song_index + m_first_song_id;
    song_entry.tempo_requested = -1;
    song_entry.gap_beats = 0.0;
    song_entry.starting_config = BankConfig();
    song_entry.delta_pitch = 0;
    song_entry.last_note_multiplier = 1.0;
    song_entry.play_next = false;
}


}  //  end ui
}  // end bach_bot
