//  system includes
#include <cstdint>  //  uint32_t
#include <list>  //  std::list
#include <memory>  //  std::shared_ptr
#include <unordered_map>  //  std::unordered_map
#include <optional>  //  std::optional
#include <wx/wx.h>  //  wxString

//  module includes
// -none-

//  local includes
#include "playlist_loader.h"  //  local include

namespace bach_bot {
namespace ui {


PlaylistLoader::PlaylistLoader(wxFrame *const parent,
                               PlayList &playlist,
                               const wxXmlNode *const p_playlist) :
    LoadingPopup(parent),
    wxThread(wxTHREAD_JOINABLE),
    m_mutex(),
    m_playlist{playlist},
    m_error_text(),
    m_playlist_root{p_playlist}
{
    progress_bar->SetRange(10);
}


int PlaylistLoader::ShowModal()
{
    Run();
    const auto retval = LoadingPopup::ShowModal();
    Wait();
    return retval;
}


wxThread::ExitCode PlaylistLoader::Entry()
{
    wxMutexLocker lock(m_mutex);

    for (auto i = 0U; i < 10; ++i) {
        wxThreadEvent tick_event(wxEVT_THREAD, LoaderEvents::TICK_EVENT);
        tick_event.SetInt(i);
        wxQueueEvent(this, tick_event.Clone());
        wxSleep(1);
    }


    wxThreadEvent exit_event(wxEVT_THREAD, LoaderEvents::EXIT_EVENT);
    exit_event.SetInt(wxID_OK);
    wxQueueEvent(this, exit_event.Clone());

    return nullptr;
}


void PlaylistLoader::on_tick_event(wxThreadEvent &event)
{
    progress_bar->SetValue(event.GetInt());
}


void PlaylistLoader::on_close_event(wxThreadEvent &event)
{
    EndModal(event.GetInt());
}


wxBEGIN_EVENT_TABLE(PlaylistLoader, wxDialog)
    EVT_THREAD(LoaderEvents::TICK_EVENT, PlaylistLoader::on_tick_event)
    EVT_THREAD(LoaderEvents::EXIT_EVENT, PlaylistLoader::on_close_event)
wxEND_EVENT_TABLE()

}  //  end ui
}  // end bach_bot
