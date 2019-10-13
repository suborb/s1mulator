//dma.cpp: dma controller emulator
//Copyright 2007 Scott Mansell <phiren@gmail.com>
//This software is licensed under the GNU GPL.

#include "include.h"

dma_controller::dma_controller(byte sPort, port_mapper *pm) {
	my_pm = pm;
	
	if (sPort > 10)
		 id = 2;
	else
		 id = 1; 
	for(int i = 0; i < 14; i++)
		pm->register_listener(this, sPort + i); //Nand command reg
	start_port=sPort;

	src_addr = 0;
	dst_addr = 0;
	counter = 0;
	mode = 0;
}

dma_controller::~dma_controller() {
	my_pm->deregister_listener(this);
}

void dma_controller::dmaGo() {
	printf("DMA%d: Copying, mode %d, %d bytes from %04x (NAND), to %04x (ZRAM1)...\n", id, mode, counter + 1, src_addr, dst_addr);
	for(int i = 0; i <= counter; i++) {
		zram1[dst_addr + i] = nand1->read(0x8000);
	}
	printf("...done\n");
	src_addr = 0;
	dst_addr = 0;
	counter = 0;
}

void dma_controller::out(byte addr, byte value) {
	switch(addr - start_port){
	case 0:
		src_addr += value;
		//printf("DMA%d: Scr Address 0 = 0x%02x\n", id, value);
		break;
	case 1:
		src_addr += value<<8;
		//printf("DMA%d: Scr Address 1 = 0x%02x\n", id, value);
		break;
	case 2:
		//printf("DMA%d: Scr Address 2 = 0x%02x\n", id, value);
		break;
	case 3:
		//printf("DMA%d: Scr Address 3 = 0x%02x\n", id, value);
		break;
	case 4:
		//printf("DMA%d: Scr Address 4 = 0x%02x\n", id, value);
		break;
	case 5:
		dst_addr += value;
		//printf("DMA%d: Dst Address 0 = 0x%02x\n", id, value);
		break;
	case 6:
		dst_addr += value<<8;
		//printf("DMA%d: Dst Address 1 = 0x%02x\n", id, value);
		break;
	case 7:
		//printf("DMA%d: Dst Address 2 = 0x%02x\n", id, value);
		break;
	case 8:
		//printf("DMA%d: Dst Address 3 = 0x%02x\n", id, value);
		break;
	case 9:
		//printf("DMA%d: Dst Address 4 = 0x%02x\n", id, value);
		break;
	case 10:
		counter += value;//counter&0xff00 + value;
		//printf("DMA%d: Counter low = 0x%02x\nCounter = %04x\n", id, value, counter);
		break;
	case 11:
		counter += value<<8; //counter&0xff + value<<8;
		//printf("DMA%d: Counter high= 0x%02x\nCounter = %04x\n", id, value, counter);
		break;
	case 12:
		mode = value;
		//printf("DMA%d: Mode = 0x%02x\n", id, value);
		break;
	case 13:
		if(value == 1) dmaGo();
		//printf("DMA%d: Command = 0x%02x\n", id, value);
		break;
	default:
		printf("DMA%d: Unknowen port 0x%02x: 0x%02x\n", id, addr, value);
		break;
	}
}

void dma_controller::register_zram1(byte *hw){
	zram1 = hw;
}

void dma_controller::register_nand1(memory_listener *hw){
	nand1 = hw;
}
