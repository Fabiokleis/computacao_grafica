CC = g++

GLLIBS = -lglfw -lGLEW -lGL


all: main.cpp
	$(CC) -o main main.cpp $(GLLIBS)

clean:
	rm -f main
