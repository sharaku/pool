# Makefile

.PHONY: all clean

CC       =g++
CFLAGS   =-Wall -ansi -pedantic -g -Wno-variadic-macros -Wno-pedantic
TARGET   =logviewr

SRC      =logvewer.c

HEADERS  =
INCLUDES =-I/usr/local/include/ -I../../../../include/
LDFLAGS  =-L/usr/local/lib/
LIBS     =

all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS) Makefile
	$(CC) -o $@ $(CFLAGS) $(SRC) $(INCLUDES) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)
