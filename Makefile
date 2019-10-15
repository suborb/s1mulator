VERSION=0.5.0-alpha7
DDIR=s1mulator-$(VERSION)
CC = `wx-config --cc`
CXX = `wx-config --cxx`
CFLAGS  = -Wall -DLSB_FIRST -DDEBUG -DEXECZ80
CXXFLAGS = -Wall `wx-config --cxxflags` -DS1MULATOR_VERSION=\"$(VERSION)\" -DEXECZ80 -DDEBUG
LIBS = `wx-config --libs` `wx-config --gl-libs` $(EXTRA_LIBS)  -lGL
OFILES = main.o memory.o port.o display.o backlight.o dma.o nand.o runctrl.o hexview.o  Z80/Z80.o Z80/Debug.o


all: s1mulator

s1mulator: free $(OFILES) 
	$(CXX) -o s1mulator$(EXTENSION) $(OFILES) $(LIBS)

test.bin: test.asm
	z80asm -b test.asm

clean:  free-clean
	rm -f s1mulator s1mulator.exe *.o *.html
	$(RM) patch extract Z80-081707.zip

distclean: clean
	rm -rf $(DDIR)*

dist:	distclean
	svn export . $(DDIR)
	tar -c $(DDIR) > $(DDIR).tar
	gzip -f $(DDIR).tar
	./cross-mingw32.sh	
	mv s1mulator-tmp.zip $(DDIR).zip
	mv s1mulator-with-wx-tmp.zip $(DDIR)-with-wx.zip
	sed -e 's/VERSION/$(VERSION)/g' index.in > index.html
	sed -e 's/<a.j.buxton@gmail.com>//g' -e 's/</\&lt;/g' README.TXT >> index.html
	echo "</pre>\n</body>\n</html>\n" >> index.html

free: fetch extract	

Z80-081707.zip:
	wget http://fms.komkon.org/EMUL8/Z80-081707.zip

fetch:  Z80-081707.zip
	@touch fetch

extract:
	unzip -a Z80-081707.zip
	@touch extract

free-clean:
	$(RM) -r Z80
