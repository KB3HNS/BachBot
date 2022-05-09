/**
 * @file play_list.h
 * @brief Playlist storage container
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
 * Playlist management structures and loading.
 */

#pragma once

//  system includes
#include <cstdint>  //  uint32_t
#include <list>  //  std::list
#include <memory>  //  std::shared_ptr, std::unique_ptr
#include <unordered_map>  //  std::unordered_map
#include <optional>  //  std::optional
#include <atomic>  //  std::atomic_bool
#include <wx/wx.h>  //  wxString

//  module includes
// -none-

//  local includes
#include "organ_midi_event.h"  //  OrganNote


namespace bach_bot {

/**
 * @brief Single song entry in a playlist
 */
struct PlayListEntry
{
    uint32_t current_song_id;
    uint32_t next_song_id;

    //  Configuration items
    wxString file_name;
    std::optional<int> tempo_detected;
    int tempo_requested;
    double gap_beats;
    BankConfig starting_config;
    int delta_pitch;
    double last_note_multiplier;
    bool play_next;

    //  Actual song data
    std::list<OrganNote> midi_events;
};


class PlayList;  //  Forward declare the type


/**
 * @brief Lock semantic for the playlist
 */
class PlayListMutex
{
public:
    PlayListMutex(const PlayListMutex&) = delete;

    PlayListMutex(std::shared_ptr<wxMutex> mutex,
                  std::atomic_bool *const lock_flag);

    ~PlayListMutex();

private:
    std::shared_ptr<wxMutex> m_mutex;
    std::atomic_bool *const m_lock_flag;
};


/**
 * @brief Actual playlist container - container is thread-safe.
 */
class PlayList
{
public:
    
    /**
     * @brief Default constructor
     */
    PlayList();

    /**
     * @brief Aquire external persistent lock
     * @return Lock object
     */
    std::unique_ptr<PlayListMutex> lock();
    
    /**
     * @brief Get an existing song
     * @param song_id sing ID to get
     * @return Playlist song's events
     */
    std::list<OrganMidiEvent> get_song_events(const uint32_t song_id) const;

    /**
     * @brief Get a mutable song entry
     * @param song_id song ID to get (default: new song entry)
     * @return Playlist entry
     * @throws std::runtime_error if song_id is specified and does not exist.
     */
    PlayListEntry& get_playlist_entry(const uint32_t song_id=0U);

    /**
     * @brief Clear all entries in the playlist and reset state.
     */
    void clear();

    /**
     * @brief Get the number of items in the playlist.
     * @return number of items in playlist
     */
    size_t count() const
    {
        return m_play_list.size();
    }

private:
    std::unordered_map<uint32_t, PlayListEntry> m_play_list;
    uint32_t m_next_song_id;
    std::shared_ptr<wxMutex> m_mutex;  ///< Access is protected by mutex
    std::atomic_bool m_is_locked;
};


}  //  end bach_bot
