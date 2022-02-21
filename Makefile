TARGETS=libalgorithms-c.so libalgorithms-c.a

VERSION ?= 0.1.0
DEBUG ?=1
OPTIMIZE ?= -O2
CC=gcc -std=gnu99
LINKER=gcc -std=gnu99
AR=ar crO

CFLAGS = -Wall -Iinclude -Iutils -Isrc -D_DEFAULT_SOURCE -D_GNU_SOURCE
LIBS = -lm -lpthread 

ifeq ($(DEBUG),1)
CFLAGS += -g -D_DEBUG
OPTIMIZE = -O0
endif

BIN_DIR=bin
LIB_DIR=lib
SRC_DIR=src
OBJ_DIR=obj
TEST_DIR=tests

BASE_SRC_DIR=src/base
BASE_OBJ_DIR=obj/base


DEPS = include/algorithms-c-common.h

SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
OBJECTS_STATIC := $(SOURCES:$(SRC_DIR)/%.c:$(OBJ_DIR)/%.o.static)


BASE_SOURCES := $(wildcard $(BASE_SRC_DIR)/*.c)
BASE_OBJECTS := $(BASE_SOURCES:$(BASE_SRC_DIR)/%.c=$(BASE_OBJ_DIR)/%.o)
BASE_OBJECTS_STATIC := $(BASE_SOURCES:$(BASE_SRC_DIR)/%.c:$(BASE_OBJ_DIR)/%.o.static)


all: do_init $(TARGETS)

libalgorithms-c.so: $(LIB_DIR)/libalgorithms.so.$(VERSION)
	cd $(LIB_DIR); ln -fs libalgorithms.so.$(VERSION) libalgorithms-c.so
	
libalgorithms-c.a: $(LIB_DIR)/libalgorithms.a.$(VERSION)
	cd $(LIB_DIR); ln -fs libalgorithms.a.$(VERSION) libalgorithms-c.a

$(LIB_DIR)/libalgorithms.so.$(VERSION): $(OBJECTS) $(BASE_OBJECTS)
	$(LINKER) -shared -mcmodel=large  -o $@ $^ $(LIBS)
	
$(LIB_DIR)/libalgorithms.a.$(VERSION): $(OBJECTS_STATIC) $(BASE_OBJECTS_STATIC)
	$(AR) $@ $^


$(OBJECTS): $(OBJ_DIR)/%.o : $(SRC_DIR)/%.c $(DEPS)
	$(CC) -fPIC -shared -o $@ -c $< $(CFLAGS)

$(OBJECTS_STATIC): $(OBJ_DIR)/%.o.static : $(SRC_DIR)/%.c $(DEPS)
	$(CC) -o $@ -c $< $(CFLAGS)
	

$(BASE_OBJECTS): $(BASE_OBJ_DIR)/%.o : $(BASE_SRC_DIR)/%.c $(DEPS)
	$(CC) -fPIC -shared -o $@ -c $< $(CFLAGS)

$(BASE_OBJECTS_STATIC): $(BASE_OBJ_DIR)/%.o.static : $(BASE_SRC_DIR)/%.c $(DEPS)
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: do_init clean tests demo
do_init:
	mkdir -p $(BIN_DIR) $(OBJ_DIR) $(BASE_OBJ_DIR) $(LIB_DIR)

clean:
	rm -f $(TARGETS) $(LIB_DIR)/libalgorithms.so.$(VERSION) $(LIB_DIR)/libalgorithms.a.$(VERSION)
	rm -f $(OBJ_DIR)/*.o $(OBJ_DIR)/*.o.static
	rm -f $(BASE_OBJ_DIR)/*.o $(BASE_OBJ_DIR)/*.o.static
	
tests:
	tests/make.sh

demo: do_init bin/dijkstra-demo

bin/dijkstra-demo: demo/Dijkstra-shortest-path.c
	$(LINKER) -o $@ $^ -lm 
	









