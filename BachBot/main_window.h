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
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/statusbr.h>
#include <wx/menu.h>
#include <wx/frame.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class MainWindow
///////////////////////////////////////////////////////////////////////////////
class MainWindow : public wxFrame
{
	private:

	protected:
		wxPanel* m_panel1;
		wxButton* play_advance_button;
		wxPanel* m_panel4;
		wxStaticText* m_staticText3;
		wxStaticText* m_staticText4;
		wxStaticText* m_staticText5;
		wxStaticText* bank_label;
		wxStaticText* m_staticText7;
		wxStaticText* mode_label;
		wxButton* stop_button;
		wxStaticText* track_label;
		wxPanel* m_panel2;
		wxStaticText* playlist_label;
		wxStatusBar* m_statusBar1;
		wxMenuBar* m_menubar1;
		wxMenu* m_menu1;
		wxMenu* m_menu2;

		// Virtual event handlers, override them in your derived class
		virtual void on_play_advance( wxCommandEvent& event ) { event.Skip(); }
		virtual void on_stop( wxCommandEvent& event ) { event.Skip(); }
		virtual void on_new_playlist( wxCommandEvent& event ) { event.Skip(); }
		virtual void on_load_playlist( wxCommandEvent& event ) { event.Skip(); }
		virtual void on_save_playlist( wxCommandEvent& event ) { event.Skip(); }
		virtual void on_open_midi( wxCommandEvent& event ) { event.Skip(); }
		virtual void on_quit( wxCommandEvent& event ) { event.Skip(); }
		virtual void on_about( wxCommandEvent& event ) { event.Skip(); }


	public:

		MainWindow( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Main Window"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,305 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );

		~MainWindow();

};

