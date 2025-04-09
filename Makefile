CC = g++

GLLIBS = -lglfw -lGLEW -lGL -lm


all: main.cpp
	$(CC) -o main main.cpp $(GLLIBS)

clean:
	rm -f main
