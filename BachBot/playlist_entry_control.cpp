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
#include <fmt/xchar.h>  //  fmt::format(L

//  module includes
// -none-

//  local includes
#include "playlist_entry_control.h"  //  local include


namespace {
    /** The amount of text normally allowed in the filename label */
    constexpr const auto NORMAL_WIDTH = 36U;
}  //  end anonymous namespace

namespace bach_bot {
namespace ui {

PlaylistEntryControl::PlaylistEntryControl(wxWindow *const parent,
                                           const PlayListEntry &song) :
    PlaylistEntryPanel(parent),
    configure_event(dummy_event),
    checkbox_event(dummy_event),
    move_event(dummy_event),
    set_next_event(dummy_event),
    m_parent{parent},
    m_song_id{song.current_song_id},
    m_filename{song.file_name},
    m_autoplay{song.play_next},
    m_up_next{false},
    m_playing{false}
{
    auto_play->SetValue(song.play_next);
    static_cast<void>(now_playing->Enable(false));
    setup_widgets();
}


const wxString& PlaylistEntryControl::get_filename() const
{
    return m_filename;
}


void PlaylistEntryControl::set_next()
{
    m_up_next = true;
    m_playing = false;
    setup_widgets();
}


void PlaylistEntryControl::set_playing()
{
    m_up_next = false;
    m_playing = true;
    setup_widgets();
}


void PlaylistEntryControl::reset_status()
{
    m_up_next = false;
    m_playing = false;
    setup_widgets();
}


void PlaylistEntryControl::set_autoplay(const bool autoplay_enabled)
{
    m_autoplay = autoplay_enabled;
    auto_play->SetValue(autoplay_enabled);
}


void PlaylistEntryControl::on_configure_clicked(wxCommandEvent &event)
{
    configure_event(m_song_id, this, false);
}


void PlaylistEntryControl::on_checkbox_checked(wxCommandEvent &event)
{
    wxMessageBox(fmt::format(L"PlayerWindow::on_autoplay_checked: {}",
                             int(auto_play->IsChecked())),
                 wxT("Debug"),
                 wxOK | wxICON_INFORMATION);

    checkbox_event(m_song_id, this, auto_play->IsChecked());
}


void PlaylistEntryControl::on_set_next(wxCommandEvent &event)
{
    set_next_event(m_song_id, this, true);
}


void PlaylistEntryControl::on_move_up(wxCommandEvent & event)
{
    move_event(m_song_id, this, true);
}


void PlaylistEntryControl::on_move_down(wxCommandEvent & event)
{
    move_event(m_song_id, this, false);
}


void PlaylistEntryControl::setup_widgets()
{
    auto width = NORMAL_WIDTH;
    if (m_playing) {
        now_playing->SetLabelText(wxT("==>"));
        width -= 4U;
    } else {
        now_playing->SetLabelText(wxT(""));
    }
    now_playing->SetValue(m_up_next);

    if (m_filename.length() < width) {
        song_label->SetLabelText(m_filename);
    } else {
        const wxString &label = fmt::format(L"...{}",
                                            m_filename.Right(width - 3U));
        song_label->SetLabelText(label);
    }
}


void PlaylistEntryControl::dummy_event(uint32_t, PlaylistEntryControl*, bool)
{
}

}  //  end ui
}  //  end bach_bot
