TARGET = bin/dbview
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))

run: clean default test

test:
	./$(TARGET) -f ./foo.db -n
	./$(TARGET) -f ./foo.db -a "Timmy H.,123 Sheshire Ln.,129"
	./$(TARGET) -f ./foo.db -a "Kenny A.,34 Walloby Way,154" -l
	./$(TARGET) -f ./foo.db -l

default: $(TARGET)

clean:
	rm -f obj/*.o
	rm -f bin/*
	rm -f foo.db

$(TARGET): $(OBJ)
	gcc -o $@ $?

obj/%.o: src/%.c
	gcc -c $< -o $@ -Iinclude
