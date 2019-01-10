OBJS	= gentest.o
SOURCE	= gentest.cpp
HEADER	= 
OUT	= gentest
CC	 = g++
FLAGS	 = -g -c -Wall
LFLAGS	 = 

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

gentest.o: gentest.cpp
	$(CC) $(FLAGS) gentest.cpp -std=c++17


clean:
	rm -f $(OBJS) $(OUT)
