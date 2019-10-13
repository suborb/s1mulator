#include "runctrl.h"

RunCtrl::RunCtrl(wxWindow* parent, int id, const wxString& title, const wxPoint& pos, const wxSize& size, long style):
    wxFrame(parent, id, title, pos, size, wxCAPTION|wxCLOSE_BOX|wxMINIMIZE_BOX|wxSYSTEM_MENU|wxCLIP_CHILDREN)
{
    toolbar = CreateToolBar (wxTB_HORZ_TEXT | wxTB_NOICONS);
    SetToolBar(toolbar);
    toolbar->AddTool(IDM_RUNCTRL_BREAK, wxT("Break"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxT("Stop execution"), wxT("Stop execution"));
    toolbar->AddTool(IDM_RUNCTRL_STEP, wxT("Step"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxT("Execute one instruction"), wxT("Execute one instruction"));
    toolbar->AddTool(IDM_RUNCTRL_RUN, wxT("Run"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxT("Run to next breakpoint"), wxT("Run to next breakpoint"));
    toolbar->Realize();

    toolbar->EnableTool(IDM_RUNCTRL_BREAK, false);

    Layout();

    toolbar->Show(TRUE);

    menubar = new wxMenuBar();
    wxMenu *view = new wxMenu();
    wxMenuItem *display = new wxMenuItem(view,IDM_VIEW_DISPLAY,_("Display"),wxEmptyString,wxITEM_CHECK);
    view->Append(display);
    display->Check(true);
    view->Append(new wxMenuItem(view,IDM_VIEW_CPU,_("CPU"),wxEmptyString,wxITEM_CHECK));
    view->Append(new wxMenuItem(view,IDM_VIEW_PORTS,_("Ports"),wxEmptyString,wxITEM_CHECK));
    view->Append(new wxMenuItem(view,IDM_VIEW_ZRAM1,_("ZRAM1"),wxEmptyString,wxITEM_CHECK));
    menubar->Append(view,_("View"));
    SetMenuBar(menubar);    

    SetClientSize(toolbar->GetSize().GetWidth(),0);
}


