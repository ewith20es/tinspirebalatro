CXX = nspire-g++
CXXFLAGS = -std=c++17 -marm -O2 -Wall -Wextra -Wno-misleading-indentation -fno-exceptions -fno-rtti -MMD -MP
OBJECTS = engine.o graphics.o app.o platform.o

all: balatro.tns

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

balatro.elf: $(OBJECTS)
	nspire-ld $(OBJECTS) -o $@

balatro.tns: balatro.elf
	genzehn --input $< --output balatro.zehn --name "Balatro" --240x320-support false
	make-prg balatro.zehn $@

desktop: build/balatro-desktop.exe

build/balatro-desktop.exe: engine.cpp graphics.cpp app.cpp platform.cpp engine.hpp graphics.hpp app.hpp
	mkdir -p build
	g++ -std=c++17 -O2 -Wall -Wextra -Wno-misleading-indentation -DBT_DESKTOP engine.cpp graphics.cpp app.cpp platform.cpp -lgdi32 -luser32 -o $@

test: build/tests.exe
	./build/tests.exe

build/tests.exe: tests.cpp engine.cpp graphics.cpp app.cpp engine.hpp graphics.hpp app.hpp
	mkdir -p build
	g++ -std=c++17 -O2 -Wall -Wextra -Wno-misleading-indentation engine.cpp graphics.cpp app.cpp tests.cpp -o $@

-include $(OBJECTS:.o=.d)
.PHONY: all desktop test
