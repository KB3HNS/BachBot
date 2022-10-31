/**
 * @file playlist_entry_control.h
 * @brief Playlist song entry display panel
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
#include <cstdint>  //  uint32_t
#include <utility>  //  std::pair
#include <functional>  //  std::function
#include <deque>  //  std::deque
#include <optional>  //  std::optional
#include <wx/wx.h>  //  wxString
#include <array>  //

 //  module includes
 // -none-

 //  local includes
#include "main_window.h"  //  PlaylistEntryPanel, LoadMidiDialog
#include "play_list.h"  //  PlayListEntry
#include "organ_midi_event.h"  //  BankConfig

namespace bach_bot {
namespace ui {

/**
 * @brief Utility to set a playlist entry string so that the right part is
 *        visible (usually containing the filename and not the path).
 * @param label Label to set
 * @param filename filename to store in label
 * @param max_len maximum number of characters label will support
 */
void set_label_filename(wxStaticText *const label,
                        const wxString &filename,
                        const size_t max_len);


/** Various color entries based on state */
enum PlaylistControlState : size_t
{
    ENTRY_NORMAL = 0U,
    ENTRY_NEXT,
    ENTRY_PLAYING,
    ENTRY_SELECTED,
    SIZE_COLOR_ARRAY
};


/** Event / Reason for generating a callback. */
enum PlaylistEntryEventId : int
{
    /** Autoplay changed event bool: current checkbox state */
    ENTRY_CHECKBOX_EVENT,

    /** Muve up/down request bool: `true` - up, `false` down */
    ENTRY_MOVED_EVENT,

    /** Set next song bool: unused (always true) */
    ENTRY_SET_NEXT_EVENT,

    /** Radio checkbox selected bool: current select state */
    ENTRY_SELECTED_EVENT,

    /** Entry deleted */
    ENTRY_DELETED_EVENT
};


/**
 * @brief On-screen control represenging an item in the playlist.
 */
class PlaylistEntryControl : public PlaylistEntryPanel
{
    /** Callback function format for events generated by this class */
    using CallBack = std::function<void(const PlaylistEntryEventId /* reason */,
                                        uint32_t /* song_id */,
                                        PlaylistEntryControl* /* `this` */,
                                        bool /* Function specific */)>;

public:
    /**
     * @brief Constructor
     * @param parent assigned parent window (scroll panel)
     * @param song song configuration
     */
    PlaylistEntryControl(wxWindow *const parent, PlayListEntry song);

    /**
     * @brief Get the filename of this song
     * @return file name
     */
    const wxString& get_filename() const;
    
    /**
     * @brief Set this song as "next to be played"
     */
    void set_next();

    /**
     * @brief Set this song as currently playing.
     */
    void set_playing();

    /**
     * @brief Clear "now playing" and "plays next" statuses
     */
    void reset_status();

    /**
     * @brief Set the autoplay checkmark
     * @param autoplay_enabled state to set to
     */
    void set_autoplay(const bool autoplay_enabled);

    /**
     * @brief Get the auto play next bool
     * @return auto play/continue to next
     */
    bool get_autoplay() const;

    /**
     * @brief Set the callback event handler function.
     * @param event_hnadler function to handle events from this control
     */
    void set_callback(CallBack event_hnadler);

    /**
     * @brief Swap this control with another
     * @param other other control to swap with
     */
    void swap(PlaylistEntryControl *const other);

    /**
    * @brief Get song events
    * @return Playlist song's events
    */
    std::deque<OrganMidiEvent> get_song_events() const;

    /**
     * @brief Get the sequence (prev song/next song) data
     * @returns pair<prev song ID, next song ID>
     */
    std::pair<uint32_t, uint32_t> get_sequence() const;

    /**
     * @brief Set this song's sequence
     * @param prev song ID of the song immediately preceeding this one
     * @param next song ID of the song immediately following this one
     * @note if either prev or next are < 0 then parameter is ignored.
     */
    void set_sequence(const int prev=-1, const int next=-1);

    /**
     * @brief Save current entry to XML structure
     * @param playlist_node store config into XML node
     */
    void save_config(wxXmlNode *const playlist_node) const
    {
        m_playlist_entry.save_config(playlist_node);
    }

    uint32_t get_song_id() const
    {
        return m_playlist_entry.song_id;
    }

    /**
     * @brief Periodically check and update this entry background color.
     * @param up_next `true` if up next, `false` otherwise
     */
    void update_color_state(const bool up_next);

    /**
     * @brief Externally set the selected state without triggering a callback.
     * @param selected set to selected or not
     */
    void select(const bool selected=true);

    /**
     * @brief Get the "am I selected" flag
     * @return control selected flag
     */
    bool is_selected() const
    {
        return now_playing->GetValue();
    }

    /**
     * @brief Get the starting regstration of this entry in the playlist.
     * @return Starting registration (memory, bank)
     */
    BankConfig get_starting_registration() const;

    /**
     * @brief Apply changes from a group dialog box.
     * @param dialog reference to group edit dialog.
     * @returns `false` if some sort of import error occurred, `true` otherwise
     * @note In the event of an import error, a dialow box will pop-up with
     *       a detail of the failure.
     */
    bool apply_group_dialog(const GroupEditMidiDialog& dialog);

protected:
    virtual void on_configure_clicked(wxCommandEvent& event) override final;
    virtual void on_checkbox_checked(wxCommandEvent &event) override final;
    virtual void on_set_next(wxCommandEvent &event) override final;
    virtual void on_move_up(wxCommandEvent &event) override final;
    virtual void on_move_down(wxCommandEvent &event) override final;
    virtual void on_radio_selected(wxCommandEvent& event) override final;
    virtual void on_remove_song(wxCommandEvent& event) override final;
    virtual void PlaylistEntryPanelOnSize(wxSizeEvent &event) override final;

private:
    /**
     * @brief (Re-)Setup the controls (filename, playing, next) based on new
     *        state.
     */
    void setup_widgets();

    /**
     * @brief Dummy callback function to use if callbacks aren't assigned.
     */
    static void dummy_event(const PlaylistEntryEventId,
                            uint32_t, PlaylistEntryControl*, bool);

    static double calculate_pix_per_char(const wxStaticText *const label);

    wxWindow *const m_parent;
    bool m_up_next;
    bool m_playing;
    uint32_t m_prev_song_id;
    uint32_t m_next_song_id;
    const wxSize m_panel_size;
    // std::optional<std::pair<wxSize, wxSize>> m_initial_sizes;
    uint32_t m_text_width;
    const double m_pix_per_char;

    PlayListEntry m_playlist_entry;
    LoadMidiDialog *m_active_dialog;

    static const size_t ARRAY_SIZE = PlaylistControlState::SIZE_COLOR_ARRAY;
    const std::array<wxColor, ARRAY_SIZE> m_colors;
    CallBack m_event_handler;
    bool m_currently_selected;
};

}  //  end ui
}  //  end bach_bot
