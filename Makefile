CC = clang
CFLAGS = -std=c99 -Wall -Wextra
LDFLAGS = -lusb-1.0

SRCS = elgato-gchd.c commands.c init.c init_720p.c init_1080p.c init_576i.c init_component_576p.c init_component_720p.c init_component_1080i.c init_component_1080p.c remove.c
OBJS = $(SRCS:.c=.o)
EXEC = elgato-gchd

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $(EXEC)

clean:
	rm -f *.o $(EXEC)
