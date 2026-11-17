CC = gcc
CFLAGS = -Wall -Wextra -Isrc
LIBS = -lraylib -lopengl32 -lgdi32 -lwinmm
SRC = $(wildcard src/*.c)
TARGET = jogo.exe

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)