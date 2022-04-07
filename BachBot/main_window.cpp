///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-a87998d)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "main_window.h"

///////////////////////////////////////////////////////////////////////////

MainWindow::MainWindow( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );

	m_panel5 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxSize( -1,75 ), wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 200 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxGridSizer* gSizer1;
	gSizer1 = new wxGridSizer( 2, 1, 0, 0 );

	play_advance_button = new wxButton( m_panel5, wxID_ANY, wxT("Play/Advance"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( play_advance_button, 0, wxALL, 5 );

	stop_button = new wxButton( m_panel5, wxID_ANY, wxT("Stop"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( stop_button, 0, wxALL, 5 );


	fgSizer1->Add( gSizer1, 1, wxALIGN_CENTER, 0 );

	wxGridSizer* gSizer2;
	gSizer2 = new wxGridSizer( 2, 3, 0, 0 );

	m_staticText3 = new wxStaticText( m_panel5, wxID_ANY, wxT("Bank"), wxDefaultPosition, wxSize( 30,-1 ), 0 );
	m_staticText3->Wrap( -1 );
	gSizer2->Add( m_staticText3, 0, wxALL, 5 );

	m_staticText4 = new wxStaticText( m_panel5, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	gSizer2->Add( m_staticText4, 0, wxALL, 5 );

	m_staticText5 = new wxStaticText( m_panel5, wxID_ANY, wxT("Mode"), wxDefaultPosition, wxSize( 30,-1 ), 0 );
	m_staticText5->Wrap( -1 );
	gSizer2->Add( m_staticText5, 0, wxALL, 5 );

	bank_label = new wxStaticText( m_panel5, wxID_ANY, wxT("1"), wxDefaultPosition, wxSize( 30,-1 ), wxALIGN_CENTER_HORIZONTAL );
	bank_label->Wrap( -1 );
	bank_label->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	gSizer2->Add( bank_label, 0, wxALL, 5 );

	m_staticText7 = new wxStaticText( m_panel5, wxID_ANY, wxT("/"), wxDefaultPosition, wxSize( 30,-1 ), wxALIGN_CENTER_HORIZONTAL );
	m_staticText7->Wrap( -1 );
	gSizer2->Add( m_staticText7, 0, wxALL, 5 );

	mode_label = new wxStaticText( m_panel5, wxID_ANY, wxT("1"), wxDefaultPosition, wxSize( 30,-1 ), wxALIGN_CENTER_HORIZONTAL );
	mode_label->Wrap( -1 );
	mode_label->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	gSizer2->Add( mode_label, 0, wxALL, 5 );


	fgSizer1->Add( gSizer2, 1, wxEXPAND, 5 );


	m_panel5->SetSizer( fgSizer1 );
	m_panel5->Layout();
	bSizer2->Add( m_panel5, 1, wxALL, 5 );

	event_count = new wxGauge( this, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL );
	event_count->SetValue( 0 );
	bSizer2->Add( event_count, 0, wxALL|wxEXPAND, 5 );

	track_label = new wxStaticText( this, wxID_ANY, wxT("Not Playing"), wxDefaultPosition, wxDefaultSize, 0 );
	track_label->Wrap( -1 );
	bSizer2->Add( track_label, 0, wxALL, 5 );

	m_panel4 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxSize( -1,500 ), wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );

	playlist_label = new wxStaticText( m_panel4, wxID_ANY, wxT("No Tracks Loaded"), wxDefaultPosition, wxDefaultSize, 0 );
	playlist_label->Wrap( -1 );
	bSizer3->Add( playlist_label, 0, wxALL, 5 );


	m_panel4->SetSizer( bSizer3 );
	m_panel4->Layout();
	bSizer2->Add( m_panel4, 1, wxALL|wxEXPAND|wxFIXED_MINSIZE, 5 );


	this->SetSizer( bSizer2 );
	this->Layout();
	m_statusBar1 = this->CreateStatusBar( 1, wxSTB_SIZEGRIP, wxID_ANY );
	m_menubar1 = new wxMenuBar( 0 );
	m_menu1 = new wxMenu();
	wxMenuItem* m_menuItem6;
	m_menuItem6 = new wxMenuItem( m_menu1, wxID_ANY, wxString( wxT("&New Playlist") ) , wxEmptyString, wxITEM_NORMAL );
	m_menu1->Append( m_menuItem6 );

	wxMenuItem* m_menuItem2;
	m_menuItem2 = new wxMenuItem( m_menu1, wxID_ANY, wxString( wxT("&Load Playlist") ) + wxT('\t') + wxT("Ctrl+O"), wxEmptyString, wxITEM_NORMAL );
	m_menu1->Append( m_menuItem2 );

	wxMenuItem* m_menuItem3;
	m_menuItem3 = new wxMenuItem( m_menu1, wxID_ANY, wxString( wxT("&Save Playlist") ) + wxT('\t') + wxT("Ctrl+S"), wxEmptyString, wxITEM_NORMAL );
	m_menu1->Append( m_menuItem3 );

	wxMenuItem* m_menuItem4;
	m_menuItem4 = new wxMenuItem( m_menu1, wxID_ANY, wxString( wxT("&Open MIDI") ) , wxEmptyString, wxITEM_NORMAL );
	m_menu1->Append( m_menuItem4 );

	m_menu1->AppendSeparator();

	wxMenuItem* m_menuItem5;
	m_menuItem5 = new wxMenuItem( m_menu1, wxID_ANY, wxString( wxT("&Quit") ) , wxEmptyString, wxITEM_NORMAL );
	m_menu1->Append( m_menuItem5 );

	m_menubar1->Append( m_menu1, wxT("&File") );

	device_select = new wxMenu();
	m_menubar1->Append( device_select, wxT("&Device Select") );

	m_menu4 = new wxMenu();
	wxMenuItem* m_menuItem7;
	m_menuItem7 = new wxMenuItem( m_menu4, wxID_ANY, wxString( wxT("&Advance") ) + wxT('\t') + wxT("Alt+="), wxEmptyString, wxITEM_NORMAL );
	m_menu4->Append( m_menuItem7 );

	wxMenuItem* m_menuItem8;
	m_menuItem8 = new wxMenuItem( m_menu4, wxID_ANY, wxString( wxT("&Prev") ) + wxT('\t') + wxT("Alt+-"), wxEmptyString, wxITEM_NORMAL );
	m_menu4->Append( m_menuItem8 );

	wxMenuItem* m_menuItem9;
	m_menuItem9 = new wxMenuItem( m_menu4, wxID_ANY, wxString( wxT("&Cancel") ) + wxT('\t') + wxT("Alt+0"), wxEmptyString, wxITEM_NORMAL );
	m_menu4->Append( m_menuItem9 );

	m_menubar1->Append( m_menu4, wxT("&Manual") );

	m_menu2 = new wxMenu();
	wxMenuItem* m_menuItem1;
	m_menuItem1 = new wxMenuItem( m_menu2, wxID_ANY, wxString( wxT("&About") ) + wxT('\t') + wxT("F1"), wxEmptyString, wxITEM_NORMAL );
	m_menu2->Append( m_menuItem1 );

	m_menubar1->Append( m_menu2, wxT("&Help") );

	this->SetMenuBar( m_menubar1 );


	this->Centre( wxBOTH );

	// Connect Events
	play_advance_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainWindow::on_play_advance ), NULL, this );
	stop_button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainWindow::on_stop ), NULL, this );
	m_menu1->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainWindow::on_new_playlist ), this, m_menuItem6->GetId());
	m_menu1->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainWindow::on_load_playlist ), this, m_menuItem2->GetId());
	m_menu1->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainWindow::on_save_playlist ), this, m_menuItem3->GetId());
	m_menu1->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainWindow::on_open_midi ), this, m_menuItem4->GetId());
	m_menu1->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainWindow::on_quit ), this, m_menuItem5->GetId());
	m_menu4->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainWindow::on_manual_advance ), this, m_menuItem7->GetId());
	m_menu4->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainWindow::on_manual_prev ), this, m_menuItem8->GetId());
	m_menu4->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainWindow::on_manual_cancel ), this, m_menuItem9->GetId());
	m_menu2->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainWindow::on_about ), this, m_menuItem1->GetId());
}

MainWindow::~MainWindow()
{
	// Disconnect Events
	play_advance_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainWindow::on_play_advance ), NULL, this );
	stop_button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainWindow::on_stop ), NULL, this );

}
