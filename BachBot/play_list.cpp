/**
 * @file play_list.cpp
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
 */


//  system includes
#include <stdexcept>  //  std::out_of_range
#include <fmt/format.h>  //  fmt::format
#include <fmt/xchar.h>  //  fmt::format(L
#include <memory>  //  std::make_unique

//  module includes
// -none-

//  local includes
#include "play_list.h"  //  local include


namespace bach_bot {

bool PlayListEntry::import_midi(SyndineImporter *importer)
{
    //  Keep large structure off of stack
    std::unique_ptr<SyndineImporter> local_importer;
    if (nullptr == importer) {
        local_importer = std::make_unique<SyndineImporter>(
            file_name.ToStdString(), song_id);
        importer = local_importer.get();
    }
    tempo_detected = importer->get_tempo();
    importer->set_bank_config(starting_config.first,
                              starting_config.second);

    if (tempo_requested > 0) {
        importer->adjust_tempo(tempo_requested);
    }
    importer->adjust_key(delta_pitch);

    midi_events = importer->get_events(gap_beats, last_note_multiplier);

    return (midi_events.size() > 0U);
}


bool PlayListEntry::load_config(const wxXmlNode *const playlist_node)
{
    auto valid = true;
    auto test_double = [&](double &dest, const wxString &str) {
        auto test_value = std::numeric_limits<double>::max();
        const auto ok = str.ToDouble(&test_value);
        if (ok) {
            dest = test_value;
        } else {
            valid = false;
        }
    };

    auto test_int = [&](int &dest, 
                        const wxString &str,
                        const int min,
                        const int max) {
        auto test_value = std::numeric_limits<long>::max();
        const auto ok = str.ToCLong(&test_value);
        if (ok && (test_value >= long(min)) && (test_value <= long(max))) {
            dest = int(test_value);
        } else {
            valid = false;
        }

    };

    file_name = playlist_node->GetNodeContent();
    if (file_name.length() == 0U) {
        throw std::out_of_range(fmt::format("Invalid filename line {}", 
                                            playlist_node->GetLineNumber()));
    }

    if (playlist_node->HasAttribute(wxT("tempo_requested"))) {
        const auto desired = playlist_node->GetAttribute(wxT("tempo_requested"));

        test_int(tempo_requested, desired, 1, 1000);
    }

    test_double(gap_beats, playlist_node->GetAttribute(wxT("gap")));
    if (gap_beats < 0.0) {
        valid = false;
    }

    int start_bank;
    int start_mode;
    test_int(start_bank, playlist_node->GetAttribute(wxT("start_bank")), 1, 8);
    test_int(start_mode, playlist_node->GetAttribute(wxT("start_mode")), 1, 100);
    if (valid) {
        starting_config = std::make_pair(uint8_t(start_bank - 1), 
                                                    uint32_t(start_mode - 1));
    }

    test_int(delta_pitch,
             playlist_node->GetAttribute(wxT("pitch")),
             -MIDI_NOTES_IN_OCTAVE, MIDI_NOTES_IN_OCTAVE);

    int autoplay;
    test_int(autoplay,
             playlist_node->GetAttribute(wxT("auto_play_next")),
             std::numeric_limits<int>::min(),
             std::numeric_limits<int>::max());
    if (valid) {
        play_next = (0U != autoplay);
    }

    test_double(last_note_multiplier,
                playlist_node->GetAttribute(wxT("last_note_multiplier")));

    return (valid && (last_note_multiplier > 0.0));
}


std::optional<wxString> PlayListEntry::load_config(
    const ui::LoadMidiDialog &dialog)
{
    auto test = [](double &dest, wxTextCtrl *const box) -> bool {
        auto test_value = std::numeric_limits<double>::max();
        const auto ok = box->GetValue().ToDouble(&test_value);
        if (ok) {
            dest = test_value;
        }

        return ok;
    };

    if (!test(last_note_multiplier,
              dialog.extend_ending_textbox)) {
        return fmt::format(L"Error in field: {}",
                           dialog.extended_ending_label->GetLabelText());
    }

    if (!test(gap_beats, dialog.initial_gap_text_box)) {
        return fmt::format(L"Error in field: {}",
                           dialog.initial_gap_label->GetLabelText());
    }

    tempo_requested = dialog.select_tempo->GetValue();
    starting_config = std::make_pair(
        uint8_t(dialog.bank_select->GetValue() - 1), 
        uint32_t(dialog.mode_select->GetValue() - 1));
    delta_pitch = dialog.pitch_change->GetValue();
    play_next = dialog.play_next_checkbox->IsChecked();

    return {};
}


void PlayListEntry::save_config(wxXmlNode *const playlist_node) const
{
    if (tempo_detected.has_value()) {
        const auto tempo = (tempo_requested > 0 ?
                            tempo_requested : tempo_detected.value());
        playlist_node->AddAttribute(wxT("tempo_requested"),
                                    wxString::Format(wxT("%i"), tempo));
    }
    playlist_node->AddAttribute(
        wxT("gap"), wxString::FromDouble(gap_beats));

    playlist_node->AddAttribute(
        wxT("start_bank"),
        wxString::Format(wxT("%d"), starting_config.first + 1U));
    playlist_node->AddAttribute(
        wxT("start_mode"),
        wxString::Format(wxT("%d"), starting_config.second + 1U));

    playlist_node->AddAttribute(
        wxT("pitch"),
        wxString::Format(wxT("%i"), delta_pitch));
    playlist_node->AddAttribute(
        wxT("last_note_multiplier"), 
        wxString::FromDouble(last_note_multiplier));
    playlist_node->AddAttribute(
        wxT("auto_play_next"),
        wxString::Format(wxT("%i"), int(play_next)));
    playlist_node->AddChild(new wxXmlNode(wxXML_TEXT_NODE, 
                                           wxT(""), 
                                           file_name));
}


void PlayListEntry::populate_dialog(ui::LoadMidiDialog &dialog) const
{
    dialog.file_name_label->SetLabelText(file_name);
    if (tempo_detected.has_value()) {
        dialog.tempo_label->SetLabelText(
            fmt::format(L"{}bpm", tempo_detected.value()));
        dialog.select_tempo->SetValue(
            (tempo_requested > 0 ? tempo_requested : tempo_detected.value()));
    } else {
        dialog.tempo_label->SetLabelText(wxT("Tempo not reported"));
        static_cast<void>(dialog.select_tempo->Enable(false));
    }

    dialog.initial_gap_text_box->SetValue(
        wxString::FromDouble(gap_beats));
    dialog.bank_select->SetValue(
        int(starting_config.first) + 1);
    dialog.mode_select->SetValue(
        int(starting_config.first) + 1);
    dialog.pitch_change->SetValue(delta_pitch);
    dialog.extend_ending_textbox->SetValue(
        wxString::FromDouble(last_note_multiplier));
    dialog.play_next_checkbox->SetValue(play_next);
}

}
