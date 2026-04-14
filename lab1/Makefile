CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -pedantic -pthread -Iinclude -I../lab0/include
TARGET := lab1_experiment
SRC_DIR := src
LAB0_SRC_DIR := ../lab0/src
OBJ_DIR := build

SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
LAB0_SOURCES := $(LAB0_SRC_DIR)/parser.cpp $(LAB0_SRC_DIR)/distance.cpp $(LAB0_SRC_DIR)/mst.cpp
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES)) \
	$(patsubst $(LAB0_SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(LAB0_SOURCES))

INPUT ?=
OUTPUT ?=
SEED ?=

.PHONY: all run clean help

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(LAB0_SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	@if [ -z "$(INPUT)" ]; then \
		echo "Usage: make run INPUT=path/to/file.tsp [OUTPUT=results.csv] [SEED=12345]"; \
		exit 1; \
	fi
	@if [ -n "$(OUTPUT)" ] && [ -n "$(SEED)" ]; then \
		./$(TARGET) "$(INPUT)" "$(OUTPUT)" "$(SEED)"; \
	elif [ -n "$(OUTPUT)" ]; then \
		./$(TARGET) "$(INPUT)" "$(OUTPUT)"; \
	elif [ -n "$(SEED)" ]; then \
		base="$(INPUT)"; \
		case "$$base" in \
			*.*) default_output="$${base%.*}_lab1_results.csv" ;; \
			*) default_output="$$base"_lab1_results.csv ;; \
		esac; \
		./$(TARGET) "$$base" "$$default_output" "$(SEED)"; \
	else \
		./$(TARGET) "$(INPUT)"; \
	fi

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

help:
	@echo "Targets:"
	@echo "  make                - build the program"
	@echo "  make run INPUT=...  - run on a TSPLIB .tsp file"
	@echo "  make clean          - remove the executable and objects"
	@echo ""
	@echo "Optional variables for make run:"
	@echo "  OUTPUT=results.csv"
	@echo "  SEED=12345"
