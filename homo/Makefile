BIN_DIR=../bin
LIB_DIR=../SEAL/bin
INCLUDE_DIR=../SEAL/SEAL
CXX=g++-7
CXXFLAGS=-march=native -O3 -DHAVE_CONFIG_H -std=c++11 -pthread
LDFLAGS=-g

.PHONY : all clean

all : raymond dct encode test sender


raymond : raymond.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) fhe_image.h raymond.cpp $(addprefix -I,$(INCLUDE_DIR)) $(addprefix -L,$(LIB_DIR)) -lseal -o $(BIN_DIR)/raymond 

dct : dct.cpp
	@-mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) dct.cpp $(addprefix -I,$(INCLUDE_DIR)) $(addprefix -L,$(LIB_DIR)) -lseal -o $(BIN_DIR)/dct 

encode : encode.cpp
	@-mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) jpge.cpp encode.cpp $(addprefix -I,$(INCLUDE_DIR)) $(addprefix -L,$(LIB_DIR)) -lseal -o $(BIN_DIR)/encode 

test: test.cpp
	@-mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) test.cpp $(addprefix -I,$(INCLUDE_DIR)) $(addprefix -L,$(LIB_DIR)) -lseal -o $(BIN_DIR)/test

sender: test.cpp
	@-mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) sender.cpp $(addprefix -I,$(INCLUDE_DIR)) $(addprefix -L,$(LIB_DIR)) -lseal -o $(BIN_DIR)/sender


clean :
	@-rm -f ../bin/* 