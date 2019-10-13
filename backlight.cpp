//backlight.cpp: defines simulated backlights.
//Copyright 2006 <a.j.buxton@gmail.com>
//This software is licensed under the GNU GPL.

#include "include.h"

void backlight::init_backlight(byte rport, byte rpin, bool rinv, byte gport, byte gpin, bool ginv, byte bport, byte bpin, bool binv, port_mapper *pm) {
	r = new port_pin(rport, rpin, rinv);
	g = new port_pin(gport, gpin, ginv);
	b = new port_pin(bport, bpin, binv);
	my_pm = pm;
	pm->register_listener(this, rport);
	if(gport != rport) pm->register_listener(this, gport);
	if(bport != rport && bport != gport) pm->register_listener(this, bport);
}

backlight::backlight(byte rport, byte rpin, bool rinv, byte gport, byte gpin, bool ginv, byte bport, byte bpin, bool binv, port_mapper *pm) {
	init_backlight(rport, rpin, rinv, gport, gpin, ginv, bport, bpin, binv, pm);
}

backlight::backlight(byte port, byte rpin, bool rinv, byte gpin, bool ginv, byte bpin, bool binv, port_mapper *pm) {
	init_backlight(port, rpin, rinv, port, gpin, ginv, port, bpin, binv, pm);
}

backlight::backlight(byte port, byte pin, bool inv, port_mapper *pm) {
	init_backlight(port, pin, inv, port, pin, inv, port, pin, inv, pm);
}

backlight::~backlight() {
	my_pm->deregister_listener(this); // deletes all of them
	delete r; delete g; delete b;
}

bool backlight::get_r(void) {
	return r->get();
}

bool backlight::get_g(void) {
	return g->get();
}

bool backlight::get_b(void) {
	return b->get();
}

void backlight::out(byte addr, byte value) {
	r->set(addr, value);
	g->set(addr, value);
	b->set(addr, value);
	if (log_port) printf("Backlight: %c%c%c\n", r->get()?'R':' ', g->get()?'G':' ', b->get()?'B':' ');
}
