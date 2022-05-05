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
#include <wx/wx.h>  //  wxLog, wxThread, etc
#include <RtMidi.h>  //  RtMidiOut

//  module includes
// -none-

//  local includes
#include "main_window.h"  //  MainWindow
#include "common_defs.h"  //  SyndyneBankCommands
#include "play_list.h"  //  PlayList


namespace bach_bot {
//  Forward declare this class to prevent circular dependencies
class PlayerThread;

namespace ui {


/**
 * @brief Application main window
 */
class PlayerWindow : public MainWindow, private wxLog
{
    //  Allow the player thread to directly access the midi interface
    friend class PlayerThread;

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

private:
    //  Locally bound UI events
    void on_thread_tick(wxThreadEvent &event);
    void on_thread_exit(wxThreadEvent &event);
    void on_device_changed(const uint32_t device_id);
    void on_bank_changed(wxThreadEvent &event);
    void on_song_starts_playing(wxThreadEvent &event);

    /**
     * @brief Manually send an explicit bank-change message
     * @param value message to send
     */
    void send_manual_message(const SyndyneBankCommands value);

    /**
     * @brief Clear the playlist window GUI
     */
    void clear_playlist_window();

    uint32_t m_counter;
    std::unique_ptr<PlayerThread> m_player_thread;
    std::list<wxMenuItem> m_midi_devices;
    RtMidiOut m_midi_out;
    uint32_t m_current_device_id;
    PlayList m_playlist;
    size_t m_current_song_event_count;
    uint32_t m_current_song_id;
    uint32_t m_first_song_id;
    uint32_t m_last_song_id;
    std::list<wxStaticText> m_song_labels;

    wxDECLARE_EVENT_TABLE();

};

}  //  end ui
}  //  end bach_bot
