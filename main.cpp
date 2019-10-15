//main.cpp: sets up core and simulated devices.
//Copyright 2006 <a.j.buxton@gmail.com>
//This software is licensed under the GNU GPL.

#include "include.h"

#include "wx/cmdline.h"
#include "wx/tokenzr.h"


#include "Z80/Z80.h"


#include "runctrl.h"
#include "hexview.h"

#define MEMORY_SIZE 0x4000

memory_mapper *memory;
port_mapper *ports;
display *d;
backlight *b;
nand *n;
dma_controller *dma1;
dma_controller *dma2;
bool core_running = true;
Z80 z;

dword init_exec = 0;
long trap_addr = -1;


void PatchZ80(register Z80 *R)
{
}

/****************************************************************************/
/* Input a byte from given I/O port                                         */
/****************************************************************************/
byte InZ80 (register word dPort) {
	byte Port = dPort&0xff;
//      printf("in: %02x, %02x\n", Port, ports->read(Port));
        return ports->in(Port);
}

/****************************************************************************/
/* Output a byte to given I/O port                                          */
/****************************************************************************/
void OutZ80 (register word dPort,register byte Value){
	byte Port = dPort&0xff;
//      printf("out: %02x, %02x\n", Port, Value);
        ports->out(Port, Value);
}

/****************************************************************************/
/* Read a byte from given memory location                                   */
/****************************************************************************/
byte RdZ80(register word Addr){
//	printf("read %04d\n", Addr);
        return memory->read(Addr);
}

/****************************************************************************/
/* Write a byte to given memory location                                    */
/****************************************************************************/
void WrZ80(register word Addr,register byte Value){
        memory->write(Addr, Value);
        return;
}



class s1mulator: public wxApp
{
private:
	RunCtrl *runctrl;
	DisplayFrame *dispframe;
	HexViewer *portsframe, *zram1frame;
	CPUViewer *cpuframe;
	wxTimer *runtimer;
	void load_bin(wxFileName, long);
public:
	// wx handlers
	virtual bool OnInit();
	virtual int OnExit(); 
	virtual void OnInitCmdLine(wxCmdLineParser& parser); 
	virtual bool OnCmdLineParsed(wxCmdLineParser& parser); 

	// event handlers
	void core_break(wxCommandEvent&);
        void core_step(wxCommandEvent&);
        void core_step10(wxCommandEvent&);
        void core_start(wxCommandEvent&);
        void core_run(wxTimerEvent&);

	void view_display(wxCommandEvent&);
	void view_cpu(wxCommandEvent&);
	void view_ports(wxCommandEvent&);
	void view_zram1(wxCommandEvent&);

	DECLARE_EVENT_TABLE()
};

IMPLEMENT_APP(s1mulator)

const wxCmdLineEntryDesc gCmdLineDesc[] = 
{ 
	// help
       { wxCMD_LINE_SWITCH, "h", "help", "Display usage info", 
		wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP }, 

	// version
       { wxCMD_LINE_SWITCH, "v", "version", "Display version and quit.",
                wxCMD_LINE_VAL_NONE, 0},

	// display params
       { wxCMD_LINE_OPTION, "D", "display", "Parameters for connected display.", 
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL}, 

	// backlight params
       { wxCMD_LINE_OPTION, "B", "backlight", "Parameters for connected backlight.", 
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL}, 

	// binary selection
       { wxCMD_LINE_OPTION, "b", "bin", "Load binary into memory. <str>=<filename>[,<offset>]",
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL},
	
	{ wxCMD_LINE_OPTION, "n","nand", "Load NAND file. <str>=<filename>",
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL},

	// z80 init params
       { wxCMD_LINE_OPTION, "e", "execute", "Execution offset.", 
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL},
       { wxCMD_LINE_OPTION, "t", "trap", "Trap address.", 
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL}, 

       { wxCMD_LINE_NONE, NULL, NULL, NULL, wxCMD_LINE_VAL_NONE, 0}, 
}; 

BEGIN_EVENT_TABLE (s1mulator, wxApp)
    EVT_MENU (IDM_RUNCTRL_BREAK, s1mulator::core_break)
    EVT_MENU (IDM_RUNCTRL_STEP,  s1mulator::core_step)
    EVT_MENU (IDM_RUNCTRL_RUN,   s1mulator::core_start)
    EVT_TIMER (-1, s1mulator::core_run)
    EVT_MENU (IDM_VIEW_DISPLAY, s1mulator::view_display)
    EVT_MENU (IDM_VIEW_CPU, s1mulator::view_cpu)
    EVT_MENU (IDM_VIEW_PORTS, s1mulator::view_ports)
    EVT_MENU (IDM_VIEW_ZRAM1, s1mulator::view_zram1)
END_EVENT_TABLE ()

void s1mulator::view_display (wxCommandEvent& event) {
	if(event.IsChecked()) {
		if(!dispframe) dispframe= new DisplayFrame(runctrl, d, b);
		dispframe->Show(true);
	} else {
		if(dispframe) dispframe->Close();
		dispframe = NULL;
	}
}

void s1mulator::view_cpu (wxCommandEvent& event) {
	if(event.IsChecked()) {
		if(!cpuframe) cpuframe = new CPUViewer(runctrl, &z);
		cpuframe->Show(true);
	} else {
		if(cpuframe) cpuframe->Close();
		cpuframe = NULL;
	}
}

void s1mulator::view_ports (wxCommandEvent& event) {
	if(event.IsChecked()) {
		if(!portsframe) portsframe= new HexViewer(runctrl, _("Ports"), ports->ports, 0x100);
		portsframe->Show(true);
	} else {
		if(portsframe) portsframe->Close();
		portsframe = NULL;
	}
}

void s1mulator::view_zram1 (wxCommandEvent& event) {
	if(event.IsChecked()) {
		if(!zram1frame) zram1frame= new HexViewer(runctrl, _("ZRAM1"), memory->zram1, 0x4000);
		zram1frame->Show(true);
	} else {
		if(zram1frame) zram1frame->Close();
		zram1frame = NULL;
	}
}


void s1mulator::core_break (wxCommandEvent& WXUNUSED(event)) {
	runtimer->Stop();
	core_running = false;
	runctrl->toolbar->EnableTool(IDM_RUNCTRL_RUN, true);
	runctrl->toolbar->EnableTool(IDM_RUNCTRL_STEP, true);
	runctrl->toolbar->EnableTool(IDM_RUNCTRL_BREAK, false);
	if(dispframe) dispframe->redraw();
	if(cpuframe) cpuframe->update();
	if(portsframe) portsframe->update();
	if(zram1frame) zram1frame->update();
}

void s1mulator::core_step10  (wxCommandEvent& event) {
        int i;
        for(i=0;i<100;i++) core_step(event);
}

void s1mulator::core_step  (wxCommandEvent& WXUNUSED(event)) {
	if(core_running) {
		printf("BUG: Can't step while running!\n");
		return;
	}

    ExecZ80(&z, 5);

	//z80_debug(&z);


	if(dispframe) dispframe->redraw();
	if(cpuframe) cpuframe->update();
	if(portsframe) portsframe->update();
	if(zram1frame) zram1frame->update();
}

void s1mulator::core_start   (wxCommandEvent& WXUNUSED(event)) {
	if (!runtimer->IsRunning()) {
		runtimer->Start(50, wxTIMER_CONTINUOUS);
		core_running = true;
		runctrl->toolbar->EnableTool(IDM_RUNCTRL_STEP, false);
		runctrl->toolbar->EnableTool(IDM_RUNCTRL_RUN, false);
		runctrl->toolbar->EnableTool(IDM_RUNCTRL_BREAK, true);
	}
	else printf("BUG: Already running!\n");
}

void s1mulator::core_run   (wxTimerEvent& WXUNUSED(event)) {
        int n,m;
	if(!core_running) { DebugZ80(&z); return;  }
        for(n=0;n<10;n++) {
                for(m=0;m<100;m++) {
			//printf("PC=%04x\r",z.PC.W);
			if(z.PC.W == trap_addr) {
				runtimer->Stop();
				// in case the timer already fired.
				core_running = false;
				printf("Trap address reached.\n");
				return;
			}
            ExecZ80(&z, 5);
		}
                if(dispframe) dispframe->redraw();
        }
}


void s1mulator::OnInitCmdLine(wxCmdLineParser& parser) 
{ 
	parser.SetDesc (gCmdLineDesc); 
	// must refuse '/' as parameter starter or cannot use "/path" style paths 
	parser.SetSwitchChars (_T("-")); 
}

bool s1mulator::OnCmdLineParsed(wxCmdLineParser& parser) 
{
	wxString param_tmp;

	if(parser.Found(_T("v"))) {
		printf("s1mulator v");
		printf(S1MULATOR_VERSION);
		printf(" (c) a.j.buxton@gmail.com\n");
		return false;
	}
	if(parser.Found(_T("D"), &param_tmp)) {
		wxStringTokenizer tkn(param_tmp, _T(","));
		long cols, pages, pport, ppin, pinv, mport, mpin, minv;
		if(!tkn.GetNextToken().ToLong(&cols,  0)) printf("Not enough arguments for display.\n");
		if(!tkn.GetNextToken().ToLong(&pages, 0)) printf("Not enough arguments for display.\n");
		if(!tkn.GetNextToken().ToLong(&pport, 0)) printf("Not enough arguments for display.\n");
		if(!tkn.GetNextToken().ToLong(&ppin,  0)) printf("Not enough arguments for display.\n");
		if(!tkn.GetNextToken().ToLong(&pinv,  0)) printf("Not enough arguments for display.\n");
		if(!tkn.GetNextToken().ToLong(&mport, 0)) printf("Not enough arguments for display.\n");
		if(!tkn.GetNextToken().ToLong(&mpin,  0)) printf("Not enough arguments for display.\n");
		if(!tkn.GetNextToken().ToLong(&minv,  0)) printf("Not enough arguments for display.\n");
		d = new display(cols, pages, pport, ppin, pinv, mport, mpin, minv, ports);
	}

	if(parser.Found(_T("B"), &param_tmp)) {
		wxStringTokenizer tkn(param_tmp, _T(","));
		long rport, rpin, rinv, gport, gpin, ginv, bport, bpin, binv;
		if(!tkn.GetNextToken().ToLong(&rport, 0)) printf("Not enough arguments for backlight.\n");
		if(!tkn.GetNextToken().ToLong(&rpin,  0)) printf("Not enough arguments for backlight.\n");
		if(!tkn.GetNextToken().ToLong(&rinv,  0)) printf("Not enough arguments for backlight.\n");
		if(!tkn.GetNextToken().ToLong(&gport, 0)) printf("Not enough arguments for backlight.\n");
		if(!tkn.GetNextToken().ToLong(&gpin,  0)) printf("Not enough arguments for backlight.\n");
		if(!tkn.GetNextToken().ToLong(&ginv,  0)) printf("Not enough arguments for backlight.\n");
		if(!tkn.GetNextToken().ToLong(&bport, 0)) printf("Not enough arguments for backlight.\n");
		if(!tkn.GetNextToken().ToLong(&bpin,  0)) printf("Not enough arguments for backlight.\n");
		if(!tkn.GetNextToken().ToLong(&binv,  0)) printf("Not enough arguments for backlight.\n");
		b = new backlight(rport, rpin, rinv, gport, gpin, ginv, bport, bpin, binv, ports);
	}
	
	if(parser.Found(_T("b"), &param_tmp)) {
		wxStringTokenizer tkn(param_tmp, _T(","));
		wxFileName filename(tkn.GetNextToken());
		long long_tmp;
		if(!filename.IsOk()) printf("Bad binary file name. Ignored.\n");
		if(!filename.FileExists()) printf("Binary file does not exist. Ignored.\n");
		if(!tkn.GetNextToken().ToLong(&long_tmp, 0)) memory->load_bin(filename, 0);
		else memory->load_bin(filename, long_tmp);
	}

	if(parser.Found(_T("n"), &param_tmp)) {
		wxStringTokenizer tkn(param_tmp, _T(","));
		wxFileName nandFile(tkn.GetNextToken());
		if(!nandFile.IsOk()) printf("Bad NAND file name. Ignored.\n");
		if(!nandFile.FileExists()) printf("NAND file does not exist. Ignored.\n");
		n = new nand(nandFile, ports);
	}

	if(parser.Found(_T("t"), &param_tmp)) {
		long long_tmp;
		if(!param_tmp.ToLong(&long_tmp, 0)) printf("Invalid value for trap address. Ignored.\n");
		else if (long_tmp >= 0xFFFF) printf("Trap address outside memory space. Ignored.\n");
		else {
			trap_addr = long_tmp;
			printf("Trap address set to: %04X\n", (dword)(trap_addr&0xFFFF));
		}
	}

	if(parser.Found(_T("e"), &param_tmp)) {
		long long_tmp;
		param_tmp.ToLong(&long_tmp, 0);
        	init_exec = long_tmp&0xFFFF;
		printf("Execution will start at %04X\n", init_exec);
	}

	return true; 
} 

bool s1mulator::OnInit() {

        runctrl = new RunCtrl((wxFrame *)NULL, -1, _("s1mulator"));
        SetTopWindow(runctrl);

	ports = new port_mapper();
	memory = new memory_mapper(ports);
	dma1 = new dma_controller(0x06, ports);
	dma2 = new dma_controller(0x14, ports);

	// call default behaviour (mandatory)
	// this calls the command line parser as well
	// this must be done after we set up memory and ports
	// so the command line options can use them

	if (!wxApp::OnInit())
		return false;

	// setup default display if not done.
    // display::display(int c, int p, byte port, byte ppin, bool pinv, byte mpin, bool minv, port_mapper *pm) {

	if(!b) b = new backlight(0xee, 0x01, true, 0xee, 0x02, false, 0xee, 0x04, false, ports);
	if(!d) d = new display(140, 6, 0xee, 0x04, false, 0x01, false, ports);
	if(!n) n = new nand(ports);
	
	memory->register_ce3(d);
	memory->register_ce1(n);
	memory->register_ce2(new nand()); //No chip: to prevent segfaults

	dma1->register_zram1(memory->zram1);
	dma1->register_nand1(n);
	dma2->register_zram1(memory->zram1);
	dma2->register_nand1(n);

	b->log_port = 1;
	memory->log_port = 1;


    ResetZ80(&z);

	z.PC.W = init_exec;
    z.SP.W = 0x3FFF;
	dispframe = new DisplayFrame(runctrl, d, b);	

	runtimer = new wxTimer(this, -1);

        dispframe->Show(TRUE);
	runctrl->Show(TRUE);
	return true;
}

int s1mulator::OnExit() { 
	// clean up 

	delete memory;
	delete ports;
	delete b;
	delete d;
	return 0; 
} 

