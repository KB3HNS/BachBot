/**
 * @file thread_loader.cpp
 * @brief  Background loading in another thread
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
#include <stdexcept>  //  std::runtime_error

//  module includes
// -none-

//  local includes
#include "thread_loader.h"  //  local include
#include "playlist_entry_control.h"  //  set_label_filename

namespace {
    constexpr const size_t MAX_FILENAME_LEN = 58U;
}

namespace bach_bot {
namespace ui {
using namespace std::placeholders;


ThreadLoader::ThreadLoader(wxFrame *const parent) :
    LoadingPopup(parent),
    wxThread(wxTHREAD_JOINABLE),
    m_mutex(),
    m_playlist(),
    m_error_text(),
    m_count{0U},
    m_last_progress_len{MAX_FILENAME_LEN},
    m_success_callback{std::bind(&ThreadLoader::dummy_callback, this, _1)}
{
}


int ThreadLoader::ShowModal()
{
    Run();
    const auto retval = LoadingPopup::ShowModal();
    Wait();
    return retval;
}


std::optional<wxString> ThreadLoader::get_error_text()
{
    wxMutexLocker lock(m_mutex);
    return m_error_text;
}


void ThreadLoader::set_on_success_callback(SuccessCallback callback)
{
    m_success_callback = callback;
}


wxThread::ExitCode ThreadLoader::Entry()
{
    wxMutexLocker lock(m_mutex);
    m_playlist.clear();
    m_error_text.reset();

    const auto count = count_children();
    wxThreadEvent start_event(wxEVT_THREAD, LoaderEvents::START_EVENT);
    start_event.SetInt(count);
    wxQueueEvent(this, start_event.Clone());

    if (count > 0) {
        m_count = uint32_t(count);
        parse_playlist();
    } else if (!m_error_text.has_value()) {
        m_error_text = wxT("Invalid file contents");
    }

    wxThreadEvent exit_event(wxEVT_THREAD, LoaderEvents::EXIT_EVENT);
    if (m_error_text.has_value()) {
        exit_event.SetInt(-1);
        m_playlist.clear();
    } else {
        exit_event.SetInt(wxID_OK);
    }
    wxQueueEvent(this, exit_event.Clone());

    return nullptr;
}


void ThreadLoader::set_error_text(const wxString &error)
{
    if (!m_error_text.has_value()) {
        m_error_text = error;
    }
}

void ThreadLoader::on_start_event(wxThreadEvent &event)
{
    const auto value = event.GetInt();
    if (value > 0) {
        progress_bar->SetRange(value);
    }
}


void ThreadLoader::on_tick_event(wxThreadEvent &event)
{
    const auto value = event.GetInt();
    if (value > 0 && value <= progress_bar->GetRange()) {
        progress_bar->SetValue(value);
    }

    const auto progress_text = wxString::Format("%i/%i",
                                                value,
                                                progress_bar->GetRange());
    progress_label->SetLabelText(progress_text);
    const auto label_len = progress_text.Length();
    //  The first entry will have a *very* brief glitch here.  I can't seem to
    // avoid it.
    if (label_len != m_last_progress_len) {
        m_last_progress_len = label_len;
        set_label_filename(filename_label,
                           filename_label->GetLabelText(),
                           MAX_FILENAME_LEN - label_len);
        Layout();
    }
}


void ThreadLoader::on_close_event(wxThreadEvent &event)
{
    filename_label->SetLabelText(wxT("Finishing..."));

    const auto retval = event.GetInt();
    if (wxID_OK == retval) {
        m_success_callback(std::move(m_playlist));
    }

    EndModal(retval);
}


void ThreadLoader::on_filename_event(wxThreadEvent &event)
{
    const auto string_len = MAX_FILENAME_LEN -
                            progress_label->GetLabelText().Length();
    set_label_filename(filename_label, event.GetString(), string_len);
}


void ThreadLoader::parse_playlist()
{
    for (auto song_id = 1U; song_id <= m_count; ++song_id) {
        PlayListEntry song_entry;
        song_entry.song_id = song_id;
        build_playlist_entry(song_entry, song_id);

        if (m_error_text.has_value()) {
            break;
        }

        wxThreadEvent file_event(wxEVT_THREAD, LoaderEvents::SET_FILENAME_EVENT);
        file_event.SetInt(int(song_entry.file_name.length()));
        file_event.SetString(song_entry.file_name);
        wxQueueEvent(this, file_event.Clone());

        if (!song_entry.import_midi()) {
            set_error_text(fmt::format(L"Unable to import song: {}", 
                                       song_entry.file_name));
        } else {
            m_playlist.push_back(std::move(song_entry));
            wxThreadEvent tick_event(wxEVT_THREAD, LoaderEvents::TICK_EVENT);
            tick_event.SetInt(int(song_id));
            wxQueueEvent(this, tick_event.Clone());
        }
    }
}


void ThreadLoader::dummy_callback(std::list<PlayListEntry>)
{
    throw std::runtime_error("Error: playlist loader callback not specified!");
}


wxBEGIN_EVENT_TABLE(ThreadLoader, wxDialog)
    EVT_THREAD(LoaderEvents::START_EVENT, ThreadLoader::on_start_event)
    EVT_THREAD(LoaderEvents::SET_FILENAME_EVENT, ThreadLoader::on_filename_event)
    EVT_THREAD(LoaderEvents::TICK_EVENT, ThreadLoader::on_tick_event)
    EVT_THREAD(LoaderEvents::EXIT_EVENT, ThreadLoader::on_close_event)
wxEND_EVENT_TABLE()


}  //  end ui
}  // end bach_bot
