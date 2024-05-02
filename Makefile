-include Makefile.inc
BASE_DIR=$(shell pwd)
SRC_DIR=$(BASE_DIR)/src
BUILD_DIR?=$(BASE_DIR)/build
RUNTIME_DIR?=$(BASE_DIR)/runtime
SCRATCH_DIR?=$(BASE_DIR)/scratch
TEST_DIR?=$(BASE_DIR)/test
INCLUDE_DIR=$(BASE_DIR)/include
BUILDIT_DIR?=$(BASE_DIR)/buildit

INCLUDES=$(wildcard $(INCLUDE_DIR)/*.h) $(wildcard $(INCLUDE_DIR)/*/*.h) $(wildcard $(BUILDIT_DIR)/include/*.h) $(wildcard $(BUILDIT_DIR)/include/*/*.h)
RUNTIME_INCLUDES=$(wildcard $(RUNTIME_DIR)/*.h) 
INCLUDE_FLAG=-I$(INCLUDE_DIR) -I$(BUILDIT_DIR)/include

SRCS=$(wildcard $(SRC_DIR)/core/*.cpp) $(wildcard $(SRC_DIR)/modules/*.cpp)
OBJS=$(subst $(SRC_DIR),$(BUILD_DIR),$(SRCS:.cpp=.o))


$(shell mkdir -p $(SCRATCH_DIR))
$(shell mkdir -p $(BUILD_DIR))
$(shell mkdir -p $(BUILD_DIR)/core)
$(shell mkdir -p $(BUILD_DIR)/impls)
$(shell mkdir -p $(BUILD_DIR)/modules)
$(shell mkdir -p $(BUILD_DIR)/runtime)
$(shell mkdir -p $(BUILD_DIR)/test)
$(shell mkdir -p $(BUILD_DIR)/test/simple_network_test)
$(shell mkdir -p $(BUILD_DIR)/test/simple_test)
$(shell mkdir -p $(BUILD_DIR)/test/simple_test_fragmentation)
$(shell mkdir -p $(BUILD_DIR)/test/simple_test_accept)
$(shell mkdir -p $(BUILD_DIR)/test/simple_test_inorder)
$(shell mkdir -p $(BUILD_DIR)/test/simple_test_reliable)
$(shell mkdir -p $(BUILD_DIR)/test/simple_test_posix)
$(shell mkdir -p $(BUILD_DIR)/test/simple_test_linux)
$(shell mkdir -p $(BUILD_DIR)/runtime/mlx5_impl)
$(shell mkdir -p $(BUILD_DIR)/runtime/posix)
$(shell mkdir -p $(BUILD_DIR)/utils)

BUILDIT_LIBRARY_NAME=buildit
BUILDIT_LIBRARY_PATH=$(BUILDIT_DIR)/build

LIBRARY_NAME=net_blocks
DEBUG ?= 0
ifeq ($(DEBUG),1)
CFLAGS=-g -std=c++11 -O0
LINKER_FLAGS=-rdynamic  -g -L$(BUILDIT_LIBRARY_PATH) -L$(BUILD_DIR) -l$(LIBRARY_NAME) -l$(BUILDIT_LIBRARY_NAME) -llz4
else
CFLAGS=-std=c++11 -O3
LINKER_FLAGS=-rdynamic  -L$(BUILDIT_LIBRARY_PATH) -L$(BUILD_DIR) -l$(LIBRARY_NAME) -l$(BUILDIT_LIBRARY_NAME) -llz4
endif


ifeq ($(DEBUG),1)
RCFLAGS=-g
else
RCFLAGS=-O3 -fno-strict-aliasing
endif




LIBRARY=$(BUILD_DIR)/lib$(LIBRARY_NAME).a

CFLAGS+=-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wmissing-declarations -Woverloaded-virtual -pedantic-errors -Wno-deprecated -Wdelete-non-virtual-dtor -Werror -fno-move-loop-invariants

all: executables $(LIBRARY)

.PHONY: dep
dep:
	make -C $(BUILDIT_DIR)

.PRECIOUS: $(BUILD_DIR)/core/%.o
.PRECIOUS: $(BUILD_DIR)/modules/%.o
.PRECIOUS: $(BUILD_DIR)/impls/%.o


.PHONY: $(BUILDIT_LIBRARY_PATH)/lib$(BUILDIT_LIBRARY_NAME).a


.PHONY: executables
executables: $(BUILD_DIR)/impls/simple


$(LIBRARY): $(OBJS)
	ar rv $(LIBRARY) $(OBJS)	
	
$(BUILD_DIR)/core/%.o: $(SRC_DIR)/core/%.cpp $(INCLUDES)
	$(CXX) $(CFLAGS) $< -o $@ $(INCLUDE_FLAG) -c
$(BUILD_DIR)/modules/%.o: $(SRC_DIR)/modules/%.cpp $(INCLUDES)
	$(CXX) $(CFLAGS) $< -o $@ $(INCLUDE_FLAG) -c
$(BUILD_DIR)/impls/%.o: $(SRC_DIR)/impls/%.cpp $(INCLUDES)
	$(CXX) $(CFLAGS) $< -o $@ $(INCLUDE_FLAG) -c

$(BUILD_DIR)/impls/simple: $(BUILD_DIR)/impls/simple.o $(LIBRARY)
	$(CXX) -o $@ $< $(LINKER_FLAGS)


# Runtime objs
$(SCRATCH_DIR)/nb_simple.c: $(BUILD_DIR)/impls/simple
		$(BUILD_DIR)/impls/simple $(SCRATCH_DIR)/gen_headers.h $(SCRATCH_DIR)/nb_simple.c 2> $(SCRATCH_DIR)/proto.txt
	cat $(SCRATCH_DIR)/proto.txt

$(BUILD_DIR)/runtime/nb_simple.o: $(SCRATCH_DIR)/nb_simple.c $(RUNTIME_INCLUDES)
	$(CC) $(RCFLAGS) $(SCRATCH_DIR)/nb_simple.c -o $(BUILD_DIR)/runtime/nb_simple.o -c -I $(RUNTIME_DIR) -I $(SCRATCH_DIR)


$(BUILD_DIR)/runtime/nb_runtime_simple.o: $(RUNTIME_DIR)/nb_runtime.c $(RUNTIME_INCLUDES) $(SCRATCH_DIR)/nb_simple.c
	$(CC) $(RCFLAGS) -o $@ $< -c -I $(SCRATCH_DIR) -I $(RUNTIME_DIR)


$(BUILD_DIR)/runtime/nb_timer.o: $(RUNTIME_DIR)/nb_timer.c $(RUNTIME_INCLUDES)
	$(CC) $(RCFLAGS) -o $@ $< -c -I $(SCRATCH_DIR) -I $(RUNTIME_DIR)


$(BUILD_DIR)/runtime/nb_compression.o: $(RUNTIME_DIR)/nb_compression.c $(RUNTIME_INCLUDES)
	$(CC) $(RCFLAGS) -o $@ $< -c -I $(SCRATCH_DIR) -I $(RUNTIME_DIR)


RDMA_CORE_FLAGS=-I$(RDMA_CORE_PATH)/include -L$(RDMA_CORE_PATH)/lib -Wl,--rpath,$(RDMA_CORE_PATH)/lib -llz4
$(BUILD_DIR)/runtime/nb_mlx5_transport.o: $(RUNTIME_DIR)/nb_mlx5_transport.cc $(RUNTIME_INCLUDES)
	$(CXX) $(RCFLAGS) -c $(RUNTIME_DIR)/nb_mlx5_transport.cc -o $(BUILD_DIR)/runtime/nb_mlx5_transport.o -I $(RUNTIME_DIR)/mlx5_impl/ -I $(SCRATCH_DIR) -I $(RUNTIME_DIR) $(RDMA_CORE_FLAGS)

.PRECIOUS: $(BUILD_DIR)/runtime/mlx5_impl/%.o
$(BUILD_DIR)/runtime/mlx5_impl/%.o: $(RUNTIME_DIR)/mlx5_impl/%.cc $(wildcard $(RUNTIME_DIR)/mlx5_impl/*)
	$(CXX) $(RCFLAGS) $< -o $@ -c -I $(RUNTIME_DIR)/mlx5_impl/ -I $(SCRATCH_DIR) $(RDMA_CORE_FLAGS)

mlx5_runtime: $(BUILD_DIR)/runtime/nb_mlx5_transport.o $(BUILD_DIR)/runtime/mlx5_impl/transport.o $(BUILD_DIR)/runtime/mlx5_impl/halloc.o
	

SIMPLE_RUNTIME_OBJS=$(BUILD_DIR)/runtime/nb_runtime_simple.o $(BUILD_DIR)/runtime/nb_simple.o $(BUILD_DIR)/runtime/nb_timer.o $(BUILD_DIR)/runtime/nb_compression.o

	
.PHONY: simple_network_test
simple_network_test: mlx5_runtime $(SIMPLE_RUNTIME_OBJS)
	$(CC) $(RCFLAGS) -c $(TEST_DIR)/test_mlx5_simple/server.c -o $(BUILD_DIR)/test/simple_network_test/server.o -I $(RUNTIME_DIR) -I $(SCRATCH_DIR) $(RDMA_CORE_FLAGS)
	$(CC) $(RCFLAGS) -c $(TEST_DIR)/test_mlx5_simple/client.c -o $(BUILD_DIR)/test/simple_network_test/client.o -I $(RUNTIME_DIR) -I $(SCRATCH_DIR) $(RDMA_CORE_FLAGS)
	$(CXX) $(BUILD_DIR)/runtime/nb_mlx5_transport.o $(BUILD_DIR)/runtime/mlx5_impl/halloc.o $(BUILD_DIR)/runtime/mlx5_impl/transport.o $(SIMPLE_RUNTIME_OBJS) $(BUILD_DIR)/test/simple_network_test/server.o -o $(BUILD_DIR)/test/network_simple_server $(RDMA_CORE_FLAGS) -libverbs -lmlx5
	$(CXX) $(BUILD_DIR)/runtime/nb_mlx5_transport.o $(BUILD_DIR)/runtime/mlx5_impl/halloc.o $(BUILD_DIR)/runtime/mlx5_impl/transport.o $(SIMPLE_RUNTIME_OBJS) $(BUILD_DIR)/test/simple_network_test/client.o -o $(BUILD_DIR)/test/network_simple_client $(RDMA_CORE_FLAGS) -libverbs -lmlx5



mlx5_impl_test: mlx5_runtime
	$(CXX) test/test_mlx5_impl/server.cpp $(BUILD_DIR)/runtime/mlx5_impl/halloc.o $(BUILD_DIR)/runtime/mlx5_impl/transport.o -o $(BUILD_DIR)/test/mlx5_impl_server -I $(RUNTIME_DIR)/mlx5_impl $(RDMA_CORE_FLAGS) -libverbs -lmlx5
	$(CXX) test/test_mlx5_impl/client.cpp $(BUILD_DIR)/runtime/mlx5_impl/halloc.o $(BUILD_DIR)/runtime/mlx5_impl/transport.o -o $(BUILD_DIR)/test/mlx5_impl_client -I $(RUNTIME_DIR)/mlx5_impl $(RDMA_CORE_FLAGS) -libverbs -lmlx5


	
.PHONY: simple_test
simple_test: executables $(SIMPLE_RUNTIME_OBJS)
	$(CC) $(RCFLAGS) -c $(TEST_DIR)/test_simple/server.c -o $(BUILD_DIR)/test/simple_test/server.o -I $(RUNTIME_DIR) -I $(SCRATCH_DIR)
	$(CC) $(RCFLAGS) -c $(TEST_DIR)/test_simple/client.c -o $(BUILD_DIR)/test/simple_test/client.o -I $(RUNTIME_DIR) -I $(SCRATCH_DIR)
	$(CC) $(RCFLAGS) -c $(RUNTIME_DIR)/nb_ipc_transport.c -o $(BUILD_DIR)/runtime/nb_ipc_transport.o -I $(RUNTIME_DIR) -I $(SCRATCH_DIR)
	$(CC) $(SIMPLE_RUNTIME_OBJS) $(BUILD_DIR)/test/simple_test/server.o $(BUILD_DIR)/runtime/nb_ipc_transport.o -o $(BUILD_DIR)/test/simple_server -llz4
	$(CC) $(SIMPLE_RUNTIME_OBJS) $(BUILD_DIR)/test/simple_test/client.o $(BUILD_DIR)/runtime/nb_ipc_transport.o -o $(BUILD_DIR)/test/simple_client -llz4

.PHONY: simple_test_fragmentation
simple_test_fragmentation: executables $(SIMPLE_RUNTIME_OBJS)
	$(CC) $(RCFLAGS) -c $(TEST_DIR)/test_simple_fragmentation/server.c -o $(BUILD_DIR)/test/simple_test_fragmentation/server.o -I $(RUNTIME_DIR) -I $(SCRATCH_DIR)
	$(CC) $(RCFLAGS) -c $(TEST_DIR)/test_simple_fragmentation/client.c -o $(BUILD_DIR)/test/simple_test_fragmentation/client.o -I $(RUNTIME_DIR) -I $(SCRATCH_DIR)
	$(CC) $(RCFLAGS) -c $(RUNTIME_DIR)/nb_ipc_transport.c -o $(BUILD_DIR)/runtime/nb_ipc_transport.o -I $(RUNTIME_DIR) -I $(SCRATCH_DIR)
	$(CC) $(SIMPLE_RUNTIME_OBJS) $(BUILD_DIR)/test/simple_test_fragmentation/server.o $(BUILD_DIR)/runtime/nb_ipc_transport.o -o $(BUILD_DIR)/test/simple_fragmentation_server -llz4
	$(CC) $(SIMPLE_RUNTIME_OBJS) $(BUILD_DIR)/test/simple_test_fragmentation/client.o $(BUILD_DIR)/runtime/nb_ipc_transport.o -o $(BUILD_DIR)/test/simple_fragmentation_client -llz4

.PHONY: simple_test_accept
simple_test_accept: executables $(SIMPLE_RUNTIME_OBJS)
	$(CC) $(RCFLAGS) -c $(TEST_DIR)/test_simple_accept/server.c -o $(BUILD_DIR)/test/simple_test_accept/server.o -I $(RUNTIME_DIR) -I $(SCRATCH_DIR)
	$(CC) $(RCFLAGS) -c $(TEST_DIR)/test_simple_accept/client.c -o $(BUILD_DIR)/test/simple_test_accept/client.o -I $(RUNTIME_DIR) -I $(SCRATCH_DIR)
	$(CC) $(RCFLAGS) -c $(RUNTIME_DIR)/nb_ipc_transport.c -o $(BUILD_DIR)/runtime/nb_ipc_transport.o -I $(RUNTIME_DIR) -I $(SCRATCH_DIR)
	$(CC) $(SIMPLE_RUNTIME_OBJS) $(BUILD_DIR)/test/simple_test_accept/server.o $(BUILD_DIR)/runtime/nb_ipc_transport.o -o $(BUILD_DIR)/test/simple_accept_server
	$(CC) $(SIMPLE_RUNTIME_OBJS) $(BUILD_DIR)/test/simple_test_accept/client.o $(BUILD_DIR)/runtime/nb_ipc_transport.o -o $(BUILD_DIR)/test/simple_accept_client

.PHONY: simple_test_inorder
simple_test_inorder: executables $(SIMPLE_RUNTIME_OBJS)
	$(CC) $(RCFLAGS) -c $(TEST_DIR)/test_simple_inorder/server.c -o $(BUILD_DIR)/test/simple_test_inorder/server.o -I $(RUNTIME_DIR) -I $(SCRATCH_DIR)
	$(CC) $(RCFLAGS) -c $(TEST_DIR)/test_simple_inorder/client.c -o $(BUILD_DIR)/test/simple_test_inorder/client.o -I $(RUNTIME_DIR) -I $(SCRATCH_DIR)
	$(CC) $(RCFLAGS) -c $(RUNTIME_DIR)/nb_ipc_transport.c -o $(BUILD_DIR)/runtime/nb_ipc_transport.o -I $(RUNTIME_DIR) -I $(SCRATCH_DIR)
	$(CC) $(SIMPLE_RUNTIME_OBJS) $(BUILD_DIR)/test/simple_test_inorder/server.o $(BUILD_DIR)/runtime/nb_ipc_transport.o -o $(BUILD_DIR)/test/simple_inorder_server
	$(CC) $(SIMPLE_RUNTIME_OBJS) $(BUILD_DIR)/test/simple_test_inorder/client.o $(BUILD_DIR)/runtime/nb_ipc_transport.o -o $(BUILD_DIR)/test/simple_inorder_client

.PHONY: simple_test_reliable
simple_test_reliable: executables $(SIMPLE_RUNTIME_OBJS)
	$(CC) $(RCFLAGS) -c $(TEST_DIR)/test_simple_reliable/server.c -o $(BUILD_DIR)/test/simple_test_reliable/server.o -I $(RUNTIME_DIR) -I $(SCRATCH_DIR)
	$(CC) $(RCFLAGS) -c $(TEST_DIR)/test_simple_reliable/client.c -o $(BUILD_DIR)/test/simple_test_reliable/client.o -I $(RUNTIME_DIR) -I $(SCRATCH_DIR)
	$(CC) $(RCFLAGS) -c $(RUNTIME_DIR)/nb_ipc_transport.c -o $(BUILD_DIR)/runtime/nb_ipc_transport.o -I $(RUNTIME_DIR) -I $(SCRATCH_DIR)
	$(CC) $(SIMPLE_RUNTIME_OBJS) $(BUILD_DIR)/test/simple_test_reliable/server.o $(BUILD_DIR)/runtime/nb_ipc_transport.o -o $(BUILD_DIR)/test/simple_reliable_server
	$(CC) $(SIMPLE_RUNTIME_OBJS) $(BUILD_DIR)/test/simple_test_reliable/client.o $(BUILD_DIR)/runtime/nb_ipc_transport.o -o $(BUILD_DIR)/test/simple_reliable_client


.PHONY: posix_interface
posix_interface: $(SIMPLE_RUNTIME_OBJS)
	$(CC) $(RCFLAGS) -c $(RUNTIME_DIR)/posix/src/network.c -o $(BUILD_DIR)/runtime/posix/network.o -I $(RUNTIME_DIR)/posix/include -I $(SCRATCH_DIR) -I $(RUNTIME_DIR)


.PHONY: simple_test_posix
simple_test_posix: posix_interface $(SIMPLE_RUNTIME_OBJS)
	$(CC) $(RCFLAGS) -c $(TEST_DIR)/test_simple_posix/server.c -o $(BUILD_DIR)/test/simple_test_posix/server.o -I $(RUNTIME_DIR) -I $(SCRATCH_DIR) -I $(RUNTIME_DIR)/posix/include
	$(CC) $(RCFLAGS) -c $(TEST_DIR)/test_simple_posix/client.c -o $(BUILD_DIR)/test/simple_test_posix/client.o -I $(RUNTIME_DIR) -I $(SCRATCH_DIR) -I $(RUNTIME_DIR)/posix/include
	$(CC) $(RCFLAGS) -c $(RUNTIME_DIR)/nb_ipc_transport.c -o $(BUILD_DIR)/runtime/nb_ipc_transport.o -I $(RUNTIME_DIR) -I $(SCRATCH_DIR)
	$(CC) $(SIMPLE_RUNTIME_OBJS) $(BUILD_DIR)/test/simple_test_posix/server.o $(BUILD_DIR)/runtime/nb_ipc_transport.o -o $(BUILD_DIR)/test/simple_posix_server $(BUILD_DIR)/runtime/posix/network.o -llz4
	$(CC) $(SIMPLE_RUNTIME_OBJS) $(BUILD_DIR)/test/simple_test_posix/client.o $(BUILD_DIR)/runtime/nb_ipc_transport.o -o $(BUILD_DIR)/test/simple_posix_client $(BUILD_DIR)/runtime/posix/network.o -llz4
	
.PHONY: simple_test_linux
simple_test_linux: executables $(SIMPLE_RUNTIME_OBJS)
	$(CC) $(RCFLAGS) -c $(TEST_DIR)/test_simple_linux/server.c -o $(BUILD_DIR)/test/simple_test_linux/server.o -I$(RUNTIME_DIR) -I$(SCRATCH_DIR)
	$(CC) $(RCFLAGS) -c $(TEST_DIR)/test_simple_linux/client.c -o $(BUILD_DIR)/test/simple_test_linux/client.o -I$(RUNTIME_DIR) -I$(SCRATCH_DIR)
	$(CC) $(RCFLAGS) -c $(RUNTIME_DIR)/nb_linux_transport.c -o $(BUILD_DIR)/runtime/nb_linux_transport.o -I $(RUNTIME_DIR) -I $(SCRATCH_DIR)
	$(CC) $(SIMPLE_RUNTIME_OBJS) $(BUILD_DIR)/test/simple_test_linux/server.o $(BUILD_DIR)/runtime/nb_linux_transport.o -o $(BUILD_DIR)/test/simple_linux_server 
	$(CC) $(SIMPLE_RUNTIME_OBJS) $(BUILD_DIR)/test/simple_test_linux/client.o $(BUILD_DIR)/runtime/nb_linux_transport.o -o $(BUILD_DIR)/test/simple_linux_client 

.PHONY: simple_test_posix_network
simple_test_posix_network: mlx5_runtime posix_interface $(SIMPLE_RUNTIME_OBJS)
	$(CC) $(RCFLAGS) -c $(TEST_DIR)/test_simple_posix/server.c -o $(BUILD_DIR)/test/simple_test_posix/server.o -I $(RUNTIME_DIR) -I $(SCRATCH_DIR) -I $(RUNTIME_DIR)/posix/include
	$(CC) $(RCFLAGS) -c $(TEST_DIR)/test_simple_posix/client.c -o $(BUILD_DIR)/test/simple_test_posix/client.o -I $(RUNTIME_DIR) -I $(SCRATCH_DIR) -I $(RUNTIME_DIR)/posix/include
	$(CXX) $(SIMPLE_RUNTIME_OBJS) $(BUILD_DIR)/runtime/mlx5_impl/halloc.o $(BUILD_DIR)/runtime/mlx5_impl/transport.o $(BUILD_DIR)/test/simple_test_posix/server.o -o $(BUILD_DIR)/test/simple_posix_network_server $(BUILD_DIR)/runtime/posix/network.o $(BUILD_DIR)/runtime/nb_mlx5_transport.o $(RDMA_CORE_FLAGS) -libverbs -lmlx5
	$(CXX) $(SIMPLE_RUNTIME_OBJS) $(BUILD_DIR)/runtime/mlx5_impl/halloc.o $(BUILD_DIR)/runtime/mlx5_impl/transport.o $(BUILD_DIR)/test/simple_test_posix/client.o -o $(BUILD_DIR)/test/simple_posix_network_client $(BUILD_DIR)/runtime/posix/network.o $(BUILD_DIR)/runtime/nb_mlx5_transport.o $(RDMA_CORE_FLAGS) -libverbs -lmlx5


simple_test_run: simple_test
	$(BUILD_DIR)/test/simple_server &
	sleep 1
	$(BUILD_DIR)/test/simple_client
	pkill -9 simple_server || true

.PHONY: utils
utils:
	$(CC) -O3 utils/hub/ipc_hub.c -o $(BUILD_DIR)/utils/ipc_hub	

clean:
	rm -rf $(BUILD_DIR)
