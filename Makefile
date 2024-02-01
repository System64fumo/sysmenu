all: sysmenu

clean:
	rm sysmenu

sysmenu: main.cpp
	g++ main.cpp -o sysmenu $$(pkg-config gtkmm-4.0 --cflags --libs) $$(pkg-config gtk4-layer-shell-0 --cflags --libs) -O3
	strip sysmenu

