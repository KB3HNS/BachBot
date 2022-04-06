#pragma once

#include <cstdint>

#include "main_window.h"
#include "player_thread.h"

class PlayerWindow : public MainWindow, private wxLog
{
public:
    PlayerWindow();

protected:
    virtual void on_play_advance(wxCommandEvent& event) override final;
    virtual void on_stop(wxCommandEvent& event) override final;
    virtual void on_new_playlist(wxCommandEvent& event) override final;
    virtual void on_load_playlist(wxCommandEvent& event) override final;
    virtual void on_save_playlist(wxCommandEvent& event) override final;
    virtual void on_open_midi(wxCommandEvent& event) override final;
    virtual void on_quit(wxCommandEvent& event) override final;
    virtual void on_about(wxCommandEvent& event) override final;

private:
    void on_thread_tick(wxThreadEvent& event);
    void on_thread_exit(wxThreadEvent& event);
    uint32_t m_counter;
    PlayerThread *m_player_thread;

    wxDECLARE_EVENT_TABLE();

};

