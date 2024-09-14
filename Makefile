CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O3 $(shell pkg-config --cflags libswscale libavformat libavutil libavcodec libswresample sdl2 ao)

LIBS = $(shell pkg-config --libs libswscale libavformat libavutil libavcodec libswresample sdl2 ao)
LDFLAGS = -lm -lcrypto

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
EXEC = cvp

$(EXEC): $(OBJ)
	$(CC) $(OBJ) $(LIBS) $(LDFLAGS) -o $(EXEC)
	rm -f $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(EXEC)
