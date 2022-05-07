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
#include <cstdint>  //  uint32_t
#include <list>  //  std::list
#include <memory>  //  std::shared_ptr
#include <unordered_map>  //  std::unordered_map
#include <optional>  //  std::optional
#include <stdexcept>  //  std::out_of_range
#include <memory>  //  std::make_unique
#include <limits>  //  std::numeric_limits
#include <wx/wx.h>  //  wxString
#include <fmt/format.h>  //  fmt::format

//  module includes
// -none-

//  local includes
#include "playlist_loader.h"  //  local include
#include "syndyne_importer.h"  //  SyndineImporter

namespace bach_bot {
namespace ui {


PlaylistLoader::PlaylistLoader(wxFrame *const parent,
                               PlayList &playlist,
                               const wxString &filename) :
    LoadingPopup(parent),
    wxThread(wxTHREAD_JOINABLE),
    m_mutex(),
    m_playlist{playlist},
    m_error_text(),
    m_playlist_doc(),
    m_filename{filename},
    m_entries()
{
}


int PlaylistLoader::ShowModal()
{
    Run();
    const auto retval = LoadingPopup::ShowModal();
    Wait();
    return retval;
}


const wxString& PlaylistLoader::get_error_text() const
{
    return m_error_text;
}


wxString PlaylistLoader::get_error_text()
{
    wxMutexLocker lock(m_mutex);
    return m_error_text;
}


wxThread::ExitCode PlaylistLoader::Entry()
{
    wxMutexLocker lock(m_mutex);

    if (!m_playlist_doc.Load(m_filename)) {
        m_error_text = wxT("Invalid file format");
    } else if (count_children(m_playlist_doc.GetRoot()) <= 0) {
        m_error_text = wxT("Invalid file contents");
    } else {
        try {
            parse_playlist(m_playlist_doc.GetRoot());
        } catch (std::out_of_range &e) {
            m_error_text = e.what();
        }
    }

    wxThreadEvent exit_event(wxEVT_THREAD, LoaderEvents::EXIT_EVENT);
    if (m_error_text.size() > 0U) {
        exit_event.SetInt(-1);
        m_playlist.clear();
    } else {
        exit_event.SetInt(wxID_OK);
    }
    wxQueueEvent(this, exit_event.Clone());

    return nullptr;
}


void PlaylistLoader::parse_playlist(const wxXmlNode *const playlist_root)
{
    m_playlist.clear();

    if (playlist_root->GetName() != L"BachBot_Playlist") {
        throw std::out_of_range("File format not recognized.");
    }
    
    auto child_num = 0;
    auto prev_song_id = 0U;
    
    for (const auto &i: m_entries) {
        const auto *child = i.second;
        if (child->GetType() != wxXML_ELEMENT_NODE || 
            child->GetName() != L"song") {
                continue;
        }

        auto lock = m_playlist.lock();
        auto &song_entry = m_playlist.get_playlist_entry();
        if (!load_playlist_config(song_entry, child)) {
            throw std::out_of_range(fmt::format("Invalid song data line {}", 
                                                child->GetLineNumber()));
        }

        if (!import_midi(song_entry)) {
            throw std::out_of_range(fmt::format("Unable to import song: {}", 
                                                song_entry.file_name.ToStdString()));
        }

        if (prev_song_id > 0U) {
            auto &prev_song = m_playlist.get_playlist_entry(prev_song_id);
            prev_song.next_song_id = song_entry.current_song_id;
        }
        prev_song_id = song_entry.current_song_id;
        child = child->GetNext();

        wxThreadEvent tick_event(wxEVT_THREAD, LoaderEvents::TICK_EVENT);
        tick_event.SetInt(child_num);
        wxQueueEvent(this, tick_event.Clone());
    }
}


void PlaylistLoader::on_start_event(wxThreadEvent &event)
{
    progress_bar->SetRange(event.GetInt());
}


void PlaylistLoader::on_tick_event(wxThreadEvent &event)
{
    progress_bar->SetValue(event.GetInt());
}


void PlaylistLoader::on_close_event(wxThreadEvent &event)
{
    EndModal(event.GetInt());
}


int PlaylistLoader::count_children(const wxXmlNode *const playlist_root)
{
    const auto *child = playlist_root->GetChildren();
    while (nullptr != child) {
        if (child->GetName() == L"song" && 
                child->GetType() == wxXML_ELEMENT_NODE)
        {
            const auto &order_text = child->GetAttribute(wxT("order"));
            auto order = std::numeric_limits<long>::min();
            const auto ok = order_text.ToCLong(&order);
            if (!ok || (order < 1)) {
                return -1;
            }

            m_entries.push_back({uint32_t(order), child});
        }
        child = child->GetNext();
    }
    wxThreadEvent start_event(wxEVT_THREAD, LoaderEvents::TICK_EVENT);
    start_event.SetInt(m_entries.size());

    std::sort(m_entries.begin(), m_entries.end(), [](const SongNode &a,
                                                     const SongNode &b) {
        return a.first < b.first;
    });
    return int(m_entries.size());
}


bool PlaylistLoader::load_playlist_config(
    PlayListEntry &song_entry, const wxXmlNode *const playlist_node) const
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

    song_entry.file_name = playlist_node->GetNodeContent();
    if (song_entry.file_name.length() == 0U) {
        throw std::out_of_range(fmt::format("Invalid filename line {}", 
                                            playlist_node->GetLineNumber()));
    }

    if (playlist_node->HasAttribute(wxT("tempo_requested"))) {
        const auto desired = playlist_node->GetAttribute(wxT("tempo_requested"));

        test_int(song_entry.tempo_requested, desired, 1, 1000);
    }

    test_double(song_entry.gap_beats, playlist_node->GetAttribute(wxT("gap")));
    if (song_entry.gap_beats < 0.0) {
        valid = false;
    }

    int start_bank;
    int start_mode;
    test_int(start_bank, playlist_node->GetAttribute(wxT("start_bank")), 1, 8);
    test_int(start_mode, playlist_node->GetAttribute(wxT("start_mode")), 1, 100);
    if (valid) {
        song_entry.starting_config = std::make_pair(uint8_t(start_bank - 1), 
                                                    uint32_t(start_mode - 1));
    }

    test_int(song_entry.delta_pitch,
             playlist_node->GetAttribute(wxT("pitch")),
             -MIDI_NOTES_IN_OCTAVE, MIDI_NOTES_IN_OCTAVE);

    int play_next;
    test_int(play_next,
             playlist_node->GetAttribute(wxT("auto_play_next")),
             std::numeric_limits<int>::min(),
             std::numeric_limits<int>::max());
    if (valid) {
        song_entry.play_next = (0U != play_next);
    }

    test_double(song_entry.last_note_multiplier,
                playlist_node->GetAttribute(wxT("last_note_multiplier")));

    return (valid && (song_entry.last_note_multiplier > 0.0));
}


bool PlaylistLoader::import_midi(PlayListEntry &song_entry) const
{
    //  Keep large structure off of stack
    auto importer = std::make_unique<SyndineImporter>(
        song_entry.file_name.ToStdString(), song_entry.current_song_id);
    song_entry.tempo_detected = importer->get_tempo();

    importer->set_bank_config(song_entry.starting_config.first,
                              song_entry.starting_config.second);

    importer->adjust_tempo(song_entry.tempo_requested);
    importer->adjust_key(song_entry.delta_pitch);

    song_entry.midi_events = importer->get_events(
        song_entry.gap_beats, song_entry.last_note_multiplier);

    return (song_entry.midi_events.size() > 0U);
}


wxBEGIN_EVENT_TABLE(PlaylistLoader, wxDialog)
    EVT_THREAD(LoaderEvents::START_EVENT, PlaylistLoader::on_start_event)
    EVT_THREAD(LoaderEvents::TICK_EVENT, PlaylistLoader::on_tick_event)
    EVT_THREAD(LoaderEvents::EXIT_EVENT, PlaylistLoader::on_close_event)
wxEND_EVENT_TABLE()

}  //  end ui
}  // end bach_bot
