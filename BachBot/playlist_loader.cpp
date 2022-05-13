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
#include <stdexcept>  //  std::out_of_range
#include <limits>  //  std::numeric_limits
#include <fmt/format.h>  //  fmt::format

//  module includes
// -none-

//  local includes
#include "playlist_loader.h"  //  local include
#include "syndyne_importer.h"  //  SyndineImporter

namespace bach_bot {
namespace ui {


PlaylistLoader::PlaylistLoader(wxFrame *const parent,
                               const wxString &filename) :
    LoadingPopup(parent),
    wxThread(wxTHREAD_JOINABLE),
    m_mutex(),
    m_playlist(),
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


std::list<PlayListEntry> PlaylistLoader::get_playlist()
{
    return std::move(m_playlist);
}


wxThread::ExitCode PlaylistLoader::Entry()
{
    wxMutexLocker lock(m_mutex);
    m_playlist.clear();

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
    
    for (auto i = m_entries.cbegin(); m_entries.cend() != i; ++i) {
        const auto *const child = i->second;

        PlayListEntry song_entry;
        song_entry.song_id = uint32_t(m_playlist.size()) + 1U;
        if (!song_entry.load_config(child)) {
            throw std::out_of_range(fmt::format("Invalid song data line {}", 
                                                child->GetLineNumber()));
        }

        if (!song_entry.import_midi()) {
            throw std::out_of_range(
                fmt::format("Unable to import song: {}", 
                            song_entry.file_name.ToStdString()));
        }

        m_playlist.push_back(std::move(song_entry));
        wxThreadEvent tick_event(wxEVT_THREAD, LoaderEvents::TICK_EVENT);
        tick_event.SetInt(int(std::distance(m_entries.cbegin(), i)));
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


wxBEGIN_EVENT_TABLE(PlaylistLoader, wxDialog)
    EVT_THREAD(LoaderEvents::START_EVENT, PlaylistLoader::on_start_event)
    EVT_THREAD(LoaderEvents::TICK_EVENT, PlaylistLoader::on_tick_event)
    EVT_THREAD(LoaderEvents::EXIT_EVENT, PlaylistLoader::on_close_event)
wxEND_EVENT_TABLE()

}  //  end ui
}  // end bach_bot
