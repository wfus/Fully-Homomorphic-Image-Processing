BIN_DIR=../bin
LIB_DIR=../SEAL/bin
INCLUDE_DIR=../SEAL/SEAL
HEADER_FILES=../include/cxxopts.h
INCLUDE_DIR_2=../include
CXX=g++-7
CXXFLAGS=-march=native -O3 -DHAVE_CONFIG_H -std=c++11 -pthread
LDFLAGS=-g

.PHONY : all clean

all : raymond dct encode server client


raymond : raymond.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(HEADER_FILES) fhe_image.h raymond.cpp $(addprefix -I,$(INCLUDE_DIR)) $(addprefix -I,$(INCLUDE_DIR_2)) $(addprefix -L,$(LIB_DIR)) -lseal -o $(BIN_DIR)/raymond 

dct : dct.cpp
	@-mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(LDFLAGS)  $(HEADER_FILES) dct.cpp $(addprefix -I,$(INCLUDE_DIR)) $(addprefix -I,$(INCLUDE_DIR_2)) $(addprefix -L,$(LIB_DIR)) -lseal -o $(BIN_DIR)/dct 

encode : encode.cpp
	@-mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(LDFLAGS)  $(HEADER_FILES) jpge.cpp encode.cpp $(addprefix -I,$(INCLUDE_DIR)) $(addprefix -I,$(INCLUDE_DIR_2)) $(addprefix -L,$(LIB_DIR)) -lseal -o $(BIN_DIR)/encode 


client: client.cpp
	@-mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(LDFLAGS)  $(HEADER_FILES) jo_jpeg.h client.cpp $(addprefix -I,$(INCLUDE_DIR_2)) $(addprefix -I,$(INCLUDE_DIR)) $(addprefix -L,$(LIB_DIR)) -lseal -o $(BIN_DIR)/client

server: server.cpp
	@-mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(LDFLAGS)  $(HEADER_FILES) server.cpp $(addprefix -I,$(INCLUDE_DIR_2)) $(addprefix -I,$(INCLUDE_DIR)) $(addprefix -L,$(LIB_DIR)) -lseal -o $(BIN_DIR)/server


clean :
	@-rm -f ../bin/* 
