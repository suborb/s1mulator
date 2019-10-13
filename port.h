//port.h: defines simulated port mapper datatypes.
//Copyright 2006 <a.j.buxton@gmail.com>
//This software is licensed under the GNU GPL.

class port_listener {
private:
public:
	bool log_port;
	virtual ~port_listener();
	virtual void out(byte, byte);
};

struct port_list_item {
	port_listener *hw;
	byte addr;
	port_list_item *next;
};

class port_mapper : public port_listener {

	
	port_list_item *port_bindings;
	byte gpio_b_output;
	byte gpio_c_output;
	byte gpio_b_input;
	byte gpio_c_input;

public:
	byte *ports;
	port_mapper();
	~port_mapper();
	void masked_write(byte, byte, byte);
	byte in(byte);
	void out(byte, byte);
	void register_listener(port_listener *, byte);
	void deregister_listener(port_listener *);
};

class port_pin {
private:
	byte a; // port address
	byte m; // pin mask
	bool i; // inverted
	bool v; // last value
	port_mapper *my_pm;
public:
	port_pin(byte, byte, bool);
	bool get(void);
	void set(byte, byte);
};
