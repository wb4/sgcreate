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

sgcreate: main.o list.o control_point.o image.o heightmap.o color.o palette.o util.o perlin.o length.o
	$(CC) $(LFLAGS) -o sgcreate main.o list.o control_point.o image.o heightmap.o color.o palette.o util.o perlin.o length.o $(LIBS)

main.o: main.c heightmap.h control_point.h length.h util.h color.h list.h image.h

list.o: list.c control_point.h list.h

control_point.o: control_point.c control_point.h

image.o: image.c util.h image.h perlin.h color.h palette.h

heightmap.o: heightmap.c heightmap.h util.h color.h image.h

color.o: color.c util.h color.h

palette.o: palette.c palette.h util.h color.h

util.o: util.c util.h

perlin.o: perlin.c util.h perlin.h

length.o: length.c length.h
