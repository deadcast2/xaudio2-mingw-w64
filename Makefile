CC = x86_64-w64-mingw32-gcc
CFLAGS = -std=c99
LDFLAGS = -mconsole
LDLIBS = -lkernel32 -lole32 -lxaudio2_8
RES = windres

main.exe: main.c resources.o
	$(CC) $(LDFLAGS) $(CFLAGS) main.c -o $@ resources.o $(LDLIBS)

resources.o:
	$(RES) resources.rc resources.o

clean:
	rm -f *.exe *.o
