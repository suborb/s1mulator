//display.h: defines simulated LCD class.
//Copyright 2006 <a.j.buxton@gmail.com>
//This software is licensed under the GNU GPL.

class display : public port_listener, public memory_listener {

public:	
	// size
	int cols;
	int pages;

	// lcd controller registers
	int cursor_col;
	int cursor_page;
	int start_line;
	bool enabled;
	bool inverted;
	bool all_on;

	// framebuffer
	int fb_c;
	int fb_p;
	byte *fb;
	bool dirty;

	// control port io
	port_pin *power;
	port_pin *mode;

	port_mapper *my_pm;
	memory_mapper *my_mm;

	// the REAL constructors
	void init_display(int, int, byte, byte, bool, byte, byte, bool, port_mapper *);
	void init_fb(int, int);

	display (int, int, byte, byte, bool, byte, bool, port_mapper *);
	display (int, int, byte, byte, bool, byte, byte, bool, port_mapper *);
	virtual ~display ();
	virtual byte read(dword);
	virtual void write(dword, byte);
	virtual void out(byte, byte);
	virtual void maketex(void);
};

class DisplayFrame : public wxFrame {
public:
        DisplayFrame(wxFrame *, display *, backlight *);
        void redraw(void);
private:
        wxGLCanvas *canvas;
        GLuint texnum;
        display *d;
	backlight *b;
};

