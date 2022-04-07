#pragma once

#include <cstdint>
#include <list>
#include <wx/wx.h>
#include <RtMidi.h>

#include "MidiFile.h"

#include "main_window.h"
#include "common_defs.h"


class PlayerThread;



class PlayerWindow : public MainWindow, private wxLog
{
    friend class PlayerThread;
public:
    PlayerWindow();
    virtual ~PlayerWindow() override;

protected:
    virtual void on_play_advance(wxCommandEvent& event) override final;
    virtual void on_stop(wxCommandEvent& event) override final;
    virtual void on_new_playlist(wxCommandEvent& event) override final;
    virtual void on_load_playlist(wxCommandEvent& event) override final;
    virtual void on_save_playlist(wxCommandEvent& event) override final;
    virtual void on_open_midi(wxCommandEvent& event) override final;
    virtual void on_quit(wxCommandEvent& event) override final;
    virtual void on_about(wxCommandEvent& event) override final;
    virtual void on_manual_advance(wxCommandEvent& event) override final;
    virtual void on_manual_prev(wxCommandEvent& event) override final;
    virtual void on_manual_cancel(wxCommandEvent& event) override final;

private:
    void on_thread_tick(wxThreadEvent& event);
    void on_thread_exit(wxThreadEvent& event);
    void on_device_changed(const uint32_t device_id);
    void send_manual_message(const SyndyneBankCommands value);
    void build_syndine_sequence(const smf::MidiEventList &event_list) const;

    uint32_t m_counter;
    PlayerThread *m_player_thread;
    std::list<wxMenuItem> m_midi_devices;
    RtMidiOut m_midi_out;
    uint32_t m_current_device_id;

    wxDECLARE_EVENT_TABLE();

};
