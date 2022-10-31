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
#include <optional>  //  std::optional
#include <fmt/xchar.h>  //  fmt::format(L
#include <fmt/format.h>  //  fmt::format

//  module includes
// -none-

//  local includes
#include "playlist_entry_control.h"  //  local include


namespace {
/** The amount of text normally allowed in the filename label */
constexpr const auto NORMAL_WIDTH = 87U;

const wxColor default_color(uint8_t(171), uint8_t(171), uint8_t(171));
}  //  end anonymous namespace

namespace bach_bot {
namespace ui {

void set_label_filename(wxStaticText *const label,
                        const wxString &filename,
                        const size_t max_len)
{
    if (filename.length() > max_len) {
        label->SetLabelText(fmt::format(L"...{}",
                                        filename.Right(max_len - 3U)));
    } else {
        label->SetLabelText(filename);
    }
}


PlaylistEntryControl::PlaylistEntryControl(wxWindow *const parent,
                                           PlayListEntry song) :
    PlaylistEntryPanel(parent),
    m_parent{parent},
    m_up_next{false},
    m_playing{false},
    m_prev_song_id{0U},
    m_next_song_id{0U},
    m_panel_size{GetSize()},
    m_text_width{NORMAL_WIDTH},
    m_pix_per_char{calculate_pix_per_char(song_label)},
    m_playlist_entry(std::move(song)),
    m_active_dialog{nullptr},
    m_colors{parent->GetBackgroundColour(),
             *wxYELLOW,
             *wxGREEN,
             *wxLIGHT_GREY},
    m_event_handler(dummy_event),
    m_currently_selected{false}
{
    auto_play->SetValue(song.play_next);
    setup_widgets();
}


const wxString& PlaylistEntryControl::get_filename() const
{
    return m_playlist_entry.file_name;
}


void PlaylistEntryControl::set_next()
{
    m_up_next = true;
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


void PlaylistEntryControl::set_callback(CallBack event_handler)
{
    m_event_handler = event_handler;
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

    if (now_playing->GetValue()) {
        now_playing->SetValue(false);
        m_event_handler(PlaylistEntryEventId::ENTRY_SELECTED_EVENT,
                        m_playlist_entry.song_id,
                        this,
                        false);

        other->now_playing->SetValue(true);
        m_event_handler(PlaylistEntryEventId::ENTRY_SELECTED_EVENT,
                        other->m_playlist_entry.song_id,
                        other,
                        true);
    } else if (other->now_playing->GetValue()) {
        other->now_playing->SetValue(false);


        m_event_handler(PlaylistEntryEventId::ENTRY_SELECTED_EVENT,
                        other->m_playlist_entry.song_id,
                        other,
                        false);

        now_playing->SetValue(true);
        m_event_handler(PlaylistEntryEventId::ENTRY_SELECTED_EVENT,
                        m_playlist_entry.song_id,
                        this,
                        true);
    }

    std::swap(m_playlist_entry, other->m_playlist_entry);
    std::swap(m_up_next, other->m_up_next);
    std::swap(m_playing, other->m_playing);

    update_ui(other);
    update_ui(this);
}


std::deque<OrganMidiEvent> PlaylistEntryControl::get_song_events() const
{
    std::deque<OrganMidiEvent> events;
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


void PlaylistEntryControl::update_color_state(const bool up_next)
{
    auto index = (now_playing->GetValue() ?
                  PlaylistControlState::ENTRY_SELECTED :
                  PlaylistControlState::ENTRY_NORMAL);
    if (m_playing) {
        index = PlaylistControlState::ENTRY_PLAYING;
    } else if (up_next) {
        index = PlaylistControlState::ENTRY_NEXT;
    }

    const auto &color = m_colors[index];
    if (GetBackgroundColour() != color) {
        SetBackgroundColour(color);
        Refresh();
    }
}


void PlaylistEntryControl::select(const bool selected)
{
    m_currently_selected = selected;
    if (now_playing->GetValue() != selected) {
        now_playing->SetValue(selected);
    }
}


BankConfig PlaylistEntryControl::get_starting_registration() const
{
    return m_playlist_entry.starting_config;
}


bool PlaylistEntryControl::apply_group_dialog(const GroupEditMidiDialog &dialog)
{
    LoadMidiDialog update_dialog(m_parent->GetGrandParent());
    m_playlist_entry.populate_dialog(update_dialog);

    //  Apply edit dialog
    if (dialog.tempo_checkbox->IsChecked()) {
        const auto tempo = update_dialog.select_tempo->GetValue() +
                           dialog.select_tempo->GetValue();

        if (tempo < update_dialog.select_tempo->GetMin() || 
                tempo > update_dialog.select_tempo->GetMax()) {
            wxMessageBox(
                fmt::format(L"Tempo adjust {} results in a tempo out-of-range {}",
                            dialog.select_tempo->GetValue(), tempo),
                wxT("Form Error"),
                wxOK | wxICON_INFORMATION);

            return false;
        }

        update_dialog.select_tempo->SetValue(tempo);
    }

    if (dialog.silence_checkbox->IsChecked()) {
        update_dialog.initial_gap_text_box->SetValue(
            dialog.initial_gap_text_box->GetValue());
    }

    if (dialog.bank_config_checkbox->IsChecked()) {
        update_dialog.memory_select->SetValue(dialog.memory_select->GetValue());
        update_dialog.mode_select->SetValue(dialog.mode_select->GetValue());
    }

    if (dialog.pitch_checkbox->IsChecked()) {
        update_dialog.pitch_change->SetValue(dialog.pitch_change->GetValue());
    }

    if (dialog.extend_ending_checkbox->IsChecked()) {
        update_dialog.extend_ending_textbox->SetValue(
            dialog.extend_ending_textbox->GetValue());
    }

    if (dialog.apply_play_next_checkbox->IsChecked()) {
        update_dialog.play_next_checkbox->SetValue(
            dialog.play_next_checkbox->IsChecked());
    }

    auto error_text = m_playlist_entry.load_config(update_dialog);
    if (error_text.has_value()) {
        wxMessageBox(error_text.value(), wxT("Form Error"),
                     wxOK | wxICON_INFORMATION);
        return false;
    }

    if (m_playlist_entry.import_midi()) {
        auto_play->SetValue(m_playlist_entry.play_next);
        if (dialog.apply_play_next_checkbox->IsChecked()) {
            m_event_handler(PlaylistEntryEventId::ENTRY_CHECKBOX_EVENT,
                            m_playlist_entry.song_id,
                            this,
                            m_playlist_entry.play_next);
        }
    } else {
        wxMessageBox(
            fmt::format(L"Failed to import for {}", m_playlist_entry.file_name),
            wxT("Import Error"),
            wxOK | wxICON_INFORMATION);

        return false;
    }

    return true;
}


void PlaylistEntryControl::on_configure_clicked(wxCommandEvent &event)
{
    LoadMidiDialog update_dialog(m_parent->GetGrandParent());
    m_playlist_entry.populate_dialog(update_dialog);

    std::optional<wxString> error_text;
    auto imported = false;
    do {
        if (error_text.has_value()) {
            wxMessageBox(error_text.value(), wxT("Form Error"),
                         wxOK | wxICON_INFORMATION);
        }

        m_active_dialog = &update_dialog;
        const auto result = update_dialog.ShowModal();
        m_active_dialog = nullptr;
        if (wxID_CANCEL == result) {
            return;
        }

    } while (error_text.has_value());

    if (m_playlist_entry.import_midi()) {
        auto_play->SetValue(m_playlist_entry.play_next);
        m_event_handler(PlaylistEntryEventId::ENTRY_CHECKBOX_EVENT,
                        m_playlist_entry.song_id,
                        this,
                        m_playlist_entry.play_next);
    }
}


void PlaylistEntryControl::on_checkbox_checked(wxCommandEvent &event)
{
    const auto checked = auto_play->IsChecked();
    const auto changed = (m_playlist_entry.play_next != checked);
    m_playlist_entry.play_next = checked;
    if (changed) {
        m_event_handler(PlaylistEntryEventId::ENTRY_CHECKBOX_EVENT,
                        m_playlist_entry.song_id,
                        this,
                        checked);
    }
}


void PlaylistEntryControl::on_set_next(wxCommandEvent &event)
{
    m_event_handler(PlaylistEntryEventId::ENTRY_SET_NEXT_EVENT,
                    m_playlist_entry.song_id,
                    this,
                    true);
}


void PlaylistEntryControl::on_move_up(wxCommandEvent & event)
{
    if (0U != m_prev_song_id) {
        m_event_handler(PlaylistEntryEventId::ENTRY_MOVED_EVENT,
                        m_playlist_entry.song_id,
                        this,
                        true);
    }
}


void PlaylistEntryControl::on_move_down(wxCommandEvent & event)
{
    if (0U != m_next_song_id) {
        m_event_handler(PlaylistEntryEventId::ENTRY_MOVED_EVENT,
                        m_playlist_entry.song_id,
                        this,
                        false);
    }
}


void PlaylistEntryControl::on_radio_selected(wxCommandEvent &event)
{
    const auto selected = now_playing->GetValue();
    if (selected != m_currently_selected) {
        m_currently_selected = selected;
        m_event_handler(PlaylistEntryEventId::ENTRY_SELECTED_EVENT,
                        m_playlist_entry.song_id,
                        this,
                        selected);
    }
    setup_widgets();
}


void PlaylistEntryControl::on_remove_song(wxCommandEvent &event)
{
    static_cast<void>(event);
    CallAfter([=]() {
        if (!m_playing) {
            m_event_handler(PlaylistEntryEventId::ENTRY_DELETED_EVENT,
                            m_playlist_entry.song_id,
                            this,
                            false);
        }
    });
}


void PlaylistEntryControl::PlaylistEntryPanelOnSize(wxSizeEvent &event)
{
    const auto new_size = event.GetSize();
    const auto delta_x = double(new_size.x - m_panel_size.x);
    m_text_width = NORMAL_WIDTH;
    if (delta_x > 0.0) {
        m_text_width += int(delta_x / m_pix_per_char);
    }
    setup_widgets();
}


void PlaylistEntryControl::setup_widgets()
{
    auto width = m_text_width;
    delete_entry_menu->Enable(!m_playing);
    if (m_playing) {
        now_playing->SetLabelText(wxT("==>"));
        width -= 6U;
    } else {
        now_playing->SetLabelText(wxT(""));
    }
    
    const auto edit_forbidden = (m_playing || m_up_next);
    static_cast<void>(configure_button->Enable(!edit_forbidden));
    if (edit_forbidden && (nullptr != m_active_dialog)) {
        m_active_dialog->Close();
    }

    set_label_filename(song_label, m_playlist_entry.file_name, width);

    Layout();
}


void PlaylistEntryControl::dummy_event(const PlaylistEntryEventId reason,
                                       uint32_t song_id, 
                                       PlaylistEntryControl*,
                                       bool value)
{
    wxMessageBox(fmt::format(L"PlayerWindow::unhandled_dummy_event {} value: {} me={}",
                             int(reason), int(value), song_id),
                 wxT("Debug"),
                 wxOK | wxICON_INFORMATION);
}


double PlaylistEntryControl::calculate_pix_per_char(
    const wxStaticText *const label)
{
    const auto label_size = label->GetSize();
    return double(label_size.x) / double(NORMAL_WIDTH);
}

}  //  end ui
}  //  end bach_bot
