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
#include <wx/wx.h>  //  wxLog, wxThread, etc
#include <RtMidi.h>  //  RtMidiOut

//  module includes
#include "MidiFile.h"  //  smf::MidiEventLis

//  local includes
#include "main_window.h"  //  MainWindow
#include "common_defs.h"  //  SyndyneBankCommands


//  Forward declare this class to prevent circular dependencies
class PlayerThread;


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
    virtual void on_play_advance(wxCommandEvent& event) override final;
    virtual void on_stop(wxCommandEvent& event) override final;
    virtual void on_new_playlist(wxCommandEvent& event) override final;
    virtual void on_load_playlist(wxCommandEvent& event) override final;
    virtual void on_save_playlist(wxCommandEvent& event) override final;
    virtual void on_open_midi(wxCommandEvent& event) override final;
    virtual void on_quit(wxCommandEvent& event) override final;
    virtual void on_about(wxCommandEvent& event) override final;
    virtual void on_manual_advance(wxCommandEvent& event) override final;
    virtual void on_manual_prev(wxCommandEvent& event) override final;
    virtual void on_manual_cancel(wxCommandEvent& event) override final;

private:
    //  Locally bound UI events
    void on_thread_tick(wxThreadEvent& event);
    void on_thread_exit(wxThreadEvent& event);
    void on_device_changed(const uint32_t device_id);
    
    /**
     * @brief Manually send an explicit bank-change message
     * @param value message to send
     */
    void send_manual_message(const SyndyneBankCommands value);

    /**
     * @brief Logic to build an appropriate midi sequence to send to the organ
     * @param value message to send
     */
    void build_syndine_sequence(const smf::MidiEventList &event_list) const;

    uint32_t m_counter;
    PlayerThread *m_player_thread;
    std::list<wxMenuItem> m_midi_devices;
    RtMidiOut m_midi_out;
    uint32_t m_current_device_id;

    wxDECLARE_EVENT_TABLE();

};
