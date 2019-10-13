//nand.cpp: Nand interface emulator
//Copyright 2007 Scott Mansell <phiren@gmail.com>
//This software is licensed under the GNU GPL.

#include "include.h"

nand::nand(port_mapper *pm) {
	my_pm = pm;
	
	pm->register_listener(this, 0x28);
	pm->register_listener(this, 0x29);
	pm->register_listener(this, 0x2a); //Nand command reg
	pm->register_listener(this, 0x2b);
	pm->register_listener(this, 0x2c);
	pm->register_listener(this, 0xec);
	pm->register_listener(this, 0xcc);
	pm->register_listener(this, 0xcd);
	pm->register_listener(this, 0xce);
	pm->register_listener(this, 0xcf);

	pm->register_listener(this, 0x01); //EM_Page_low -- for address and cmd latches
	address_latch = false;
	command_latch = false;
	

	mode = 0; //idle
	enabled = true;

	//Setup id string
	id[0] = 0xad;	//Hynix
	id[1] = 0xf1;	//HY27UH081G2M
	id[3] = 0x15;	//Page size=2k, Spare Aera size=8, Serial access time=Fast, Block Size=128k, Organization=X8
	blocks = 1024;
	pages = 64;
	page_size = 2048;

	canWrite = false;

	loaded_page = new byte[page_size];
	memset(loaded_page, '\0', page_size);
	file = false;
}

nand::nand(wxFileName filename, port_mapper *pm) {
	my_pm = pm;
	
	pm->register_listener(this, 0x28);
	pm->register_listener(this, 0x29);
	pm->register_listener(this, 0x2a); //Nand command reg
	pm->register_listener(this, 0x2b);
	pm->register_listener(this, 0x2c);
	pm->register_listener(this, 0xec);
	pm->register_listener(this, 0xcc);
	pm->register_listener(this, 0xcd);
	pm->register_listener(this, 0xce);
	pm->register_listener(this, 0xcf);

	pm->register_listener(this, 0x01); //EM_Page_low -- for address and cmd latches
	address_latch = false;
	command_latch = false;

	mode = 0xff;
	enabled = true;

	//Setup id string
	id[0] = 0xad;	//Hynix
	id[1] = 0xf1;	//HY27UH081G2M
	id[3] = 0x15;	//Page size=2k, Spare Aera size=8, Serial access time=Fast, Block Size=128k, Organization=X8
	blocks = 1024;
	pages = 64;
	page_size = 2048;

	canWrite = false;

 	nandchip = new wxFile(filename.GetFullPath());
	loaded_page = new byte[page_size];
	memset(loaded_page, '\0', page_size);
	file = true;
}

nand::nand() { //Empty chip
	enabled = false;
}

nand::~nand() {
	my_pm->deregister_listener(this);
}

void nand::getAddress(byte addr) {
	address[addressCycle] = addr;
	//printf("NAND: address[%d] = 0x%02x\n", addressCycle, addr);
	addressCycle++;
}

void nand::reset() {
	mode= NAND_RESET;
	column= 0;
	page= 0;
	block= 0;
	memset(loaded_page, '\0', page_size);
	canWrite = false;
}
void nand::command(byte cmd) {
switch(mode) {
case NAND_READ:
	switch(cmd) {
	case 0x30:
		mode = NAND_READ_MODE_1;
		processAddress();
		//printf("NAND: Read Mode 1, column:0x%04d page:%02d block:%04d\n", column, page, block);
		load_page();
		break;
	case 0xff:
		reset();
	}
default:
	switch(cmd) {
	case 0x00:
		mode = NAND_READ;
		addressCycle = 0;
		//printf("NAND: Read...\n");
		break;
	case 0x70:
		mode = NAND_STATUS;
		printf("NAND: Get status\n");
		break;
	case 0x90:
		mode = NAND_ID;
		id_byte = 0;
		printf("NAND: Get ID\n");
		break;
	case 0xFF:
		printf("NAND: Reset\n");
		break;
	default:
		printf("NAND: Unknowen Command: 0x%02x\n", cmd);
	}
}
}

void nand::processAddress() {
	column = address[0] + address[1]<<8; // First address cycle + 2nd cycle
	page = address[2]&0x3f; // Lowest 6 bits of 3rd address cycle
	block = (address[2]&0xc0)>>6 + address[3]<<8;
}

void nand::load_page() {
	if(file) {
	nandchip->Seek(block*pages*page_size + page*page_size, wxFromStart);
	nandchip->Read(loaded_page, page_size);
	}
}
	

void nand::write(dword addr, byte d) {
	if(enabled) {
		if(!canWrite) {
			//printf("NAND: command 0x%04x: 0x%02x\n", addr, d);
			if(command_latch && !address_latch)
				command(d);
			if(address_latch && !command_latch)
				getAddress(d);
		} else {
			printf("NAND: write 0x%04x: 0x%02x\n", addr, d);
		}
	}
}

byte nand::read(dword addr) {
	if(enabled){
	//printf("NAND: read 0x%04x\n", addr);
	switch(mode) {
	case NAND_STATUS:
		return 0xe0;
	case NAND_ID:
		if(id_byte==4) mode = NAND_RESET;
		return id[id_byte++]; //Return byte from id string, and increase index
	case NAND_READ_MODE_1:
 		return loaded_page[column++];
	}
	}
	return 0;
}

void nand::out(byte addr, byte value) {
	if(enabled) {
	switch(addr){
	case 0x01: //EM_PAGE_LOW
		command_latch = value&1;
		address_latch = value>>1;
		printf("NAND: address/command latches: %02x , cmd=%d, addr=%d\n", value, command_latch, address_latch);
		break;
	case 0x2a: //NAND command reg
		command(value);
		break;
	default:
		printf("NAND: Unknowen port 0x%02x: 0x%02x\n", addr, value);
		break;
	}
	}
}
