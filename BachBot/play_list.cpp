/**
 * @file syndyne_importer.cpp
 * @brief MIDI -> Syndyne Sequence conversion.
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
#include <stdexcept>  //  std::logic_error
#include <fmt/format.h>  //  fmt::format

//  module includes
// -none-

//  local includes
#include "play_list.h"  //  local include


namespace bach_bot {
PlayListMutex::PlayListMutex(std::shared_ptr<wxMutex> mutex, bool *const lock_flag) :
    m_mutex(mutex),
    m_lock_flag{lock_flag}
{
    m_mutex->Lock();
    if (*m_lock_flag) {
        throw std::logic_error("Attempting double lock!");
    }

    *m_lock_flag = true;
}


PlayListMutex::~PlayListMutex()
{
    *m_lock_flag = false;
    m_mutex->Unlock();
}


PlayList::PlayList() :
    m_play_list(),
    m_next_song_id{1U},
    m_mutex(new wxMutex(wxMUTEX_DEFAULT)),
    m_is_locked{false}
{
}


PlayListMutex PlayList::lock()
{
    return PlayListMutex(m_mutex, &m_is_locked);
}


std::list<OrganNote> PlayList::get_song_events(const uint32_t song_id) const
{
    PlayListEntry song;
    wxMutexLocker lock(*m_mutex);
    const auto entry = m_play_list.find(song_id);
    if (m_play_list.end() != entry) {
        song = entry->second;
    }

    return std::move(song.midi_events);
}


PlayListEntry &PlayList::get_playlist_entry(const uint32_t song_id)
{
    if (!m_is_locked) {
        throw std::runtime_error("Error: lock not aquired");
    }

    auto entry = m_play_list.find(song_id);
    if (m_play_list.end() != entry) {
        return entry->second;
    }

    if (song_id > 0U) {
        throw std::runtime_error(fmt::format("Error: song id {} not found", 
                                 song_id));
    }

    const auto new_song_id = m_next_song_id;
    ++m_next_song_id;
    auto &song = m_play_list[new_song_id];
    song.current_song_id = new_song_id;
    return song;
}


void PlayList::clear()
{
    wxMutexLocker lock(*m_mutex);
    m_next_song_id = 1U;
    m_play_list.clear();
}



}
