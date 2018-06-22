# Makefile

.PHONY: all clean

CC       =g++
CFLAGS   =-Wall -ansi -pedantic -g
TARGET   =libwq.bit-unittest

SRC      =gtest_bit.cpp

HEADERS  =
INCLUDES =-I/usr/local/include/ -I../../../../include/
LDFLAGS  =-L/usr/local/lib/ -L../../
LIBS     =-pthread -lgtest -lgtest_main

all: $(TARGET) test

$(TARGET): $(SRC) $(HEADERS) Makefile
	$(CC) -o $@ $(CFLAGS) $(SRC) $(INCLUDES) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)

test: $(TARGET)
	./$(TARGET) --gtest_output=xml:gtest_result.bit.linux-x86.xml
