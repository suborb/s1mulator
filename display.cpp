//display.cpp: LCD simulator.
//Copyright 2006 <a.j.buxton@gmail.com>
//This software is licensed under the GNU GPL.

#include "include.h"

void display::init_display(int c, int p, byte pport, byte ppin, bool pinv, byte mport, byte mpin, bool minv, port_mapper *pm) {

	cols = c;
	pages = p;
	cursor_col = 0;
	cursor_page = 0;
	inverted = 0;
	all_on = 0;
	enabled = 1;

	power = new port_pin(pport, ppin, pinv);
	mode = new port_pin(mport, mpin, minv);

	my_pm = pm;

	pm->register_listener(this, pport);
	if(mport != pport) pm->register_listener(this, mport);

        // the actual texture size must be a power of 2

        fb_c = 1;
        fb_p = 1;
        while(fb_c < c) fb_c = fb_c<<1;
        while(fb_p < p) fb_p = fb_p<<1;

        fb = new byte[fb_p*fb_c];
	memset(fb, 0, fb_p*fb_c);

        dirty = TRUE;

//      printf("%d,%d\n", fb_p, fb_c);

}

display::display(int c, int p, byte pport, byte ppin, bool pinv, byte mport, byte mpin, bool minv, port_mapper *pm) {
	init_display(c, p, pport, ppin, pinv, mport, mpin, minv, pm);
}

display::display(int c, int p, byte port, byte ppin, bool pinv, byte mpin, bool minv, port_mapper *pm) {
	init_display(c, p, port, ppin, pinv, port, mpin, minv, pm);
}

display::~display() {
	my_pm->deregister_listener(this);
	delete[] fb;
	delete power;
	delete mode;
}

void display::write(dword addr, byte d) {
    //printf("Display write %04x, %02x set =%d\n",addr,d,mode->get());
	if(mode->get()) {
		fb[cursor_page+(fb_p*cursor_col)] = d;
		cursor_col = (cursor_col+1)%cols;
		dirty = TRUE;
	}
	else {
		switch(d&0xfe) {
		case 0xae:
			enabled = d&0x01;
			break;
		case 0xa6:
			inverted = d&0x01;
			dirty = TRUE;
			break;
		case 0xa4:
			all_on = d&0x01;
			dirty = TRUE;
			break;
		default:
			if(d&0xC0 == 0x40) start_line = d&0x3F;
			else switch(d&0xF0) {
			case 0xB0:
				cursor_page = (d&0x0f)%pages;
				break;
			case 0x00:
				cursor_col = ((cursor_col&0xf0)|(d&0x0f))%cols;
				break;
			case 0x10:
				cursor_col = ((cursor_col&0x0f)|((d&0x0f)<<4))%cols;
				break;
			}
			break;
		}
	}
}

byte display::read(dword addr) {
	if(mode->get()) return fb[cursor_page+(fb_p*cursor_col)];
	else return 1; // TODO: return status properly
}

void display::out(byte addr, byte value) {
	power->set(addr, value);
	mode->set(addr, value);
}

void display::maketex(void) {
        if(dirty) {
                GLfloat index_map[2] = {0.0, 1.0};
                if(all_on) { index_map[0] = 1.0; index_map[1] = 1.0; }
                else if(inverted) { index_map[0] = 1.0; index_map[1] = 0.0; }
                glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 2, index_map);
                glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 2, index_map);
                glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 2, index_map);
                glPixelMapfv(GL_PIXEL_MAP_I_TO_A, 2, index_map);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fb_p*8, fb_c, 0, GL_COLOR_INDEX, GL_BITMAP, fb);
                dirty = FALSE;
        }
}

void DisplayFrame::redraw(void) {
        // fill  = with backlight colour
        // copy fb -> window
        // flip canvas
        canvas->SetCurrent();

        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        glViewport(0, 0, (GLint)(d->cols*2), (GLint)(d->pages*8*2));
        glOrtho(0,d->cols, d->pages*8, 0, -1, 1);


        // opengl init
        glEnable(GL_BLEND);
        glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
        glPixelStorei(GL_UNPACK_LSB_FIRST, 1);

        glEnable(GL_TEXTURE_2D);
//        glGenTextures(1, &texnum);
        glBindTexture(GL_TEXTURE_2D, texnum);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	d->maketex();

        if(b) glClearColor(b->get_r()+0.1, b->get_g()+0.2, b->get_b()+0.1, 0.0);
        else glClearColor(1.0,1.0,1.0,0.0);
        glClear(GL_COLOR_BUFFER_BIT);

        glColor4f(1.0, 1.0, 1.0, 1.0);
        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(0.0,0.0); glVertex2i(0,0);
        glTexCoord2f(1.0,0.0); glVertex2i(0,d->fb_p*8);
        glTexCoord2f(0.0,1.0); glVertex2i(d->fb_c,0);
        glTexCoord2f(1.0,1.0); glVertex2i(d->fb_c,d->fb_p*8);
        glEnd();

        glFlush();

        canvas->SwapBuffers();
}

DisplayFrame::DisplayFrame(wxFrame *parent, display *the_display, backlight *the_backlight) 
: wxFrame(parent, -1,  _("Display"), wxDefaultPosition, wxDefaultSize, wxMINIMIZE_BOX | wxCAPTION | wxCLIP_CHILDREN)
{
	d = the_display;
	b = the_backlight;
        int attribs[] = {WX_GL_RGBA, 0};
        canvas = new wxGLCanvas(this, -1, wxPoint(0,0), wxDefaultSize, wxNO_BORDER, _("gl area"), attribs);

        canvas->SetClientSize(wxSize(d->cols*2,d->pages*8*2));
        SetClientSize(canvas->GetSize());
        canvas->SetCurrent();

        glEnable(GL_TEXTURE_2D);
        glGenTextures(1, &texnum);
}
