PROJECT_NAME := server
CXX := g++
CXXFLAGS := -Wall -Wextra -std=c++11
SRC_DIR := src/elevator_server
BUILD_DIR := build

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all clean

all: $(PROJECT_NAME)

$(PROJECT_NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

clean:
	$(RM) -r $(BUILD_DIR) $(PROJECT_NAME)
