CXX = g++
CXX_CFLAGS = -Wall
EXEC = qt428
SRC = $(wildcard *.cpp)
OBJS = $(SRC:.cpp=.o)

.SUFFIXES: .cpp .o

.cpp.o:
	$(CXX) -c $(CXX_CFLAGS) $<

$(EXEC): $(OBJS)
	$(CXX) $(CXX_CFLAGS) -o $@ $^

test:
	./$(EXEC) 192.168.155.6 | mplayer --really-quiet -

clean:
	rm -f $(EXEC)
	rm -f $(OBJS)
