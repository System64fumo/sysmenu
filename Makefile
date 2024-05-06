CXXFLAGS=-march=native -mtune=native -Os -s -Wall

all: sysmenu

clean:
	rm sysmenu *.o

window.o: src/window.cpp
	g++ -c src/window.cpp -o window.o \
	$$(pkg-config gtkmm-4.0 --cflags --libs) \
	$$(pkg-config gtk4-layer-shell-0 --cflags --libs) \
	$(CXXFLAGS)

launcher.o: src/launcher.cpp
	g++ -c src/launcher.cpp -o launcher.o \
	$$(pkg-config gtkmm-4.0 --cflags --libs) \
	$(CXXFLAGS)

main.o: src/main.cpp
	g++ -c src/main.cpp -o main.o \
	$$(pkg-config gtkmm-4.0 --cflags --libs) \
	$$(pkg-config gtk4-layer-shell-0 --cflags --libs) \
	$(CXXFLAGS)

sysmenu: main.o window.o launcher.o
	g++ main.o window.o launcher.o -o sysmenu \
	$$(pkg-config gtkmm-4.0 --cflags --libs) \
	$$(pkg-config gtk4-layer-shell-0 --cflags --libs) \
	$(CXXFLAGS)

