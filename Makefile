CC = gcc
CFLAGS := -g
TARGET=dimserv
BIN_DIR=bin
SRC_DIR=src


all: $(TARGET)

$(TARGET): $(SRC_DIR)/$(TARGET).c
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$(TARGET) $(SRC_DIR)/$(TARGET).c

clean:
	rm -rf $(BIN_DIR)/*

PHONY: clean
