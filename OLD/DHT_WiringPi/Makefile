IDIR =./include
CC=gcc
CFLAGS=-I$(IDIR) -O3 -std=gnu99 -Wall

ODIR=obj
LDIR =./lib

LIBS=-lwiringPi -lrt -pthread

_DEPS = getTemp.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = getTemp.o 
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

getTemp: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
