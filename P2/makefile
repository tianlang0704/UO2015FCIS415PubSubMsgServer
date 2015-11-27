CC	:= gcc
CFLAGS	:= -g
OFLAGS	:= -g
LIBS	:= -pthread 
SRCS	:= $(wildcard *.c)
OBJS	:= $(SRCS:.c=.o)
EXEC	:= P2


$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJS) $(LIBS)
	rm -f *.o

%.o: %.c
	$(CC) $(OFLAGS) -o $@ -c $<

.Phony: clean

clean:
	rm -f $(OBJS) $(EXEC)
