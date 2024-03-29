#include <wx/wx.h>
#include <wx/image.h>

class HexViewer: public wxFrame {
public:
    HexViewer(wxWindow*, const wxString&, byte *, int );
    virtual void update(void);
private:
    byte *data;
    int length;
    wxTextCtrl* text_ctrl_1;
}; 

class CPUViewer: public wxFrame {
public:
    CPUViewer(wxWindow*, Z80 * );
    virtual void update(void);
private:
    Z80 *z;
    wxTextCtrl* text_ctrl_1;
}; 
