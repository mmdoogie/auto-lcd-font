FT_CFLAGS = `pkg-config --cflags freetype2`
FT_LDFLAGS = `pkg-config --libs freetype2`

alf: alf.o
	gcc $(FT_LDFLAGS) alf.o -o alf

alf.o: alf.c
	gcc $(FT_CFLAGS) -c alf.c -o alf.o

clean:
	rm alf.o alf
