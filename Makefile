CC = gcc

EXEC = hy345sh

SRC = hy345.c

all: $(EXEC)

$(EXEC): $(SRC)
	$(CC) -o $(EXEC) $(SRC)

clean:
	rm -f $(EXEC)

.PHONY: all clean