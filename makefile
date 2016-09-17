CC=g++
CFLAGS=-g -Wall
EXEC=server client

all:$(EXEC)

#静态模式
$(EXEC):%:%.o
	$(CC) $< -o $@ $(CFLAGS)
	
%.o:%.cpp
	$(CC) -c $< -o $@


.PHONY:clean
clean:
	rm -f *.o  $(EXEC)
