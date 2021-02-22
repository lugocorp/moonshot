BUILD:=bin
SRC:=$(shell find src | grep -e "\.c")
OBJ:=$(patsubst src/%.c,$(BUILD)/%.o,$(SRC))
LIBNAME:=$(BUILD)/liblang.so

all: clean $(LIBNAME)

clean:
	rm -rf $(BUILD)

$(BUILD):
	mkdir $(BUILD)

$(BUILD)/%.o: $(BUILD)
	gcc -c -fPIC src/$*.c -o $@

$(LIBNAME): $(OBJ)
	gcc -shared $(OBJ) -o $(LIBNAME)

$(BUILD)/cli: $(LIBNAME)
	gcc -c tools/cli.c -o $(BUILD)/cli.o
	gcc $(BUILD)/cli.o $(LIBNAME) -o $(BUILD)/cli
