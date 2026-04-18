CXX = g++
CXXFLAGS = -std=c++23 -O2 -Wall -Wextra

SERVER_SRCS = $(wildcard server/*.cpp) $(wildcard common/*.cpp)
CLIENT_SRCS = $(wildcard client/*.cpp) $(wildcard common/*.cpp)

SERVER_EXEC = kayles_server
CLIENT_EXEC = kayles_client
EXECS = $(SERVER_EXEC) $(CLIENT_EXEC)

.PHONY: all clean server client

all: $(EXECS)

server: $(SERVER_EXEC)

client: $(CLIENT_EXEC)

$(SERVER_EXEC): $(SERVER_SRCS)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(CLIENT_EXEC): $(CLIENT_SRCS)
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(EXECS)
