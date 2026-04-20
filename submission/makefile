CXX = g++
CXXFLAGS = -std=c++23 -O2 -Wall -Wextra

SERVER_SRCS = kayles_server server server_config server_args game
CLIENT_SRCS = kayles_client client client_config client_args

SERVER_EXEC = kayles_server
CLIENT_EXEC = kayles_client
EXECS = $(SERVER_EXEC) $(CLIENT_EXEC)

.PHONY: all clean server client

all: $(EXECS)

server: $(SERVER_EXEC)

client: $(CLIENT_EXEC)

$(SERVER_EXEC): $(SERVER_SRCS:%=%.cpp)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(CLIENT_EXEC): $(CLIENT_SRCS:%=%.cpp)
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(EXECS)
