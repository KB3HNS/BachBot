// wxWidgets "Hello world" Program
// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#include <wx/wx.h>

#include "player_window.h"


class MyApp : public wxApp
{
public:
    virtual bool OnInit();

private:
    PlayerWindow *m_window = nullptr;
};


wxIMPLEMENT_APP(MyApp);


bool MyApp::OnInit()
{
    m_window = new PlayerWindow();
    m_window->Show(true);
    return true;
}
