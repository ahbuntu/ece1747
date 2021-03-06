####################################################################################################
##
## CONFIGURATION
##
####################################################################################################

SHELL = /bin/bash

## Compilation parameters

CC = gcc
CXX = g++
CXXFLAGS := `sdl-config --cflags` -I./src -Wall -g
## Other CXXFLAGS: (also defined in src/Settings.h)
## -D__DISABLE_RATE_MONITOR__	<== don't monitor trasnfer: faster transfer but no statistics
## -D__COMPRESSED_MESSAGES__	<== update messages from server to clients are compressed with zlib
## -D__MESSAGE_BUFFER__		<== each thread stores messages in a private buffer and periodicaly
## 				transfers them to the main queue
## -D__SERVER_PLAYER_RATIO__	<== each player has a coefficient depending on its speed (works well
##				only if the server has one regular update thread)
## -D__USE_3DS_GRAPHICS__	<== use 3ds models instead of vrml for client GUI
LDFLAGS := `sdl-config --libs` -lSDL_net -lm
## -lz is also needed when compiling with -D__COMPRESSED_MESSAGES__
LDFLAGS_CLIENT = $(LDFLAGS) -lGL -lGLU
LDFLAGS_SERVER = $(LDFLAGS)

## Source and binary files

BUILD_DIR = build
SOURCE_DIR = src

SHARED_SOURCES   = $(wildcard $(SOURCE_DIR)/*.cpp) 	     $(wildcard $(SOURCE_DIR)/utils/*.cpp)
GAME_SOURCES_CPP = $(wildcard $(SOURCE_DIR)/game/*.cpp)
GAME_SOURCES_C   = $(wildcard $(SOURCE_DIR)/game/*.c)
COMM_SOURCES     = $(wildcard $(SOURCE_DIR)/comm/*.cpp)
GRAPHICS_SOURCES = $(wildcard $(SOURCE_DIR)/graphics/*.cpp) \
	$(wildcard $(SOURCE_DIR)/graphics/vrml/*.cpp) \
	$(wildcard $(SOURCE_DIR)/graphics/3ds/*.cpp) \
	$(wildcard $(SOURCE_DIR)/graphics/texture/*.cpp) \
	$(wildcard $(SOURCE_DIR)/graphics/font/*.cpp)

## SERVER
SERVER_DIR  = $(SOURCE_DIR)/server
SERVER_SRCS_CPP = $(SHARED_SOURCES) $(GAME_SOURCES_CPP) $(COMM_SOURCES) $(wildcard $(SERVER_DIR)/*.cpp)
SERVER_SRCS_C   = $(GAME_SOURCES_C)
SERVER_OBJS = $(patsubst $(SOURCE_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SERVER_SRCS_CPP)) \
              $(patsubst $(SOURCE_DIR)/%.c,$(BUILD_DIR)/%.o,$(SERVER_SRCS_C))

## CLIENT
CLIENT_DIR  = $(SOURCE_DIR)/client
CLIENT_SRCS_CPP = $(SHARED_SOURCES) $(GRAPHICS_SOURCES) $(GAME_SOURCES_CPP) $(COMM_SOURCES) \
				$(wildcard $(CLIENT_DIR)/*.cpp) $(wildcard $(CLIENT_DIR)/AStar/*.cpp) 	
CLIENT_SRCS_C   = $(GAME_SOURCES_C)
CLIENT_OBJS = $(patsubst $(SOURCE_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CLIENT_SRCS_CPP))	\
              $(patsubst $(SOURCE_DIR)/%.c,$(BUILD_DIR)/%.o,$(CLIENT_SRCS_C))

SERVER_BINARY_NAME = server
CLIENT_BINARY_NAME = client

## Parameters for running

MASTER_CONFIG_FILE = config_lightest.ini
HOST_NAME := `uname -n`

####################################################################################################
##
## BUILD
##
####################################################################################################

all: client server

## Create directories for object files
$(BUILD_DIR):
	@echo "Creating directories for the object files"
	@mkdir $(BUILD_DIR)
	@mkdir $(BUILD_DIR)/utils
	@mkdir $(BUILD_DIR)/server
	@mkdir $(BUILD_DIR)/client
	@mkdir $(BUILD_DIR)/client/AStar
	@mkdir $(BUILD_DIR)/comm
	@mkdir $(BUILD_DIR)/game
	@mkdir $(BUILD_DIR)/graphics
	@mkdir $(BUILD_DIR)/graphics/vrml
	@mkdir $(BUILD_DIR)/graphics/3ds
	@mkdir $(BUILD_DIR)/graphics/texture
	@mkdir $(BUILD_DIR)/graphics/font

## Build client and server

graphics: $(patsubst $(SOURCE_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(GRAPHICS_SOURCES))
game: $(patsubst $(SOURCE_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(GAME_SOURCES))
shared: $(patsubst $(SOURCE_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SHARED_SOURCES))
comm: $(patsubst $(SOURCE_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(COMM_SOURCES))
client_only: $(CLIENT_OBJS)
server_only: $(SERVER_OBJS)

client: $(BUILD_DIR) $(CLIENT_OBJS)
	$(CXX) -o $(CLIENT_BINARY_NAME) $(CLIENT_OBJS) $(LDFLAGS_CLIENT)
server: $(BUILD_DIR) $(SERVER_OBJS)
	$(CXX) -o $(SERVER_BINARY_NAME) $(SERVER_OBJS) $(LDFLAGS_SERVER)

## Target for generic cpp files
$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

## Target for generic c files
$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c
	$(CC) -c $(CXXFLAGS) -o $@ $<


## Call another makefile to build the game monitor
gamemonitor:
	make -C src/gamemonitor

## Clean
.PHONY: clean
clean:
	-rm -f -R $(BUILD_DIR)
	-rm -f $(SERVER_BINARY_NAME) $(CLIENT_BINARY_NAME)
	-rm -f core*
clean_all: clean
	-make -C src/gamemonitor clean

####################################################################################################
##
## RUN
##
####################################################################################################

PORT=3000

## run
run_server: server
	./$(SERVER_BINARY_NAME) $(MASTER_CONFIG_FILE) $(PORT)

run_10: client
	for (( i=1; i<=10; i++ )); do ./$(CLIENT_BINARY_NAME) $(HOST_NAME):$(PORT) > /dev/null & echo -n $$i" "; done; echo ""
run_100: client
	for (( i=1; i<=100; i++ )); do ./$(CLIENT_BINARY_NAME) $(HOST_NAME):$(PORT) > /dev/null & echo -n $$i" "; done; echo ""
run_200: client
	for (( i=1; i<=200; i++ )); do ./$(CLIENT_BINARY_NAME) $(HOST_NAME):$(PORT) > /dev/null & echo -n $$i" "; done; echo ""
run_400: client
	for (( i=1; i<=400; i++ )); do ./$(CLIENT_BINARY_NAME) $(HOST_NAME):$(PORT) > /dev/null & echo -n $$i" "; done; echo ""
run_1000: client
	for (( i=1; i<=1000; i++ )); do ./$(CLIENT_BINARY_NAME) $(HOST_NAME):$(PORT) > /dev/null & echo -n $$i" "; done; echo ""

run_client: client
	./$(CLIENT_BINARY_NAME) $(HOST_NAME):$(PORT)
run_client_gui: client
	./$(CLIENT_BINARY_NAME) --gui $(HOST_NAME):$(PORT)

## client
## --gui --nogui --size=640x480 --bpp=32 --fullscreen --debug-AI --disconnect_timeout=5
## --console-fps --fps-average-param=0.99
