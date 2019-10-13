INC_DIR=./inc
OBJ_DIR=./obj
LIB_DIR=./lib
SRC_DIR=./src
BIN_DIR=./bin

CC=gcc
RM=rm -f

TARGET=librwlock.a

CFLAGS=-I${INC_DIR}

${OBJ_DIR}/%.o : ${SRC_DIR}/%.c ${INC_DIR}/%.h
	${CC} ${CFLAGS} -c $< -o $@

${BIN_DIR}/$(TARGET) : ${OBJ_DIR}/rwlock.o
	ar rcs $@ ${OBJ_DIR}/*.o

path : 
	@echo "export RWLOCK_PATH=\`pwd\`"

all: ${BIN_DIR}/$(TARGET)

clean :
	$(RM) $(BIN_DIR)/* $(OBJ_DIR)/*
