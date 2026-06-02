BIN_DIR = bin
OBJ_DIR = obj
SRC_DIR = src
TARGET = gv
LIBS = -lz -lGL -lGLEW -lglfw -lm
CFLAGS= -Wall -DLOG_LEVEL=0 -Iinclude

SRCS= $(wildcard $(SRC_DIR)/*.c)
OBJS= $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

all: clean $(BIN_DIR)/$(TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(OBJ_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(BIN_DIR)/$(TARGET): $(OBJS)
	mkdir -p $(BIN_DIR)
	gcc $(OBJS) -o $@ $(LIBS)


clean:
	rm -f $(BIN_DIR)/$(TARGET) $(OBJ_DIR)/*.o
