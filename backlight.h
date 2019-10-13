//backlight.h: defines simulated backlights
//Copyright 2006 <a.j.buxton@gmail.com>
//This software is licensed under the GNU GPL.

class backlight : public port_listener {
private:
	port_mapper *my_pm;
	port_pin *r;
	port_pin *g;
	port_pin *b;

	void init_backlight(byte, byte, bool, byte, byte, bool, byte, byte, bool, port_mapper *);

public:
	backlight(byte, byte, bool, byte, byte, bool, byte, byte, bool, port_mapper *);
	backlight(byte, byte, bool, byte, bool, byte, bool, port_mapper *);
	backlight(byte, byte, bool, port_mapper *);
	virtual ~backlight();
	virtual bool get_r(void);
	virtual bool get_g(void);
	virtual bool get_b(void);
	virtual void out(byte, byte);
};
