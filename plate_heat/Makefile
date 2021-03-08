OBJS	= gentest.o gentestgals.o
HEADER	= 
OUT	= gentest gentestgals
CC	 = g++
FLAGS	 = -g -c -Wall
LFLAGS	 = 

all: gentest gentestgals

gentest: gentest.o
	$(CC) -g gentest.o -o gentest $(LFLAGS)

gentestgals: gentestgals.o
	$(CC) -g gentestgals.o -o gentestgals $(LFLAGS)

gentest.o: gentest.cpp
	$(CC) $(FLAGS) gentest.cpp -std=c++17

gentestgals.o: gentestgals.cpp
	$(CC) $(FLAGS) gentestgals.cpp -std=c++17

clean:
	rm -f $(OBJS) $(OUT)
