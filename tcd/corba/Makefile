ORBIT=`orbit-config server --libs --cflags`
GNOME=`gnome-config gnome gnomeui gnorba --libs --cflags`

all: cdaudio-client cdaudio-server

cdaudio-skels.c: cdaudio.idl 
	orbit-idl cdaudio.idl

cdaudio-stubs.c: cdaudio.idl
	orbit-idl cdaudio.idl

cdaudio-common.c: cdaudio.idl
	orbit-idl cdaudio.idl

cdaudio-server: cdaudio-skels.c cdaudio-common.c cdaudio-server.c
	gcc -g -Wall $(ORBIT) $(GNOME) -o $@ $^
	-killall cdaudio-server

cdaudio-client: cdaudio-stubs.c cdaudio-common.c cdaudio-client.c
	gcc -g -Wall $(ORBIT) $(GNOME) -o $@ $^

clean:
	rm -f *~ *.o cdaudio-server cdaudio-client
