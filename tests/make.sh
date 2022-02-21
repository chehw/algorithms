#!/bin/bash

[ -z "${NO_DEBUG}" ] && DEBUG="-D_DEBUG"

target=${1-"dijkstra-shortest-path"}

target=$(basename ${target})
target=${target/.[ch]/}


LINKER="gcc -std=gnu99 -g ${DEBUG} -Wall -Iinclude -Iutils -Itests -D_DEFAULT_SOURCE -D_GNU_SOURCE " 


case "${target}" in 
	dijkstra-shortest-path)
		echo "build $target ..."
		${LINKER} -DTEST_DIJKSTRA_SHORTEST_PATH -DALGORITHMS_C_STAND_ALONE \
			-o tests/${target} \
			src/dijkstra-shortest-path.c src/base/*.c \
			-lm -lpthread
		;;
	common|clib-stack|clib-slist|clib-*)
		${LINKER} -DTEST_ALGORITHMS_C_COMMON -DALGORITHMS_C_STAND_ALONE \
			-o tests/test_common src/common.c src/base/*.c
		;;
	*)
		exit 1
		;;
esac

