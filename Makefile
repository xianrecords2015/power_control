CC = g++
CFLAGS = -Wall -g -Wno-narrowing -DPAHO_BUILD_SAMPLES=FALSE

SRC = corinmqtt.cpp main.cpp
OBJ = $(SRC:.cpp=.o)

TARGET = power_control

LDFLAGS = -lpthread -lboost_filesystem -lboost_system -lpaho-mqtt3a -lpaho-mqtt3c -ljsoncpp -li2c -lbcm2835

INCLUDE = -I/usr/include/boost 
INCLUDE += -I/home/pi/spdlog/include
LIBRARY = -L/usr/lib/boost 

all: $(TARGET)

%.o: %.cpp
	$(CC) -c -o $@ $< $(INCLUDE) $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LIBRARY) $(LDFLAGS)

.PHONY: clean

clean:
	rm -f $(OBJ)
