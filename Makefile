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

sgcreate: sgcreate.o list.o control_point.o image.o heightmap.o color.o util.o perlin.o metrics.o color_ramp.o
	$(CC) $(LFLAGS) -o sgcreate sgcreate.o list.o control_point.o image.o heightmap.o color.o util.o perlin.o metrics.o color_ramp.o $(LIBS)

sgcreate.o: sgcreate.c image.h color_ramp.h metrics.h control_point.h heightmap.h util.h list.h color.h

list.o: list.c control_point.h list.h

control_point.o: control_point.c control_point.h

image.o: image.c color_ramp.h metrics.h perlin.h image.h color.h util.h

heightmap.o: heightmap.c color.h util.h color_ramp.h image.h metrics.h heightmap.h

color.o: color.c util.h color.h

util.o: util.c util.h

perlin.o: perlin.c perlin.h util.h

metrics.o: metrics.c metrics.h

color_ramp.o: color_ramp.c image.h metrics.h color_ramp.h color.h util.h
