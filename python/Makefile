LIB = `python-config --libs`
INCLUDES = `python-config --cflags`
INSTALLDIR = /usr/lib/python2.6/site-packages

all: gmediadb.so

gmediadb.so: gmediadb-python.c
	gcc -shared $(LIB) $(INCLUDES) -o gmediadb.so gmediadb-python.c -fPIC

install:
	cp gmediadb.so $(INSTALLDIR)/.
