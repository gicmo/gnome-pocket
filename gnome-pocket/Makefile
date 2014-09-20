all: test

OBJECT_FILES = test.o test-load.o gnome-pocket-item.o gnome-pocket.o
CFLAGS = -g -Wall `pkg-config --cflags rest-0.7 goa-1.0 json-glib-1.0 gom-1.0`
LIBS = `pkg-config --libs rest-0.7 goa-1.0 json-glib-1.0 gom-1.0`

test: test.o gnome-pocket-item.o gnome-pocket.o
	gcc -g -Wall -o test test.o gnome-pocket-item.o gnome-pocket.o $(LIBS)

clean:
	rm -f test *.o
