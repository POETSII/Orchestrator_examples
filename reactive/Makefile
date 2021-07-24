OBJS	= reactiveGen.o
HEADER	= 
OUT	= reactiveGen
CC	 = g++
FLAGS	 = -g -c -Wall
LFLAGS	 = 

all: reactiveGen

reactiveGen: reactiveGen.o
	$(CC) -g reactiveGen.o -o reactiveGen $(LFLAGS)

reactiveGen.o: reactiveGen.cpp
	$(CC) $(FLAGS) reactiveGen.cpp -std=c++17

clean:
	rm -f $(OBJS) $(OUT)
