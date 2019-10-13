//dma.h: Defines the dma controller emulation
//Copyright 2007 Scott Mansell <phiren@gmail.com>
//This software is licensed under the GNU GPL.

class dma_controller : public port_listener {
private:
	//mappers
	port_mapper *my_pm;
	memory_mapper *my_mm;

	byte *zram1;
	memory_listener *nand1;

	byte start_port;
	byte id;

	dword src_addr;
	dword dst_addr;
	dword counter;
	byte mode;
	
	void dmaGo();
public:
	void register_zram1(byte *);
	void register_nand1(memory_listener *);
	dma_controller (byte, port_mapper *);
	virtual ~dma_controller ();
	virtual void out(byte, byte);
};

