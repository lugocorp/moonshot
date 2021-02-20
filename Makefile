BUILD:=bin
SRC:=$(shell find src | grep -e "\.c")
OBJ:=$(patsubst src/%.c,$(BUILD)/%.o,$(SRC))
LIBNAME:=$(BUILD)/liblang.so

all: clean $(LIBNAME)

clean:
	rm -rf $(BUILD) ./test

$(BUILD):
	mkdir $(BUILD)

$(BUILD)/%.o: $(BUILD)
	gcc -c -fPIC src/$*.c -o $@

$(LIBNAME): $(OBJ)
	gcc -shared $(OBJ) -o $(LIBNAME)

test: $(LIBNAME)
	gcc -c tools/testing.c -o $(BUILD)/test.o
	gcc $(BUILD)/test.o $(LIBNAME) -o ./test
