//port.cpp: port mapper.
//Copyright 2006 <a.j.buxton@gmail.com>
//This software is licensed under the GNU GPL.

#include "include.h"

port_listener::~port_listener() {}

void port_listener::out(byte addr, byte value) {};

port_mapper::port_mapper() {
	ports = new byte[0x100];
	memset(ports, 0, 0x100);
	port_bindings = NULL;
}


port_mapper::~port_mapper() {
	port_list_item *tmp;
	while (port_bindings != NULL) {
		tmp = port_bindings;
		port_bindings = port_bindings->next;
		delete tmp;
	}
	delete[] ports;	
}

byte port_mapper::in(byte addr) {
	return ports[addr];
}

void port_mapper::out(byte addr, byte value) {
	port_list_item *tmp = port_bindings;

	//io registers
	if(addr == 0xF2) gpio_b_output = value;
	else if(addr == 0xF4) gpio_c_output = value&0x07;
	else if(addr == 0xF0 || addr == 0xF1 || addr == 0xF3) ports[addr] = value&0x77;
	else ports[addr] = value;

	// GPIO B latches	
	if(addr == 0xF0 || addr == 0xF1 || addr == 0xF2) {
		// value sent to listener
		value = gpio_b_output & ports[0xF0] & (~ports[0xF1]) & 0x77;	
		// value for the Cpu
		value = ports[0xF2] = (gpio_b_input & ports[0xF1]) | value;
		// address for listener
		addr = 0xF2;
		// hide key latch'd outputs from listener
		value &= (0x77&~ports[0xEF]);
	}

	// GPIO C latches
	if(addr == 0xF3 || addr == 0xF4) {
		// value we send to listener
		value = (gpio_c_output & (ports[0xF3] & ~(ports[0xF3]>>4) & 0x07));
		// value stored for the CPU
		ports[0xF4] = (gpio_c_input & (ports[0xF3]>>4)) | value;
		// address for listener
		addr = 0xF4;
		
	}

	// GPO A - listener might not get bit 2 if it is disabled 
	if(addr == 0xEE) value &= (0xFB|(value>>1));
	

	while (tmp != NULL) { // notify hw
		if (tmp->addr == addr) {
			tmp->hw->out(addr, value);
			
		}
		tmp=tmp->next;
	}
}

void port_mapper::register_listener(port_listener *hw, byte addr){
	port_list_item *tmp = new port_list_item;

	tmp->hw = hw;
	tmp->addr = addr;
	tmp->next = port_bindings;

	port_bindings = tmp;
}

void port_mapper::deregister_listener(port_listener *hw){
	port_list_item *tmp = port_bindings;
	port_list_item *prev = NULL;

	while (tmp != NULL) {
		if(tmp->hw == hw) {
			if (prev == NULL) {
				port_bindings = tmp->next;
				prev = NULL;
			}
			else prev->next = tmp->next;
			delete tmp;
		}
		else prev = tmp;
		tmp = tmp->next;
	}
}

port_pin::port_pin(byte addr, byte mask, bool inv) {
	a = addr;
	m = mask;
	i = inv;
	v = inv;
}

bool port_pin::get(void) {
	return v;
}

void port_pin::set(byte addr, byte value) {
	if(addr == a) v = (value&m)^i;
}

