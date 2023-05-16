CC=gcc
INC=
CFLAGS=-O -Wall $(INC) `MagickWand-config --cflags`
#CFLAGS_OPT=-O3 -Wall $(INC) `MagickWand-config --cflags`
#CFLAGS=-g $(INC) `MagickWand-config --cflags`
CFLAGS_OPT=-g $(INC) `MagickWand-config --cflags`
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

main.o: main.c color.h control_point.h util.h heightmap.h image.h list.h

list.o: list.c control_point.h list.h

control_point.o: control_point.c control_point.h

image.o: image.c color.h perlin.h util.h image.h palette.h

heightmap.o: heightmap.c image.h util.h heightmap.h color.h

color.o: color.c color.h util.h

palette.o: palette.c palette.h color.h util.h

util.o: util.c util.h

perlin.o: perlin.c perlin.h util.h


ply2pov: ply2pov.o
	$(CC) -O -Wall -o ply2pov ply2pov.o

ply2pov.o: ply2pov.c
	$(CC) -O -Wall -c ply2pov.c
