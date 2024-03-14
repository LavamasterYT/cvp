CC = gcc
CFLAGS = -Wall -Wextra -std=c11

LIBS = $(shell pkg-config --cflags --libs libswscale libavformat libavutil libavcodec mpv)
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