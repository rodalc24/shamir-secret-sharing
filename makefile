######## VARS ###################################
CC 			:= gcc
CFLAGS	:= -Ofast
IFLAGS	:= -Iinclude
LDFLAGS := -lcrypto

SRC_DIR		:= src
INC_DIR 	:= include
OBJ_DIR 	:= obj
BIN_DIR 	:= bin
TST_DIR   := test

all: $(BIN_DIR)/sss $(TST_DIR)/test_aes $(TST_DIR)/test_sss $(TST_DIR)/test_share
.PHONY: clean clear

######## OBJS ###################################
$(OBJ_DIR)/main.o: $(SRC_DIR)/main.c $(INC_DIR)/aes.h $(INC_DIR)/sss.h
$(OBJ_DIR)/aes.o: $(SRC_DIR)/aes.c $(INC_DIR)/aes.h
$(OBJ_DIR)/test_aes.o: $(SRC_DIR)/test_aes.c $(INC_DIR)/aes.h
$(OBJ_DIR)/sss.o: $(SRC_DIR)/sss.c $(INC_DIR)/sss.h
$(OBJ_DIR)/test_sss.o: $(SRC_DIR)/test_sss.c $(INC_DIR)/sss.h
$(OBJ_DIR)/share.o: $(SRC_DIR)/share.c $(INC_DIR)/share.h
$(OBJ_DIR)/test_share.o: $(SRC_DIR)/test_share.c $(INC_DIR)/share.h
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(IFLAGS)

######## BINS ###################################
$(BIN_DIR)/sss: $(OBJ_DIR)/main.o $(OBJ_DIR)/aes.o $(OBJ_DIR)/sss.o $(OBJ_DIR)/share.o
	@mkdir -p $(BIN_DIR)
	$(CC) -o $@ $^ $(LDFLAGS) 

$(TST_DIR)/test_aes: $(OBJ_DIR)/test_aes.o $(OBJ_DIR)/aes.o
$(TST_DIR)/test_sss: $(OBJ_DIR)/test_sss.o $(OBJ_DIR)/sss.o
$(TST_DIR)/test_share: $(OBJ_DIR)/test_share.o $(OBJ_DIR)/share.o
$(TST_DIR)/%: $(OBJ_DIR)/%.o
	@mkdir -p $(TST_DIR)
	$(CC) -o $@ $^ $(LDFLAGS) 

######## CMDS ###################################
clean: 
	rm -rf $(OBJ_DIR)

clear: clean
	rm -rf $(BIN_DIR) $(TST_DIR)
