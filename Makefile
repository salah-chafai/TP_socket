CC = gcc
CFLAGS = -Wall -Wextra -g -I./src
SRC_DIR = src
BIN_DIR = bin

all: client middle_man server

client: $(SRC_DIR)/client.c $(SRC_DIR)/shared.c
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $(BIN_DIR)/$@

middle_man: $(SRC_DIR)/middle_man.c $(SRC_DIR)/shared.c
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $(BIN_DIR)/$@

server: $(SRC_DIR)/server.c $(SRC_DIR)/shared.c
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $(BIN_DIR)/$@

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean
