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
#include <utility>  //  std::pair
#include <map>  //  std::map
#include <optional>  //  std::optional
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
#include "label_animator.h"  //  LabelAnimator
#include "bitmap_painter.h"  //  BitmapPainter


namespace bach_bot {
//  Forward declare this class to prevent circular dependencies
class PlayerThread;

namespace ui {

/**
 * @brief One-time initialize the global accelerator table on startup.
 */
void initialize_global_accelerator_table();


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
    TICK_EVENT = wxID_HIGHEST,
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

    //  Internal events
    MOVE_DOWN_EVENT,  ///< On Move down accelerator (Ctrl+Down)
    MOVE_UP_EVENT,  ///< On Move up accelerator (Ctrl+Up)
    SET_NEXT_EVENT,  ///< On Set next accelerator (Ctrl+Enter
    PLAY_ACTIVATE_EVENT,   ///< On Play/Activate accelerator (F5)
    UI_ANIMATE_TICK,  ///< Timer tick event

    END_UI_EVENTS  ///< Terminating item, not used by UI
};


/**
 * @brief Application main window
 */
class PlayerWindow : public MainWindow, private wxLog
{
    //  Allow the player thread to directly access the midi interface
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
    virtual void on_close(wxCloseEvent &event) override final;
    virtual void on_save_as(wxCommandEvent &event) override final;
    virtual void on_drop_midi_file(wxDropFilesEvent &event) override final;
    virtual void on_memory_up_button_clicked(wxCommandEvent &event) override final;
    virtual void on_memory_down_button_clicked(wxCommandEvent &event) override final;
    virtual void on_mode_up_button_clicked(wxCommandEvent &event) override final;
    virtual void on_mode_down_button_clicked(wxCommandEvent &event) override final;
    virtual void next_buttonOnButtonClick(wxCommandEvent &event) override final;
    virtual void prev_buttonOnButtonClick(wxCommandEvent &event) override final;
    virtual void cancel_buttonOnButtonClick(wxCommandEvent &event) override final;
    virtual void on_sync_button_clicked(wxCommandEvent &event) override final;
    virtual void on_select_multi(wxCommandEvent &event) override final;
    virtual void on_clear_selection(wxCommandEvent &event) override final;
    virtual void on_shift_up(wxCommandEvent &event) override final;
    virtual void on_shift_down(wxCommandEvent &event) override final;
    virtual void on_group_edit(wxCommandEvent &event) override final;
    virtual void on_delete_selected(wxCommandEvent &event) override final;

private:
    //  Locally bound UI events
    void on_thread_tick(wxThreadEvent &event);
    void on_thread_exit(wxThreadEvent &event);
    void on_bank_changed(wxThreadEvent &event);
    void on_song_starts_playing(wxThreadEvent &event);
    void on_song_done_playing(wxThreadEvent &event);
    void on_accel_down_event(wxCommandEvent &event);
    void on_accel_up_event(wxCommandEvent &event);
    void on_accel_play_next_event(wxCommandEvent &event);
    void on_timer_tick(wxTimerEvent &event);

    /**
     * @brief Control menu move event handler
     * @param song_id control song ID
     * @param control pointer to control
     * @param direction direction to be moved
     * @sa PlaylistEntryControl::move_event
     */
    void on_move_event(const uint32_t song_id,
                       PlaylistEntryControl *control,
                       const bool direction);

    /**
     * @brief on checkbox clicked event handler for playlist controls
     * @param song_id song ID associated with playlist entry
     * @param checked new state of auto play next checkbox
     */
    void on_checkbox_event(const uint32_t song_id, const bool checked);

    /**
     * @brief Internal device changed handler
     * @param device_id new MIDI out device ID selected
     */
    void on_device_changed(const uint32_t device_id);

    /**
     * @brief Manually send an explicit bank-change message
     * @param value message to send
     */
    void send_manual_message(const SyndyneBankCommands value);

    /**
     * @brief Clear the playlist window GUI
     */
    void clear_playlist_window();

    /**
     * @brief Add entry `song` to the end of the playlist
     * @param song song data entry
     */
    void add_playlist_entry(const PlayListEntry &song);

    /**
     * @brief Reset the layout of the scroll panel
     */
    void layout_scroll_panel() const;

    /**
     * @brief Set the next song ID (update GUI and player thread)
     * @note This function may not actually change the selection of next song
     *       if the order changed because of reordering the playlist and the
     *       user has already told us to "play *THIS* song next.
     * @param song_id song ID that is "next"
     * @param priority set `true` when requested through the UI as opposed to
     *        playback / reordering action.
     */
    void set_next_song(uint32_t song_id, const bool priority=false);

    /**
     * @brief check to see if the application should be closed
     * @param event incoming event from wxWidgets
     * @retval `true` ok to close application
     * @retval `false user cancelled, prevent closure (veto event)
     */
    bool pre_close_check(wxCommandEvent &event);

    /**
     * @brief Consolidated logic to initialize the player thread and update the
     *        UI.
     */
    void start_player_thread();

    /**
     * @brief Scroll the playlist container to (roughly) center on a widget
     * @param widget widget to center on
     */
    void scroll_to_widget(const PlaylistEntryControl *const widget);

    /**
     * @brief Update the "Current Config" section of the UI
     * @param send_update set `false` to not notify the player thread of the
     *                    current config
     */
    void update_config_ui(const bool send_update=true);

    /**
     * @brief Update the window title with platlist data
     * @param playlist_changed flag to set if playlist changed or not
     */
    void update_window_title(const bool playlist_changed);

    /**
     * @brief Remove a song from the playlist.
     * @param widget control representing song to remove.
     */
    void remove_song(PlaylistEntryControl *const widget,
                     const uint32_t song_id);

    /**
     * @brief Logic to handle when a playlist entry has been selected.
     * @param song_id song ID of entry
     * @param widget widget emitting selected
     */
    void on_control_selected(const uint32_t song_id,
                             PlaylistEntryControl *const widget);

    std::unique_ptr<PlayerThread> m_player_thread;
    std::list<wxMenuItem> m_midi_devices;
    RtMidiOut m_midi_out;
    uint32_t m_current_device_id;
    size_t m_current_song_event_count;
    uint32_t m_current_song_id;
    std::pair<uint32_t, bool> m_next_song_id;
    std::pair<uint32_t, uint32_t> m_song_list;  ///< front/end of playlist
    std::map<uint32_t, PlaylistEntryType> m_song_labels;
    BankConfig m_current_config;
    std::optional<wxString> m_playlist_name;
    bool m_playlist_changed;
    PlaylistEntryControl *m_selected_control;
    wxAcceleratorTable m_accel_table;
    wxTimer m_ui_animation_timer;
    LabelAnimator m_up_next_label;
    LabelAnimator m_playing_label;
    BitmapPainter m_background;
    bool m_sync_config;

    wxDECLARE_EVENT_TABLE();

};

}  //  end ui
}  //  end bach_bot
