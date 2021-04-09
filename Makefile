PREFIX=/usr
INCLUDES_DIR=$(PREFIX)/include

all:
	@echo Nothing to do

install:
	install -D -m 644 src/parser.h $(INCLUDES_DIR)/cpp_parser/parser.cpp

demo: recursive network_proto

% : demo/%.cpp
	mkdir -p bin;
	g++ -o bin/$@ -std=gnu++1z $^;

clean:
	rm main
