CC = clang
CCFLAGS = -Wall -Wextra
LDFLAGS = -lusb-1.0

SRCS = elgato-gchd.c
OBJS = $(SRCS:.c=.o)
EXEC = elgato-gchd

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CCFLAGS) $(OBJS) $(LDFLAGS) -o $(EXEC)

clean:
	rm -f *.o $(EXEC)
