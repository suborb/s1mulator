//memory.cpp: memory mapper.
//Copyright 2006 <a.j.buxton@gmail.com>
//This software is licensed under the GNU GPL.

#include "include.h"

memory_listener::~memory_listener() {}

byte memory_listener::read(dword addr) { return 0; };

void memory_listener::write(dword addr, byte value) {};

void memrset(byte *mem, int unused, int size) {
	int n;
	for(n=0;n<size;n++) mem[n] = rand();
}

memory_mapper::memory_mapper(port_mapper *pm) {
	zram1 = new byte[0x4000];
	zram2 = new byte[0x1000];
	brom = new byte[0x2000];
	trom = new byte[0x5400];

	srand(time(0));
	// memrset(zram1, 0, 0x4000);
    //     memrset(zram2, 0, 0x1000);
    //     memrset(brom, 0, 0x2000);
    //     memrset(trom, 0, 0x5400);

	pm->register_listener(this, 0x01);
	pm->register_listener(this, 0x02);
	pm->register_listener(this, 0x05);

	ce1 = NULL;
	ce2 = NULL;
	ce3 = NULL;

	internal_page = 0;
	external_page_low = 0;
	external_page_high = 0;

	my_pm = pm;
}

memory_mapper::~memory_mapper() {
	my_pm->deregister_listener(this);

	delete[] zram1;	
	delete[] zram2;	
	delete[] brom;	
	delete[] trom;	
}

byte memory_mapper::read(dword addr) {
	switch(addr>>14) {
	case 0:
		// zram1
		return zram1[addr];
	case 0x1:
		switch(internal_page&0x07) {
		// reserved
		case 0x03:
			break;
		// dsp
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x04:
		case 0x05:
		case 0x06:
			break;
		// zram2
		case 0x07:
			return zram2[addr&0xfff];
		}
		break;
	case 0x2:
	case 0x3:
		// extended memory
		switch(external_page_high&0x38) { // 02h
		// brom or trom
		case 0:
			switch(external_page_low) { // 01h
			case 0:
				return brom[addr&0x1fff];
			case 2:
				return trom[addr&0x53ff];
			}
		case 0x08:
			if(ce1) return ce1->read(addr&0xefff);
			else return 0;
		case 0x10:
			if(ce2) return ce2->read(addr&0xefff);
			else return 0;
		case 0x18:
			if(ce3) return ce3->read(addr&0xefff);
			else return 0;
		}
		break;
	}
	return 0;
}

void memory_mapper::write(dword addr, byte value) {
	switch(addr>>14) {
	case 0:
		// zram1
		zram1[addr] = value;
	case 0x1:
		switch(internal_page&0x07) {
		// reserved
		case 0x03:
			break;
		// dsp
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x04:
		case 0x05:
		case 0x06:
			break;
		// zram2
		case 0x07:
			zram2[addr&0xfff] = value;
			break;
		}
		break;
	case 0x2:
	case 0x3:
		// extended memory
		switch(external_page_high&0x38) { // 02h
		// brom or trom
		case 0:
			switch(external_page_low) { // 01h
			case 0:
				brom[addr&0x1fff] = value;
				break;
			case 2:
				trom[addr&0x53ff] = value;
				break;
			}
			break;
		case 0x08:
			if(ce1) ce1->write(addr&0xefff, value);
			break;
		case 0x10:
			if(ce2) ce2->write(addr&0xefff, value);
			break;
		case 0x18:
			if(ce3) ce3->write(addr&0xefff, value);
			break;
		}
		break;
	}
}

void memory_mapper::out(byte addr, byte value) {
	switch(addr) {
	case 0x01:
		external_page_low = value;
		break;
	case 0x02:
		external_page_high = value;
		break;
	case 0x05:
		internal_page = value;
		break;
	}
	if(log_port) {
	switch(addr) {
	case 0x05:
		switch(internal_page&0x07) {
		// reserved
		case 0x03:
			printf("0x4000: RSRVD\n");
			break;
		// dsp
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x04:
		case 0x05:
		case 0x06:
			printf("0x4000: DSP\n");
			break;
		// zram2
		case 0x07:
			printf("0x4000: ZRAM2\n");
			break;
		default:
			printf("0x4000: ERROR\n");
			break;
		}
		break;
	case 0x01:
	case 0x02:
		switch(external_page_high&0x38) { // 02h
		// brom or trom
		case 0:
			switch(external_page_low) { // 01h
			case 0:
				printf("0x8000: BROM\n");
				break;
			case 2:
				printf("0x8000: TROM\n");
				break;
			}
			break;
		case 0x08:
			printf("0x8000: CE1 (NAND)\n");
			break;
		case 0x10:
			printf("0x8000: CE2 (Second NAND)\n");
			break;
		case 0x18:
			printf("0x8000: CE3 (Display)\n");
			break;
		default:
			printf("0x8000: ERROR\n");
			break;
		}
		break;
	}
	}
}

void memory_mapper::register_ce1(memory_listener *hw){
	ce1 = hw;
}

void memory_mapper::register_ce2(memory_listener *hw){
	ce2 = hw;
}

void memory_mapper::register_ce3(memory_listener *hw){
	ce3 = hw;
}

void memory_mapper::load_bin(wxFileName filename, dword addr) {
	wxFile binfile(filename.GetFullPath());

        if(!binfile.IsOpened()) { printf("Could not open binary file.\n"); return; }
 
	if(addr == 0x8000) {
		if((binfile.Length()) > 0x2000) {
                	printf("Binary too big for BROM. Not loaded.\n"); 
			return; 
		}

	        binfile.Read(brom, binfile.Length());
	        return;
	}

	else if(addr >= 0x3fff) {
		printf("Binary offset address outside memory space. Not loading.\n");
		return;
	}

	if((binfile.Length()+addr) > 0x4000) { 
		printf("Binary too big for memory. Not loaded.\n"); 
		return; 
	}

	binfile.Read(zram1+addr, binfile.Length());	
	return; 
}

