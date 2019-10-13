#include <wx/wx.h>
#include <wx/image.h>

enum
{
    IDM_RUNCTRL_BREAK = 200,
    IDM_RUNCTRL_STEP,
    IDM_RUNCTRL_RUN,
    IDM_VIEW_DISPLAY,
    IDM_VIEW_CPU,
    IDM_VIEW_PORTS,
    IDM_VIEW_ZRAM1
};


class RunCtrl: public wxFrame {
public:
    RunCtrl(wxWindow* parent, int id, const wxString& title, const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize, long style=wxDEFAULT_FRAME_STYLE);
    wxToolBar* toolbar;
    wxMenuBar* menubar;
};
