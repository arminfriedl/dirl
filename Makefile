# See LICENSE file for copyright and license details
# dirl - customizable directory listings based on quark
.POSIX:

include config.mk

COMPONENTS = data http sock util dirl

all: dirl

main.o: main.c util.h sock.h http.h arg.h config.h
http.o: http.c http.h util.h http.h data.h config.h
data.o: data.c data.h util.h http.h dirl.h
dirl.o: dirl.c dirl.h util.h http.h
sock.o: sock.c sock.h util.h
util.o: util.c util.h

dirl: $(COMPONENTS:=.o) $(COMPONENTS:=.h) main.o config.mk
	$(CC) -o $@ $(CPPFLAGS) $(CFLAGS) $(COMPONENTS:=.o) main.o $(LDFLAGS)

config.h:
	cp config.def.h $@

clean:
	rm -f dirl main.o $(COMPONENTS:=.o)
