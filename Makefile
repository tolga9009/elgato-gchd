CC = clang
CFLAGS = -std=c99 -Wall -Wextra
LDFLAGS = -lusb-1.0

SRCS = elgato-gchd.c
OBJS = $(SRCS:.c=.o)
EXEC = elgato-gchd

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $(EXEC)

clean:
	rm -f *.o $(EXEC)
