CC = gcc
CFLAGS = -g -std=gnu11 -Werror -Wall -Wextra -Wpedantic -Wmissing-declarations -Wmissing-prototypes -Wold-style-definition
OBJ = mexec.o

mexec: $(OBJ)
		$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
		-rm *.o mexec
