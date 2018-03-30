BIN_DIR=../bin
LIB_DIR=../SEAL/bin
INCLUDE_DIR=../SEAL/SEAL
CXX=g++-7
CXXFLAGS=-march=native -O3 -DHAVE_CONFIG_H -std=c++11 -pthread
LDFLAGS=-g

.PHONY : all clean

all : raymond dct encode


raymond : raymond.cpp
	$(CXX) raymond.cpp $(CXXFLAGS) $(LDFLAGS) $(addprefix -I,$(INCLUDE_DIR)) $(addprefix -L,$(LIB_DIR)) -lseal -o $(BIN_DIR)/raymond 

dct : dct.cpp
	@-mkdir -p $(dir $@)
	$(CXX) dct.cpp $(CXXFLAGS) $(LDFLAGS) $(addprefix -I,$(INCLUDE_DIR)) $(addprefix -L,$(LIB_DIR)) -lseal -o $(BIN_DIR)/dct 

encode : encode.cpp
	@-mkdir -p $(dir $@)
	$(CXX) jpge.cpp encode.cpp $(CXXFLAGS) $(LDFLAGS) $(addprefix -I,$(INCLUDE_DIR)) $(addprefix -L,$(LIB_DIR)) -lseal -o $(BIN_DIR)/encode 


clean :
	@-rm -f ../bin/* 