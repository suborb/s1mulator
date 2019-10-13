//nand.h: defines the NAND interface emulator
//Copyright 2007 Scott Mansell <phiren@gmail.com>
//This software is licensed under the GNU GPL.


#define NAND_STATUS 0x70
#define NAND_ID 0x90
#define NAND_READ 0x00
#define NAND_READ_MODE_1 0x30
#define NAND_RESET 0xFF


class nand : public port_listener, public memory_listener {
private:
	//mappers
	port_mapper *my_pm;
	memory_mapper *my_mm;

	//nand Structor
	int blocks;
	int pages;
	int page_size;
	wxFile *nandchip;
	byte *loaded_page;
	bool file;


	int mode;
	bool enabled;

	bool command_latch;
	bool address_latch;

	bool canWrite;

	int addressCycle; 
	
	byte address[5];
	// Address 0 and 1 contain 12byts of column address followed by 0000
	// Address 2 is ROW 1
	// Address 3 is ROW 2

	int column;
	int block;
	byte page;

	//id
	byte id[4];
	byte id_byte;

	void processAddress();
	void command(byte);
	void getAddress(byte);
	void load_page();
	void reset();
	
	
public:
	nand (port_mapper *);
	nand (wxFileName, port_mapper *);
	nand ();
	//void loadFile(wxFileName);
	virtual ~nand ();
	virtual byte read(dword);
	virtual void write(dword, byte);
	virtual void out(byte, byte);
};
