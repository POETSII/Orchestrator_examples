OBJS	= arithGen.o
HEADER	= 
OUT	= arithGen
CC	 = g++
FLAGS	 = -g -c -Wall
LFLAGS	 = 

all: arithGen

arithGen: arithGen.o
	$(CC) -g arithGen.o -o arithGen $(LFLAGS)

arithGen.o: arithGen.cpp
	$(CC) $(FLAGS) arithGen.cpp -std=c++17

clean:
	rm -f $(OBJS) $(OUT)
