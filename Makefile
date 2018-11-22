CXX := g++

LDFLAGS := `pkg-config --static --libs gl glfw3 glew`

CXXFLAGS := -Wall -std=c++11 -Wno-unused-variable -Wno-unused-function

OBJ_DIR := obj
SRC_DIR := src
PRG_NAME := cover

PRG_SRCS := $(wildcard $(SRC_DIR)/*.cpp)
PRG_HEADERS := $(wildcard $(SRC_DIR)/*.h)
PRG_OBJS := $(PRG_SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS := $(PRG_OBJS:.o=.d)

RM := rm -rf

.PHONY: all clean run

all: $(PRG_NAME)

-include $(DEPS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -MMD -o $@ 

$(PRG_NAME): $(PRG_OBJS)
	$(CXX) $^ $(LDFLAGS) -o $@
	@echo
	@echo "----->	SUCCESS"

clean:
	-$(RM) $(PRG_NAME)
	-$(RM) $(OBJ_DIR)

run:
	@./$(PRG_NAME)
