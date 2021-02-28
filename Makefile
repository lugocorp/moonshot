BUILD:=bin
SRC:=$(shell find src | grep -e "\.c")
OBJ:=$(patsubst src/%.c,$(BUILD)/%.o,$(SRC))
LIBNAME:=$(BUILD)/liblang.so

all: clean $(LIBNAME)

clean:
	rm -rf $(BUILD)

$(BUILD):
	mkdir $(BUILD)

$(BUILD)/%.o: src/%.c $(BUILD)
	gcc -c -fPIC src/$*.c -o $@

$(LIBNAME): $(OBJ)
	gcc -shared $(OBJ) -o $(LIBNAME)

$(BUILD)/%: tools/%.c $(LIBNAME)
	gcc -c tools/$*.c -o $(BUILD)/$*.o
	gcc $(BUILD)/$*.o $(LIBNAME) -o $(BUILD)/$*
