CXX = clang++
CXXFLAGS = -fsanitize=address -g -lpthread

SERVER_SRC = SocketServer.cpp 
SERVER_EXEC = SocketServer 

all: $(SERVER_EXEC) 

$(SERVER_EXEC): $(SERVER_SRC) 
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f *.o $(SERVER_EXEC)