
#include <fmt/xchar.h>
#include <wx/wx.h>
#include <chrono>

#include "MidiFile.h"

#include "player_window.h"


namespace {
    std::list<OrganMidiEvent> file_events;
}


PlayerWindow::PlayerWindow() :
    MainWindow(nullptr),
    wxLog(),
    m_counter{0U},
    m_player_thread{nullptr}
{
}


void PlayerWindow::on_play_advance(wxCommandEvent& event)
{
    if (nullptr == m_player_thread) {
        m_player_thread = new PlayerThread(this);
        m_player_thread->play(file_events);
    } else {
        m_player_thread->signal_advance();
    }
}


void PlayerWindow::on_stop(wxCommandEvent& event)
{
    if (nullptr != m_player_thread) {
        m_player_thread->signal_stop();
    }
}


void PlayerWindow::on_new_playlist(wxCommandEvent& event)
{
    wxMessageBox("New Playlist",
        "create new playlist", wxOK | wxICON_INFORMATION);
}


void PlayerWindow::on_load_playlist(wxCommandEvent& event)
{
    wxMessageBox("Load Playlist",
        "load playlist", wxOK | wxICON_INFORMATION);
}


void PlayerWindow::on_save_playlist(wxCommandEvent& event)
{
    wxMessageBox("Save Playlist",
        "Save current playlist", wxOK | wxICON_INFORMATION);
}


void PlayerWindow::on_open_midi(wxCommandEvent& event)
{
    wxFileDialog open_dialog(this, "Open MIDI File", "", "", 
                             "MIDI Files|*.mid", 
                             wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (open_dialog.ShowModal() == wxID_CANCEL) {
        return;
    }

    smf::MidiFile midifile;
    midifile.read(open_dialog.GetPath().ToStdString());
    midifile.doTimeAnalysis();
    midifile.linkNotePairs();
    midifile.joinTracks();

    std::list<OrganMidiEvent> events;
    auto current_tick = 0;
    auto current_time = 0.0;
    for (auto i = 0; i < midifile[0].size(); ++i) {
        OrganMidiEvent ev;
        const auto &midi_event = midifile[0][i];
        ev.event_code = uint8_t(midi_event[0]);
        if (midi_event.size() > 1) {
            ev.byte1 = uint8_t(midi_event[1]);
            if (midi_event.size() > 2) {
                ev.byte2 = uint8_t(midi_event[2]);
            }
        }

        ev.delay = midi_event.tick;
        ev.delta = ev.delay - current_tick;
        current_tick = ev.delay;

        ev.seconds = midi_event.seconds;
        ev.delta_time = ev.seconds - current_time;
        current_time = ev.seconds;
        events.push_back(ev);
    }

    file_events = std::move(events);
}


void PlayerWindow::on_quit(wxCommandEvent& event)
{
    Close(true);
}


void PlayerWindow::on_about(wxCommandEvent& event)
{
    wxMessageBox("This is a wxWidgets' Hello world sample",
                 "About Hello World", wxOK | wxICON_INFORMATION);
}


wxBEGIN_EVENT_TABLE(PlayerWindow, wxFrame)
    EVT_THREAD(PlayerEvents::TICK_EVENT, PlayerWindow::on_thread_tick)
    EVT_THREAD(PlayerEvents::EXIT_EVENT, PlayerWindow::on_thread_exit)
wxEND_EVENT_TABLE()


void PlayerWindow::on_thread_tick(wxThreadEvent& event)
{
    ++m_counter;
    wxString label_text(fmt::format(L"Song {}", event.GetInt()));
    track_label->SetLabel(label_text);
}


void PlayerWindow::on_thread_exit(wxThreadEvent& event)
{
    delete m_player_thread;
    m_player_thread = nullptr;
}
