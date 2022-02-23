TARGETS=libalgorithms-c.so libalgorithms-c.a

VERSION ?= 0.2.0
DEBUG ?=1
OPTIMIZE ?= -O2
CC=gcc -std=gnu99
LINKER=gcc -std=gnu99
AR=ar crf

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
OBJECTS_STATIC := $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o.static)


BASE_SOURCES := $(wildcard $(BASE_SRC_DIR)/*.c)
BASE_OBJECTS := $(BASE_SOURCES:$(BASE_SRC_DIR)/%.c=$(BASE_OBJ_DIR)/%.o)
BASE_OBJECTS_STATIC := $(BASE_SOURCES:$(BASE_SRC_DIR)/%.c=$(BASE_OBJ_DIR)/%.o.static)


all: do_init $(TARGETS)
	echo "objects: $(OBJECTS)"
	echo "static_objects: $(OBJECTS_STATIC)"

libalgorithms-c.so: $(LIB_DIR)/libalgorithms-c.so
$(LIB_DIR)/libalgorithms-c.so: $(LIB_DIR)/libalgorithms-c.so.$(VERSION)
	cd $(LIB_DIR); ln -fs libalgorithms-c.so.$(VERSION) libalgorithms-c.so
	
libalgorithms-c.a: $(LIB_DIR)/libalgorithms-c.a
$(LIB_DIR)/libalgorithms-c.a: $(LIB_DIR)/libalgorithms-c.a.$(VERSION)
	cd $(LIB_DIR); ln -fs libalgorithms-c.a.$(VERSION) libalgorithms-c.a

$(LIB_DIR)/libalgorithms-c.so.$(VERSION): $(OBJECTS) $(BASE_OBJECTS)
	$(LINKER) -shared -o $@ $^ $(LIBS)
	
$(LIB_DIR)/libalgorithms-c.a.$(VERSION): $(OBJECTS_STATIC) $(BASE_OBJECTS_STATIC)
	echo "$(AR) $@ $(OBJECTS_STATIC) $(BASE_OBJECTS_STATIC)"
	$(AR) $@ $(OBJECTS_STATIC) $(BASE_OBJECTS_STATIC)


$(OBJECTS): $(OBJ_DIR)/%.o : $(SRC_DIR)/%.c $(DEPS)
	$(CC) -fPIC -shared -o $@ -c $< $(CFLAGS)

$(OBJECTS_STATIC): $(OBJ_DIR)/%.o.static : $(SRC_DIR)/%.c $(DEPS)
	$(CC) -o $@ -c $< $(CFLAGS)
	

$(BASE_OBJECTS): $(BASE_OBJ_DIR)/%.o : $(BASE_SRC_DIR)/%.c $(DEPS)
	$(CC) -fPIC -shared -o $@ -c $< $(CFLAGS)

$(BASE_OBJECTS_STATIC): $(BASE_OBJ_DIR)/%.o.static : $(BASE_SRC_DIR)/%.c $(DEPS)
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: do_init clean tests demo samples
do_init:
	mkdir -p $(BIN_DIR) $(OBJ_DIR) $(BASE_OBJ_DIR) $(LIB_DIR)

clean:
	cd $(LIB_DIR); rm -f $(TARGETS) libalgorithms-c.so.$(VERSION) libalgorithms-c.a.$(VERSION)
	rm -f $(OBJ_DIR)/*.o $(OBJ_DIR)/*.o.static
	rm -f $(BASE_OBJ_DIR)/*.o $(BASE_OBJ_DIR)/*.o.static
	rm -f bin/samples-dijkstra bin/samples-dijkstra-static bin/dijkstra-demo 
	
tests:
	tests/make.sh

demo: do_init bin/dijkstra-demo

bin/dijkstra-demo: demo/Dijkstra-shortest-path.c
	$(LINKER) -o $@ $^ -lm 
	
samples: do_init bin/samples-dijkstra bin/samples-dijkstra-static

bin/samples-dijkstra-static: samples/samples-dijkstra.c $(LIB_DIR)/libalgorithms-c.a
	$(LINKER) -o $@ $^ $(CFLAGS) $(LIBS)
	
bin/samples-dijkstra: samples/samples-dijkstra.c $(LIB_DIR)/libalgorithms-c.so
	$(LINKER) -o $@ samples/samples-dijkstra.c $(CFLAGS) -Llib -Wl,-rpath=lib -lalgorithms-c $(LIBS)







