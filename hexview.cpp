#include "include.h"

#include "z80.h"

#include "hexview.h"


HexViewer::HexViewer(wxWindow* parent, const wxString& title, byte *d, int l)
: wxFrame(parent, -1,  title, wxDefaultPosition, wxDefaultSize, wxMINIMIZE_BOX | wxCAPTION | wxCLIP_CHILDREN)
{
    data = d; length = l;

    text_ctrl_1 = new wxTextCtrl(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxTE_DONTWRAP);
    text_ctrl_1->SetFont(wxFont(10, wxTELETYPE, wxNORMAL, wxNORMAL, 0, wxT("")));
    update();

    

    wxBoxSizer* sizer_1 = new wxBoxSizer(wxVERTICAL);
    sizer_1->Add(text_ctrl_1, 1, wxEXPAND|wxADJUST_MINSIZE, 0);
    SetAutoLayout(true);
    SetSizer(sizer_1);
    sizer_1->Fit(this);
    sizer_1->SetSizeHints(this);
    SetClientSize(450,300);
    Layout();

}

void HexViewer::update(void) {
	int n;
	wxString s;

	text_ctrl_1->Clear();
	for(n=0;n<length;n+=16) {
		s.Printf(_("%04X: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n"), n, data[n], data[n+1], data[n+2], data[n+3], data[n+4], data[n+5], data[n+6], data[n+7], data[n+8], data[n+9], data[n+10], data[n+11], data[n+12], data[n+13], data[n+14], data[n+15]);
		text_ctrl_1->AppendText(s);
	}
}

CPUViewer::CPUViewer(wxWindow* parent, Z80CPU *zz)
: wxFrame(parent, -1,  _("CPU"), wxDefaultPosition, wxDefaultSize, wxMINIMIZE_BOX | wxCAPTION | wxCLIP_CHILDREN)
{
	z = zz;

    text_ctrl_1 = new wxTextCtrl(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxTE_DONTWRAP);
    text_ctrl_1->SetFont(wxFont(10, wxTELETYPE, wxNORMAL, wxNORMAL, 0, wxT("")));
    update();

    

    wxBoxSizer* sizer_1 = new wxBoxSizer(wxVERTICAL);
    sizer_1->Add(text_ctrl_1, 1, wxEXPAND|wxADJUST_MINSIZE, 0);
    SetAutoLayout(true);
    SetSizer(sizer_1);
    sizer_1->Fit(this);
    sizer_1->SetSizeHints(this);
    SetClientSize(450,150);
    Layout();

}

//  UI8   r[14];      // [0]->{B  C  D  E  H  L  F  A  I  R  IXh IXl IYh IYl}
//  UI16  rUpd;       // bit mask of registers updated by last instr

//  UI8   r_[8];      // [0]->{B' C' D' E' H' L' F' A'}
//  UI16  rUpd_;      // bit mask of registers updated by last instr

//  UI16  rp[8];      // [0]->{BC, DE, HL, SP, AF, PC, IX, IY}
//  UI16  rpUpd;      // bit mask of registers updated by last instr

void CPUViewer::update(void) {
	wxString s;

	text_ctrl_1->Clear();
	s.Printf(_("PC: 0x%04X (%5d)  SP: 0x%04X (%5d)  \n"), 
		z->rp[PC], z->rp[PC], z->rp[SP], z->rp[SP]);
	text_ctrl_1->AppendText(s);
	s.Printf(_("BC: 0x%04X (%5d)   B: 0x%02X (%3d)   C: 0x%02X (%3d)\n"), 
		z->rp[BC], z->rp[BC], z->r[B], z->r[B], z->r[C], z->r[C]);
	text_ctrl_1->AppendText(s);
	s.Printf(_("DE: 0x%04X (%5d)   D: 0x%02X (%3d)   E: 0x%02X (%3d)\n"), 
		z->rp[DE], z->rp[DE], z->r[D], z->r[D], z->r[E], z->r[E]);
	text_ctrl_1->AppendText(s);
	s.Printf(_("HL: 0x%04X (%5d)   H: 0x%02X (%3d)   L: 0x%02X (%3d)\n"), 
		z->rp[HL], z->rp[HL], z->r[H], z->r[H], z->r[L], z->r[L]);
	text_ctrl_1->AppendText(s);
	s.Printf(_("AF: 0x%04X (%5d)   F: 0x%02X (%3d)   A: 0x%02X (%3d)\n"), 
		z->rp[AF], z->rp[AF], z->r[F], z->r[F], z->r[A], z->r[A]);
	text_ctrl_1->AppendText(s);
	s.Printf(_("IX: 0x%04X (%5d) IXh: 0x%02X (%3d) IXl: 0x%02X (%3d)\n"), 
		z->rp[IX], z->rp[IX], z->r[IXh], z->r[IXh], z->r[IXl], z->r[IXl]);
	text_ctrl_1->AppendText(s);
	s.Printf(_("IY: 0x%04X (%5d) IYh: 0x%02X (%3d) IYl: 0x%02X (%3d)\n"), 
		z->rp[IY], z->rp[IY], z->r[IYh], z->r[IYh], z->r[IYl], z->r[IYl]);
	text_ctrl_1->AppendText(s);

	// TODO: flags and other stuff
}

