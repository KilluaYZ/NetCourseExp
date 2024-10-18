CXX = clang++
CXXFLAGS = -fsanitize=address -g

SERVER_SRC = myserver.cpp
SOCKET_SRC = Socket.hpp
SERVER_EXEC = myserver

all: $(SERVER_EXEC) $(CLIENT_EXEC)

$(SERVER_EXEC): $(SERVER_SRC) 
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f *.o $(SERVER_EXEC)