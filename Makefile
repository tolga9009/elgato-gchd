CXX = clang++
CXXFLAGS = -std=c++11 -Wall -Wextra
LDFLAGS = -lusb-1.0

SRCS = elgato-gchd.cpp
OBJS = $(SRCS:.c=.o)
EXEC = elgato-gchd

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) -o $(EXEC)

clean:
	rm -f *.o $(EXEC)
