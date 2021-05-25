OBJS	= gentest.o gentestgals.o gentestalt.o
HEADER	= 
OUT	= gentest gentestgals gentestalt
CC	 = g++
FLAGS	 = -g -c -Wall
LFLAGS	 = 

all: gentest gentestgals gentestalt

gentest: gentest.o
	$(CC) -g gentest.o -o gentest $(LFLAGS)

gentestgals: gentestgals.o
	$(CC) -g gentestgals.o -o gentestgals $(LFLAGS)

gentestalt: gentestalt.o
	$(CC) -g gentestalt.o -o gentestalt $(LFLAGS)

gentest.o: gentest.cpp
	$(CC) $(FLAGS) gentest.cpp -std=c++17

gentestgals.o: gentestgals.cpp
	$(CC) $(FLAGS) gentestgals.cpp -std=c++17

gentestalt.o: gentestalt.cpp
	$(CC) $(FLAGS) gentestalt.cpp -std=c++17

clean:
	rm -f $(OBJS) $(OUT)
