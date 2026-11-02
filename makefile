######## VARS ###################################
CC 			:= gcc
CFLAGS	:= -Ofast
IFLAGS	:= -Iinclude
LDFLAGS := -lgmp -lm

SRC_DIR		:= src
INC_DIR 	:= include
OBJ_DIR 	:= obj
BIN_DIR 	:= bin

all: $(BIN_DIR)/sss
.PHONY: clean clear

######## OBJS ###################################
$(OBJ_DIR)/main.o: $(SRC_DIR)/main.c $(INC_DIR)/aes.h
$(OBJ_DIR)/aes.o: $(SRC_DIR)/aes.c $(INC_DIR)/aes.h
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(IFLAGS)

######## BINS ###################################
$(BIN_DIR)/sss: $(OBJ_DIR)/main.o $(OBJ_DIR)/aes.o
$(BIN_DIR)/%: $(OBJ_DIR)/%.o
	@mkdir -p $(BIN_DIR)
	$(CC) -o $@ $^ $(LDFLAGS) 

######## CMDS ###################################
clean: 
	rm -rf $(OBJ_DIR)

clear: clean
	rm -rf $(BIN_DIR)
