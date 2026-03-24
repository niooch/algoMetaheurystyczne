CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -pedantic -Iinclude
TARGET := tsp_experiment
SRC_DIR := src
OBJ_DIR := build

SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))

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
			*.*) default_output="$${base%.*}_results.csv" ;; \
			*) default_output="$$base"_results.csv ;; \
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
	@echo ""
	@echo "Example:"
	@echo "  make run INPUT=../dj38.tsp OUTPUT=../dj38_results.csv SEED=12345"
