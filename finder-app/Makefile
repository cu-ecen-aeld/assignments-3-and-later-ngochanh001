TARGET = writer

SRC = writer.c

CC = $(CROSS_COMPILE)gcc

OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) -c -o $@ $<

clean:
	rm -f $(TARGET) *.o
