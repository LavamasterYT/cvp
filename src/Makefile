CC = gcc
CFLAGS = -Wall -Wextra -std=c11

LIBS = $(shell pkg-config --cflags --libs libswscale libavformat libavutil libavcodec libswresample sdl2 ao)
LDFLAGS = -lm

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
EXEC = cvp

$(EXEC): $(OBJ)
	$(CC) $(OBJ) $(LIBS) $(LDFLAGS) -o $(EXEC)
	rm -f $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(EXEC)
