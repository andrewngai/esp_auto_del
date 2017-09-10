.PHONY: build

EXE = esp_auto_del
build:
	g++ -o $(EXE) main.cpp -std=c++11 -l paho-mqtt3c -I/home/pi/json/src/
