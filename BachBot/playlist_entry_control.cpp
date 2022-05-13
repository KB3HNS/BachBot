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
#include <utility>  //  std::swap
#include <stdexcept>  //  std::runtime_error
#include <fmt/xchar.h>  //  fmt::format(L
#include <fmt/format.h>  //  fmt::format

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
                                           PlayListEntry song) :
    PlaylistEntryPanel(parent),
    configure_event(dummy_event),
    checkbox_event(dummy_event),
    move_event(dummy_event),
    set_next_event(dummy_event),
    m_parent{parent},
    m_up_next{false},
    m_playing{false},
    m_prev_song_id{0U},
    m_next_song_id{0U},
    m_playlist_entry(std::move(song))
{
    auto_play->SetValue(song.play_next);
    static_cast<void>(now_playing->Enable(false));
    setup_widgets();
}


const wxString& PlaylistEntryControl::get_filename() const
{
    return m_playlist_entry.file_name;
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
    m_playlist_entry.play_next = autoplay_enabled;
    auto_play->SetValue(autoplay_enabled);
}


bool PlaylistEntryControl::get_autoplay() const
{
    return m_playlist_entry.play_next;
}


void PlaylistEntryControl::swap(PlaylistEntryControl *const other)
{
    auto update_ui = [](PlaylistEntryControl *const control) {
        control->auto_play->SetValue(control->m_playlist_entry.play_next);
        control->setup_widgets();
    };

    if (other->m_prev_song_id == m_playlist_entry.song_id) {
        std::swap(m_next_song_id, other->m_prev_song_id);
    } else if (other->m_next_song_id == m_playlist_entry.song_id) {
        std::swap(other->m_next_song_id, m_prev_song_id);
    } else {
        throw std::runtime_error(fmt::format("{} is not related to {}",
                                             m_playlist_entry.song_id,
                                             other->get_song_id()));
    }

    std::swap(m_playlist_entry, other->m_playlist_entry);
    std::swap(m_up_next, other->m_up_next);
    std::swap(m_playing, other->m_playing);

    update_ui(other);
    update_ui(this);
}


std::list<OrganMidiEvent> PlaylistEntryControl::get_song_events() const
{
    std::list<OrganMidiEvent> events;
    std::for_each(m_playlist_entry.midi_events.begin(),
                  m_playlist_entry.midi_events.end(),
                  [&events](const OrganNote &event) {
                      events.emplace_back(event.clone());
                  });

    return events;
}


std::pair<uint32_t, uint32_t> PlaylistEntryControl::get_sequence() const
{
    return std::make_pair(m_prev_song_id, m_next_song_id);
}


void PlaylistEntryControl::set_sequence(const int prev, const int next)
{
    if (prev >= 0) {
        m_prev_song_id = uint32_t(prev);
    }
    if (next >= 0) {
        m_next_song_id = uint32_t(next);
    }
}


void PlaylistEntryControl::on_configure_clicked(wxCommandEvent &event)
{
    configure_event(m_playlist_entry.song_id, this, false);
}


void PlaylistEntryControl::on_checkbox_checked(wxCommandEvent &event)
{
    wxMessageBox(fmt::format(L"PlayerWindow::on_autoplay_checked: {}",
                             int(auto_play->IsChecked())),
                 wxT("Debug"),
                 wxOK | wxICON_INFORMATION);

    checkbox_event(m_playlist_entry.song_id, this, auto_play->IsChecked());
}


void PlaylistEntryControl::on_set_next(wxCommandEvent &event)
{
    set_next_event(m_playlist_entry.song_id, this, true);
}


void PlaylistEntryControl::on_move_up(wxCommandEvent & event)
{
    if (0U != m_prev_song_id) {
        move_event(m_playlist_entry.song_id, this, true);
    }
}


void PlaylistEntryControl::on_move_down(wxCommandEvent & event)
{
    if (0U != m_next_song_id) {
        move_event(m_playlist_entry.song_id, this, false);
    }
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
    static_cast<void>(configure_button->Enable(!m_playing));
    now_playing->SetValue(m_up_next);

    if (m_playlist_entry.file_name.length() < width) {
        song_label->SetLabelText(m_playlist_entry.file_name);
    } else {
        const wxString &label = fmt::format(
            L"...{}", m_playlist_entry.file_name.Right(width - 3U));
        song_label->SetLabelText(label);
    }

    Layout();
}


void PlaylistEntryControl::dummy_event(uint32_t, PlaylistEntryControl*, bool)
{
}

}  //  end ui
}  //  end bach_bot
