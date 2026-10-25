
CXX      ?= clang++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -pedantic
TARGET   := gc_engine
SRCDIR   := src
SOURCES  := $(SRCDIR)/main.cpp

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SOURCES) $(SRCDIR)/gc_engine.h $(SRCDIR)/http_server.h
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)
	@echo ""
	@echo "  ✓ Build complete: ./$(TARGET)"
	@echo "  Run with:  ./$(TARGET) [port]"
	@echo ""

run: $(TARGET)
	./$(TARGET) 8080

clean:
	rm -f $(TARGET)
