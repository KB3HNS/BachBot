
#include <chrono>
#include <algorithm>
#include <array>
#include <fmt/xchar.h>
#include <wx/wx.h>

#include "player_window.h"
#include "player_thread.h"


namespace {
    std::list<OrganMidiEvent> file_events;

    struct MidiTracking
    {
        bool on_now;
        uint8_t node_number;
        uint8_t channel_number;

        uint32_t midi_ticks_on_time;
        uint32_t last_midi_off_time;

        OrganMidiEvent *note_on;
        OrganMidiEvent *note_off;
    };
}


PlayerWindow::PlayerWindow() :
    MainWindow(nullptr),
    wxLog(),
    m_counter{0U},
    m_player_thread{nullptr},
    m_midi_devices(),
    m_midi_out(),
    m_current_device_id{0U}
{
    for (auto i = 0U; i < m_midi_out.getPortCount(); ++i) {
        m_midi_devices.emplace_back(
            device_select, wxID_ANY, wxString(m_midi_out.getPortName(i)),
            wxEmptyString, wxITEM_RADIO
        );
        device_select->Append(&m_midi_devices.back());
        device_select->Bind(wxEVT_COMMAND_MENU_SELECTED, 
                            [=](wxCommandEvent&) { on_device_changed(i); },
                            m_midi_devices.back().GetId()
        );
    }
    m_midi_devices.front().Check();
}


void PlayerWindow::on_play_advance(wxCommandEvent &event)
{
    if (nullptr == m_player_thread) {
        event_count->SetValue(0);
        event_count->SetRange(int(file_events.size()));
        m_player_thread = new PlayerThread(this, m_current_device_id);
        m_player_thread->play(file_events);
        std::for_each(m_midi_devices.begin(), m_midi_devices.end(), 
                      [](wxMenuItem &i) { i.Enable(false); });
    } else {
        m_player_thread->signal_advance();
    }
}


void PlayerWindow::on_stop(wxCommandEvent &event)
{
    if (nullptr != m_player_thread) {
        m_player_thread->signal_stop();
        m_player_thread->Wait();
    }
}


void PlayerWindow::on_new_playlist(wxCommandEvent &event)
{
    wxMessageBox("New Playlist",
                 "create new playlist", wxOK | wxICON_INFORMATION);
}


void PlayerWindow::on_load_playlist(wxCommandEvent &event)
{
    wxMessageBox("Load Playlist",
                 "load playlist", wxOK | wxICON_INFORMATION);
}


void PlayerWindow::on_save_playlist(wxCommandEvent &event)
{
    wxMessageBox("Save Playlist",
                 "Save current playlist", wxOK | wxICON_INFORMATION);
}


void PlayerWindow::on_open_midi(wxCommandEvent &event)
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
    build_syndine_sequence(midifile[0]);
}


void PlayerWindow::on_quit(wxCommandEvent &event)
{
    Close(true);
}


void PlayerWindow::on_about(wxCommandEvent &event)
{
    wxMessageBox("This is a wxWidgets' Hello world sample",
                 "About Hello World", wxOK | wxICON_INFORMATION);
}


void PlayerWindow::on_thread_tick(wxThreadEvent &event)
{
    ++m_counter;
    const auto current_setting = uint32_t(event.GetInt());
    const auto bank = current_setting % 8U;
    const auto mode = current_setting / 8U;
    bank_label->SetLabel(wxString(fmt::format(L"{}", bank + 1U)));
    mode_label->SetLabel(wxString(fmt::format(L"{}", mode + 1U)));
    const auto events_complete = file_events.size() - 
                                 m_player_thread->get_events_remaining();
    event_count->SetValue(int(events_complete));
}


void PlayerWindow::on_thread_exit(wxThreadEvent &event)
{
    if (nullptr != m_player_thread) {
        delete m_player_thread;
        m_player_thread = nullptr;
    }

    event_count->SetValue(0);
    std::for_each(m_midi_devices.begin(), m_midi_devices.end(),
                  [](wxMenuItem& i) { i.Enable(); });
}


void PlayerWindow::on_device_changed(const uint32_t device_id)
{
    m_current_device_id = device_id;
}


void PlayerWindow::on_manual_advance(wxCommandEvent &event)
{
    send_manual_message(SyndyneBankCommands::NEXT_BANK);
}


void PlayerWindow::on_manual_prev(wxCommandEvent &event)
{
    send_manual_message(SyndyneBankCommands::PREV_BANK);
}


void PlayerWindow::on_manual_cancel(wxCommandEvent &event)
{
    send_manual_message(SyndyneBankCommands::GENERAL_CANCEL);
}


void PlayerWindow::send_manual_message(const SyndyneBankCommands value)
{
    std::array<uint8_t, MIDI_MESSAGE_SIZE> midi_message;
    midi_message[0] = make_midi_command_byte(0U, MidiCommands::CONTROL_CHANGE);
    midi_message[1] = SYNDINE_CONTROLLER_ID;
    midi_message[2] = value;

    const auto port_open = m_midi_out.isPortOpen();
    if (!port_open) {
        m_midi_out.openPort(m_current_device_id);
    }

    m_midi_out.sendMessage(midi_message.data(), MIDI_MESSAGE_SIZE);

    if (!port_open) {
        m_midi_out.closePort();
    }
}


void PlayerWindow::build_syndine_sequence(const smf::MidiEventList& event_list) const
{
    std::list<OrganMidiEvent> events;
    auto current_tick = 0;
    auto current_time = 0.0;
    for (auto i = 0; i < event_list.size(); ++i) {
        OrganMidiEvent ev;
        const auto& midi_event = event_list[i];
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


PlayerWindow::~PlayerWindow()
{
    wxCommandEvent e;
    on_stop(e);
    std::for_each(m_midi_devices.begin(), m_midi_devices.end(), 
                  [=](wxMenuItem &i) { device_select->Remove(&i); });
}


wxBEGIN_EVENT_TABLE(PlayerWindow, wxFrame)
    EVT_THREAD(PlayerEvents::TICK_EVENT, PlayerWindow::on_thread_tick)
    EVT_THREAD(PlayerEvents::EXIT_EVENT, PlayerWindow::on_thread_exit)
wxEND_EVENT_TABLE()
