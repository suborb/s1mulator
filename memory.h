//memory.h: defines simulated memory mapper datatypes.
//Copyright 2006 <a.j.buxton@gmail.com>
//This software is licensed under the GNU GPL.

class memory_listener {
public:
	bool log_mem;
	virtual ~memory_listener();
	virtual byte read(dword);
	virtual void write(dword, byte);
};


class memory_mapper : public port_listener {

	byte internal_page, external_page_low, external_page_high;

	memory_listener *ce1,*ce2,*ce3,*dsp;

	port_mapper *my_pm;

public:
	byte *zram1,*zram2,*brom,*trom;

	memory_mapper(port_mapper *);
	~memory_mapper();
	byte read(dword);
	void write(dword, byte);
	virtual void out(byte, byte);
	void register_ce1(memory_listener *);
	void register_ce2(memory_listener *);
	void register_ce3(memory_listener *);
	void load_bin(wxFileName, dword);
};
