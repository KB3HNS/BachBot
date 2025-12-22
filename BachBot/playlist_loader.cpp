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

//  module includes
// -none-

//  local includes
#include "playlist_loader.h"  //  local include

namespace bach_bot {
namespace ui {


PlaylistXmlLoader::PlaylistXmlLoader(wxFrame *const parent,
                                     const wxString &filename) :
    ThreadLoader(parent),
    m_parser(),
    m_filename{filename},
    m_entries()
{
}


int PlaylistXmlLoader::count_children()
{
    try {
        m_parser.load(m_filename);
        m_entries = m_parser.get_children();
    } catch (const PlaylistParseError &e) {
        set_error_text(e.what());
        return -1;
    }

    return int(m_entries.size());
}


void PlaylistXmlLoader::build_playlist_entry(PlayListEntry &song_entry,
                                             const uint32_t song_number)
{
    const auto child = m_entries[song_number - 1U];
    if (!song_entry.load_config(child)) {
        set_error_text(wxString::Format(wxT("Invalid song data line %i"),
                                        child->GetLineNumber()));
    }
}


PlaylistDndLoader::PlaylistDndLoader(wxFrame *const parent,
                                     const wxDropFilesEvent &event,
                                     const uint32_t first_song_id) :
    ThreadLoader(parent),
    m_files_dropped(size_t(event.GetNumberOfFiles())),
    m_first_song_id{first_song_id}
{
    auto p_filename = event.GetFiles();
    for (auto i = 0; i < event.GetNumberOfFiles(); ++i) {
        m_files_dropped[size_t(i)] = *p_filename;
        ++p_filename;
    }
}


int PlaylistDndLoader::count_children()
{
    for (size_t i = 0U; i < m_files_dropped.size(); ++i) {
        const auto &filename = m_files_dropped[i];
        wxFileName file(filename);
        if (file.GetExt() == L"bbp") {
            auto &loader = m_parsers.emplace_back();
            std::vector<PlaylistParser::SongNode> list;
            try {
                loader.load(filename);
                list = loader.get_children();
            } catch (const PlaylistParseError &e) {
                set_error_text(e.what());
                return -1;
            }

            for (auto entry : list) {
                m_files_to_load.push_back(std::pair{entry, i});
            }
        } else {
            m_files_to_load.push_back(filename);
        }
    }

    return int(m_files_to_load.size());
}


void PlaylistDndLoader::build_playlist_entry(PlayListEntry &song_entry,
                                             const uint32_t song_number)
{
    PlaylistAccess accessor(*this, song_entry, song_number + m_first_song_id);
    std::visit(accessor, m_files_to_load[song_number - 1U]);
}


PlaylistDndLoader::PlaylistAccess::PlaylistAccess(PlaylistDndLoader &parent,
                                                  PlayListEntry &entry,
                                                  const uint32_t song_number) :
    m_parent{parent},
    m_entry{entry},
    m_song_number{song_number},
    m_config{wxConfig::Get()}
{
}


void PlaylistDndLoader::PlaylistAccess::operator()(const wxString &entry)
{
    auto mode = m_config->ReadLong(L"import/mode", 1);
    auto mem = m_config->ReadLong(L"import/mem", 1);
    auto pitch = m_config->ReadLong(L"import/pitch", 0);

    m_entry.file_name = entry;
    m_entry.song_id = m_song_number;
    m_entry.tempo_requested = -1;
    m_entry.gap_beats = m_config->ReadDouble(L"import/gap", 0.0);
    m_entry.starting_config = BankConfig(uint32_t(mem), uint8_t(mode));
    m_entry.delta_pitch = int(pitch);
    m_entry.last_note_multiplier = m_config->ReadDouble(L"import/mul", 1.0);
    m_entry.play_next = m_config->ReadBool(L"import/next", false);
}


void PlaylistDndLoader::PlaylistAccess::operator()(const PlaylistEntry &entry)
{
    if (!m_entry.load_config(entry.first)) {
        m_parent.set_error_text(wxString::Format(
            wxT("Invalid song data at %s:%i"),
            m_parent.m_files_dropped[entry.second],
            entry.first->GetLineNumber()));
    } else {
        m_entry.song_id = m_song_number;
    }
}

}  //  end ui
}  // end bach_bot
