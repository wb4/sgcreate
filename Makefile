CC=gcc
INC=
CFLAGS=-O -Wall $(INC) `MagickWand-config --cflags`
#CFLAGS=-pg $(INC) `MagickWand-config --cflags`
LIBS=`MagickWand-config  --ldflags --libs` -lm
LFLAGS=
#LFLAGS=-pg

.SUFFIX: .c .o

.c.o:
	$(CC) $(CFLAGS) -c $<

all: sgcreate ply2pov

clean:
	rm -rf sgcreate ply2pov *.o

sgcreate: main.o list.o control_point.o image.o heightmap.o color.o palette.o util.o perlin.o
	$(CC) $(LFLAGS) -o sgcreate main.o list.o control_point.o image.o heightmap.o color.o palette.o util.o perlin.o $(LIBS)

main.o: main.c list.h control_point.h color.h util.h heightmap.h image.h

list.o: list.c control_point.h list.h

control_point.o: control_point.c control_point.h

image.o: image.c perlin.h palette.h util.h image.h color.h

heightmap.o: heightmap.c heightmap.h util.h image.h color.h

color.o: color.c color.h util.h

palette.o: palette.c palette.h color.h util.h

util.o: util.c util.h

perlin.o: perlin.c util.h perlin.h


ply2pov: ply2pov.o
	$(CC) -O -Wall -o ply2pov ply2pov.o

ply2pov.o: ply2pov.c
	$(CC) -O -Wall -c ply2pov.c
