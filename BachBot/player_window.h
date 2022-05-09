/**
 * @file player_window.h
 * @brief Application main window
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
 * Class definition for the main application window.
 */

#pragma once

//  system includes
#include <cstdint>  //  uint32_t
#include <list>  //  std::list
#include <memory>  //  std::unique_ptr
#include <unordered_map>  //  std::unordered_map
#include <wx/wx.h>  //  wxLog, wxThread, etc
#include <RtMidi.h>  //  RtMidiOut

//  module includes
// -none-

//  local includes
#include "main_window.h"  //  MainWindow
#include "common_defs.h"  //  SyndyneBankCommands
#include "play_list.h"  //  PlayList, PlayListEntry
#include "playlist_entry_control.h"  //  PlaylistEntryControl
#include "organ_midi_event.h"  //  BankConfig

namespace bach_bot {
//  Forward declare this class to prevent circular dependencies
class PlayerThread;

namespace ui {

/**
 * @brief wx Events handled by this class
 */
enum PlayerWindowEvents : int
{
    //  Player thread events
    /**
     * @brief Periodic message sent to UI to refresh screen.
     * @note
     * "Int" value contains events remaining. 
     */
    TICK_EVENT = 1001,
    SONG_START_EVENT,  ///< On start playing song, "Int" is song id.
    SONG_LYRIC_EVENT,  ///< Update lyrics, int is string number (future)
    SONG_META_EVENT,  ///< Future use
    /**
     * @brief Sent on bank change format same as tick
     * @note
     * "Int" value is formatted as `(mode << 3) | bank`
     */
    BANK_CHANGE_EVENT,
    /**
     * @brief Song ended
     * @note "Int" 0 -> do not anadvance, != 0 advance to next song
     */
    SONG_END_EVENT,
    EXIT_EVENT,  ///< On thread exit message "Int" is return code.

    //  Playlist entry events
};


/**
 * @brief Application main window
 */
class PlayerWindow : public MainWindow, private wxLog
{
    //  Allow the player thread to directly access the midi interface
    friend class PlayerThread;
    using PlaylistEntryType = std::unique_ptr<PlaylistEntryControl>;

public:

    /**
     * @brief default constructor
     */
    PlayerWindow();

    virtual ~PlayerWindow() override;

protected:
    
    // Events defined in the UI builder
    virtual void on_play_advance(wxCommandEvent &event) override final;
    virtual void on_stop(wxCommandEvent &event) override final;
    virtual void on_new_playlist(wxCommandEvent &event) override final;
    virtual void on_load_playlist(wxCommandEvent &event) override final;
    virtual void on_save_playlist(wxCommandEvent &event) override final;
    virtual void on_open_midi(wxCommandEvent &event) override final;
    virtual void on_quit(wxCommandEvent &event) override final;
    virtual void on_about(wxCommandEvent &event) override final;
    virtual void on_manual_advance(wxCommandEvent &event) override final;
    virtual void on_manual_prev(wxCommandEvent &event) override final;
    virtual void on_manual_cancel(wxCommandEvent &event) override final;
    virtual void on_close(wxCloseEvent& event) override final;

private:
    //  Locally bound UI events
    void on_thread_tick(wxThreadEvent &event);
    void on_thread_exit(wxThreadEvent &event);
    void on_bank_changed(wxThreadEvent &event);
    void on_song_starts_playing(wxThreadEvent &event);
    void on_song_done_playing(wxThreadEvent &event);

    /**
     * @brief Internal device changed handler
     * @param device_id new MIDI out device ID selected
     */
    void on_device_changed(const uint32_t device_id);

    void on_autoplay_checked(PlaylistEntryPanel *const panel);

    /**
     * @brief Manually send an explicit bank-change message
     * @param value message to send
     */
    void send_manual_message(const SyndyneBankCommands value);

    /**
     * @brief Clear the playlist window GUI
     */
    void clear_playlist_window();

    void add_playlist_entry(const PlayListEntry &song);

    void layout_scroll_panel() const;

    uint32_t m_counter;
    std::unique_ptr<PlayerThread> m_player_thread;
    std::list<wxMenuItem> m_midi_devices;
    RtMidiOut m_midi_out;
    uint32_t m_current_device_id;
    PlayList m_playlist;
    size_t m_current_song_event_count;
    uint32_t m_current_song_id;
    uint32_t m_next_song_id;
    uint32_t m_first_song_id;
    uint32_t m_last_song_id;
    std::unordered_map<uint32_t, PlaylistEntryType> m_song_labels;
    BankConfig m_current_config;

    wxDECLARE_EVENT_TABLE();

};

}  //  end ui
}  //  end bach_bot
