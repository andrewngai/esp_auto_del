.PHONY: build

EXE = esp_maintain
build:
#	g++ -o $(EXE) main.cpp -std=c++11 -l paho-mqtt3c -I/home/pi/json/src/
	g++ -o $(EXE) main.cpp -std=c++11 -l paho-mqtt3c

install:
	sudo cp ./$(EXE) /usr/bin/$(EXE)

uninstall:
	sudo rm /usr/bin/$(EXE)
