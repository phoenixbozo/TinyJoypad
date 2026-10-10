CC       = gcc
CFLAGS   = -O3 -Wall
CPPFLAGS = -I/usr/include/simavr -I/usr/local/include
LIBS     = -lsimavr -lelf -lSDL2 -lm -lpthread

SRC = src/main.c src/board.c src/ssd1306.c src/i2c_bb.c
OBJ = $(SRC:.c=.o)
TARGET = tinyjoypad_emu

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
