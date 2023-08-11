#!/usr/bin/make -f

INC_DIR             := include
INC                 := $(shell find $(EXAMPLE_DIR) -type f -name "*.hpp")

EXAMPLE_DIR         := example
EXAMPLE_SRC         := $(shell find $(EXAMPLE_DIR) -type f -name "*.cpp")
EXAMPLE_BUILD_DIR   := $(EXAMPLE_DIR)/build
EXAMPLE_EXE         := $(patsubst $(EXAMPLE_DIR)/%.cpp,$(EXAMPLE_BUILD_DIR)/%,$(EXAMPLE_SRC))

WARN_FLAGS          := -Wall -Wextra -Wpedantic -Weffc++ -Wfloat-equal -Wshadow -Wcast-align -Wswitch-default -Winit-self -Wredundant-decls
CXXFLAGS            := -std=c++17 $(WARN_FLAGS) -DNDEBUG -O3

DOXYGEN_DIR         := doxygen
DOXYGEN_CONF        := $(DOXYGEN_DIR)/doxygen.conf
DOX                 := $(shell find $(DOXYGEN_DIR) -type f -name "*.dox")
DOC_DIR             := doc/

README_FILE         := README.md


.DEFAULT_GOAL       := help


.PHONY: all
all:    example doc



.PHONY: example
example:    $(EXAMPLE_EXE)

$(EXAMPLE_BUILD_DIR)/%:	$(EXAMPLE_DIR)/%.cpp $(INC_DIR)/*.hpp
	mkdir -p $(EXAMPLE_BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) -o $@ $<



.PHONY: doc
doc:    $(DOC_DIR)

$(DOC_DIR):	$(DOXYGEN_CONF) $(DOX) $(INC) $(EXAMPLE_SRC) $(README_FILE)
	mkdir -p $(DOC_DIR)
	doxygen $(DOXYGEN_CONF)
	touch $(DOC_DIR)



.PHONY: clean
clean:
	rm -rf $(EXAMPLE_BUILD_DIR) $(DOC_DIR)



.PHONY: help
help:
	@echo "available targets:"
	@printf "         %-14s %s\n" example "build examples"
	@printf "         %-14s %s\n" doc "make documentation"
	@printf "         %-14s %s\n" all "build examples and make documentation"
	@printf "         %-14s %s\n" clean "remove examples and documentation"
	@printf "         %-14s %s\n" help "print this help and exit (default target)"
