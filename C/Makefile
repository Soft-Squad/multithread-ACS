OBJS	= ACS.o
SOURCE	= ACS.c
HEADER	= 
OUT	= ACS
CC	 = gcc
FLAGS	 = -g -c -Wall
LFLAGS	 = -lpthread

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

ACS.o: ACS.c
	$(CC) $(FLAGS) ACS.c 


clean:
	rm -f $(OBJS) $(OUT)