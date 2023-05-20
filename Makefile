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

all: sgcreate

clean:
	rm -rf sgcreate *.o

sgcreate: main.o list.o control_point.o image.o heightmap.o color.o palette.o util.o perlin.o metrics.o
	$(CC) $(LFLAGS) -o sgcreate main.o list.o control_point.o image.o heightmap.o color.o palette.o util.o perlin.o metrics.o $(LIBS)

main.o: main.c util.h control_point.h heightmap.h image.h list.h metrics.h color.h

list.o: list.c control_point.h list.h

control_point.o: control_point.c control_point.h

image.o: image.c color.h image.h metrics.h util.h palette.h perlin.h

heightmap.o: heightmap.c color.h util.h heightmap.h metrics.h image.h

color.o: color.c util.h color.h

palette.o: palette.c util.h color.h palette.h

util.o: util.c util.h

perlin.o: perlin.c perlin.h util.h

metrics.o: metrics.c metrics.h
