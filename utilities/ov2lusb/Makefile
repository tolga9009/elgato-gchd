CXX = clang++
CXXFLAGS = -std=c++11 -Wall -Wextra
LDFLAGS =

SRCS = ov2lusb.cpp
OBJS = $(SRCS:.c=.o)
EXEC = ov2lusb

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) -o $(EXEC)

clean:
	rm -f *.o $(EXEC)
