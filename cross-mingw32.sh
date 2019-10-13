#!/bin/sh
# This script will cross compile on Ubuntu Linux using the mingw32 packages.
#
# To set up a build environment: 
#
# sudo apt-get install mingw32 mingw32-binutils mingw32-runtime
# wget http://prdownloads.sourceforge.net/wxwindows/wxWidgets-2.6.3.tar.bz2
# tar -xvjpf wxWidgets-2.6.3.tar.bz2
# cd wxWidgets-2.6.3/
# ./configure --prefix=/opt/wxWidgets-2.6.3-msw --host=i586-mingw32msvc \
#    --target=i586-mingw32msvc --with-msw --with-opengl --enable-shared \
#    --enable-monolithic --disable-threads
# make
# sudo make install
#
# NOTE: You should NOT try to use this script if you are compiling
# native on win32, eg using djgpp or similar. 

WXPATH=/opt/wxWidgets-2.6.3-msw

PATH=$WXPATH/bin/:$PATH \
EXTRA_LIBS=-lopengl32 \
EXTENSION=.exe \
make

zip -j s1mulator-with-wx-tmp.zip s1mulator.exe $WXPATH/lib/*.dll \
	README.TXT Changelog.txt LICENSE test.bin test.asm 

zip -j s1mulator-tmp.zip s1mulator.exe \
        README.TXT Changelog.txt LICENSE test.bin test.asm
