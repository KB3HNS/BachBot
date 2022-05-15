///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-a87998d)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/button.h>
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statbox.h>
#include <wx/gauge.h>
#include <wx/statline.h>
#include <wx/scrolwin.h>
#include <wx/statusbr.h>
#include <wx/menu.h>
#include <wx/frame.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>
#include <wx/radiobut.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

namespace bach_bot
{
	namespace ui
	{
		///////////////////////////////////////////////////////////////////////////////
		/// Class MainWindow
		///////////////////////////////////////////////////////////////////////////////
		class MainWindow : public wxFrame
		{
			private:

			protected:
				wxButton* play_advance_button;
				wxButton* stop_button;
				wxStaticText* m_staticText3;
				wxStaticText* m_staticText4;
				wxStaticText* m_staticText5;
				wxStaticText* bank_label;
				wxStaticText* m_staticText7;
				wxStaticText* mode_label;
				wxGauge* event_count;
				wxStaticText* track_label;
				wxStaticBoxSizer* sbSizer1;
				wxStaticText* next_label;
				wxScrolledWindow* playlist_panel;
				wxBoxSizer* playlist_container;
				wxBoxSizer* header_container;
				wxStaticText* m_staticText25;
				wxStaticText* m_staticText26;
				wxStaticText* m_staticText27;
				wxStaticText* playlist_label;
				wxStaticLine* m_staticline1;
				wxStatusBar* m_statusBar1;
				wxMenuBar* m_menubar1;
				wxMenu* m_menu1;
				wxMenu* device_select;
				wxMenu* m_menu4;
				wxMenu* m_menu2;

				// Virtual event handlers, override them in your derived class
				virtual void on_close( wxCloseEvent& event ) { event.Skip(); }
				virtual void on_drop_midi_file( wxDropFilesEvent& event ) { event.Skip(); }
				virtual void on_keydown_event( wxKeyEvent& event ) { event.Skip(); }
				virtual void on_keyup_event( wxKeyEvent& event ) { event.Skip(); }
				virtual void on_play_advance( wxCommandEvent& event ) { event.Skip(); }
				virtual void on_stop( wxCommandEvent& event ) { event.Skip(); }
				virtual void on_bank_change_next( wxMouseEvent& event ) { event.Skip(); }
				virtual void on_bank_change_prev( wxMouseEvent& event ) { event.Skip(); }
				virtual void on_new_playlist( wxCommandEvent& event ) { event.Skip(); }
				virtual void on_load_playlist( wxCommandEvent& event ) { event.Skip(); }
				virtual void on_save_playlist( wxCommandEvent& event ) { event.Skip(); }
				virtual void on_save_as( wxCommandEvent& event ) { event.Skip(); }
				virtual void on_open_midi( wxCommandEvent& event ) { event.Skip(); }
				virtual void on_quit( wxCommandEvent& event ) { event.Skip(); }
				virtual void on_manual_advance( wxCommandEvent& event ) { event.Skip(); }
				virtual void on_manual_prev( wxCommandEvent& event ) { event.Skip(); }
				virtual void on_manual_cancel( wxCommandEvent& event ) { event.Skip(); }
				virtual void on_about( wxCommandEvent& event ) { event.Skip(); }


			public:

				MainWindow( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("BachBot Player"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 430,410 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL|wxWANTS_CHARS );

				~MainWindow();

		};

		///////////////////////////////////////////////////////////////////////////////
		/// Class LoadMidiDialog
		///////////////////////////////////////////////////////////////////////////////
		class LoadMidiDialog : public wxDialog
		{
			private:

			protected:
				wxStaticText* m_staticText10;
				wxStaticText* m_staticText14;
				wxStaticText* m_staticText17;
				wxStaticText* m_staticText19;
				wxStaticText* m_staticText20;
				wxStaticText* m_staticText21;
				wxStaticText* m_staticText22;
				wxStaticText* m_staticText15;
				wxStaticText* m_staticText18;

			public:
				wxStaticText* file_name_label;
				wxStaticText* tempo_label;
				wxSpinCtrl* select_tempo;
				wxStaticText* initial_gap_label;
				wxTextCtrl* initial_gap_text_box;
				wxSpinCtrl* bank_select;
				wxSpinCtrl* mode_select;
				wxSpinCtrl* pitch_change;
				wxStaticText* extended_ending_label;
				wxTextCtrl* extend_ending_textbox;
				wxCheckBox* play_next_checkbox;
				wxStdDialogButtonSizer* m_sdbSizer1;
				wxButton* m_sdbSizer1OK;
				wxButton* m_sdbSizer1Cancel;

				LoadMidiDialog( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Import MIDI File"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 400,286 ), long style = wxDEFAULT_DIALOG_STYLE );

				~LoadMidiDialog();

		};

		///////////////////////////////////////////////////////////////////////////////
		/// Class LoadingPopup
		///////////////////////////////////////////////////////////////////////////////
		class LoadingPopup : public wxDialog
		{
			private:

			protected:
				wxStaticText* m_staticText22;
				wxGauge* progress_bar;

			public:

				LoadingPopup( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Loading Playlist..."), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION );

				~LoadingPopup();

		};

		///////////////////////////////////////////////////////////////////////////////
		/// Class PlaylistEntryPanel
		///////////////////////////////////////////////////////////////////////////////
		class PlaylistEntryPanel : public wxPanel
		{
			private:

			protected:
				wxBoxSizer* layout;
				wxRadioButton* now_playing;
				wxStaticText* song_label;
				wxButton* configure_button;
				wxCheckBox* auto_play;
				wxMenu* context_menu;

				// Virtual event handlers, override them in your derived class
				virtual void on_radio_selected( wxCommandEvent& event ) { event.Skip(); }
				virtual void on_configure_clicked( wxCommandEvent& event ) { event.Skip(); }
				virtual void on_checkbox_checked( wxCommandEvent& event ) { event.Skip(); }
				virtual void on_set_next( wxCommandEvent& event ) { event.Skip(); }
				virtual void on_move_up( wxCommandEvent& event ) { event.Skip(); }
				virtual void on_move_down( wxCommandEvent& event ) { event.Skip(); }


			public:

				PlaylistEntryPanel( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 375,43 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

				~PlaylistEntryPanel();

				void PlaylistEntryPanelOnContextMenu( wxMouseEvent &event )
				{
					this->PopupMenu( context_menu, event.GetPosition() );
				}

		};

	} // namespace ui
} // namespace bach_bot

