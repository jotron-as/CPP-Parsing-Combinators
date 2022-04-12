TARGET=libparser.so
PREFIX=/usr/local
INCLUDES_DIR=$(PREFIX)/include
LIB_DIR=$(PREFIX)/lib
CC=gcc
STD=c++17

all: $(TARGET)

$(TARGET): src/parser.cpp
	gcc -std=$(STD) -shared -fpic -o $@ $^

install: install-header $(LIB_DIR)/$(TARGET)

install-header: $(INCLUDES_DIR)/cpp_parser/parser.h

$(INCLUDES_DIR)/cpp_parser/parser.h: src/parser.h
	install -D -m 644 src/parser.h $(INCLUDES_DIR)/cpp_parser/parser.h

$(LIB_DIR)/$(TARGET): $(TARGET)
	install -D -m 755 $(TARGET) $(LIB_DIR)/$(TARGET)

demo: recursive network_proto

% : demo/%.cpp
	mkdir -p bin
	g++ -o bin/$@ -std=$(STD) $^ -L. -lparser -Wl,-rpath=.

clean:
	rm -fr bin *.so
