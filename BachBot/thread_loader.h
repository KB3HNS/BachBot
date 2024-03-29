/**
 * @file thread_loader.h
 * @brief Background loading in another thread
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
#include <list>  //  std::list
#include <optional>  //  std::optional
#include <functional>  //  std::function
#include <wx/wx.h>  //  wxThread, etc

//  module includes
// -none-

//  local includes
#include "play_list.h"  //  PlayList, PlayListEntry
#include "main_window.h"  //  LoadingPopup


namespace bach_bot {
namespace ui {

/**
 * @brief Playlist loading in another thread logic.
 */
class ThreadLoader : public LoadingPopup, wxThread
{
public:
    using SuccessCallback = std::function<void(std::list<PlayListEntry>)>;
    /**
     * @brief Constructor
     * @param parent Parent window (main window)
     */
    ThreadLoader(wxFrame *const parent);

    int ShowModal() override;

    /**
     * @brief Get error text
     * @return error text (valid only if ShowModal doesn't return wxID_OK)
     */
    std::optional<wxString> get_error_text();

    void set_on_success_callback(SuccessCallback callback);

protected:
    virtual ExitCode Entry() override;

    /**
     * @brief Prepare data and count children (ie songs) that will be imported.
     * @return number of children
     * @retval <= 0 indicates error
     */
    virtual int count_children() = 0;

    /**
     * @brief Load song configuration
     * @param[in/out] song_entry load configuration
     * @param song_number number of song in list (same as `song_entry.song_id`)
     * @note
     * function should call `set_error_text` if an error occurs.  This will 
     * abort further processing.  This function should only load the
     * configuration - the MIDI events will be loaded when control is returned.
     */
    virtual void build_playlist_entry(PlayListEntry &song_entry,
                                      const uint32_t song_number) = 0;

    /**
     * @brief Set error text when a failure occurs (aborts futher processing)
     * @param error error text to report to the user
     */
    void set_error_text(const wxString &error);

private:
    enum LoaderEvents : int
    {
        START_EVENT = wxID_HIGHEST,
        SET_FILENAME_EVENT,
        TICK_EVENT,
        EXIT_EVENT
    };

    //  Events in event table
    void on_start_event(wxThreadEvent &event);
    void on_tick_event(wxThreadEvent &event);
    void on_close_event(wxThreadEvent &event);
    void on_filename_event(wxThreadEvent &event);

    /**
     * @brief Internal loop functionb to call playlist entry building.
     */
    void parse_playlist();

    void dummy_callback(std::list<PlayListEntry>);

    wxMutex m_mutex;
    std::list<PlayListEntry> m_playlist;
    std::optional<wxString> m_error_text;
    uint32_t m_count;
    size_t m_last_progress_len;
    SuccessCallback m_success_callback;

    wxDECLARE_EVENT_TABLE();
};

}  //  end ui
}  // end bach_bot
