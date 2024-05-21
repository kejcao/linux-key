CXX = g++
CXXFLAGS = -pedantic -Wall -std=c++23
TARGET = listen
SRC = main.cpp
OBJ = $(SRC:.cpp=.o)
LIBS = -ludev

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $(TARGET) $(LIBS)

$(OBJ): $(SRC)
	$(CXX) $(CXXFLAGS) -c $(SRC)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean

