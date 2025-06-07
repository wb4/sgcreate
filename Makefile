CC=gcc
INC=
CFLAGS=-O -Wall $(INC) `pkg-config --cflags MagickWand`
#CFLAGS=-pg $(INC) `pkg-config --cflags MagickWand`
LIBS=`pkg-config --libs MagickWand` -lm
LFLAGS=
#LFLAGS=-pg

.SUFFIX: .c .o

.c.o:
	$(CC) $(CFLAGS) -c $<

all: sgcreate

clean:
	rm -rf sgcreate *.o

sgcreate: sgcreate.o list.o control_point.o image.o heightmap.o color.o palette.o util.o perlin.o metrics.o
	$(CC) $(LFLAGS) -o sgcreate sgcreate.o list.o control_point.o image.o heightmap.o color.o palette.o util.o perlin.o metrics.o $(LIBS)

sgcreate.o: sgcreate.c image.h list.h heightmap.h util.h control_point.h color.h metrics.h

list.o: list.c list.h control_point.h

control_point.o: control_point.c control_point.h

image.o: image.c palette.h util.h metrics.h color.h image.h perlin.h

heightmap.o: heightmap.c heightmap.h metrics.h util.h image.h color.h

color.o: color.c util.h color.h

palette.o: palette.c palette.h color.h util.h

util.o: util.c util.h

perlin.o: perlin.c util.h perlin.h

metrics.o: metrics.c metrics.h

color_ramp.o: color_ramp.c util.h color_ramp.h color.h image.h metrics.h
