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
#include <optional>  //  std::optional
#include <wx/wx.h>  //  wxString
#include <wx/xml/xml.h>  //  wxXml API

//  module includes
// -none-

//  local includes
#include "organ_midi_event.h"  //  OrganNote, BankConfig
#include "main_window.h"  //  ui::LoadMidiDialog
#include "syndyne_importer.h"  //  SyndineImporter

namespace bach_bot {

/**
 * @brief Single song entry in a playlist
 */
struct PlayListEntry
{
    uint32_t song_id;

    //  Configuration items
    wxString file_name;
    int tempo_requested;
    double gap_beats;
    BankConfig starting_config;
    int delta_pitch;
    double last_note_multiplier;
    bool play_next;

    //  Actual song data
    std::optional<int> tempo_detected;
    std::list<OrganNote> midi_events;

    /**
     * @brief Load MIDI file and import events
     * @param importer optionally provide an already allocated SyndineImporter
     *        instance to use to load events
     * @retval `false` song not loaded
     * @retval `true` song loaded successfully
     */
    bool import_midi(SyndineImporter *importer=nullptr);

    /**
     * @brief Load the playlist configuration into the song_entry structure
     * @param playlist_node XML node for data
     * @throws std::out_of_rang if the filename node is empty
     * @retval `true` data loaded
     * @retval `false` parse error
     */
    bool load_config(const wxXmlNode *const playlist_node);

    /**
     * @brief Load configuration from a dialog box
     * @param dialog dialog box
     * @returns form field name containing invalid data
     * @retval `false` no errors in form
     */
    std::optional<wxString> load_config(const ui::LoadMidiDialog &dialog);

    /**
     * @brief Save current entry to XML structure
     * @param playlist_node store config into XML node
     */
    void save_config(wxXmlNode *const playlist_node) const;

    /**
     * @brief Populate a configure song dialog box based on configuration.
     * @param[in] dialog 
     */
    void populate_dialog(ui::LoadMidiDialog &dialog) const;

    static constexpr const auto CFGMIDI_DIALOG_MAX_LEN = 36U;
};


}  //  end bach_bot
