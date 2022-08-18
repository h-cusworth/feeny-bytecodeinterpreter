MAIN_OUT := cfeeny
TEST_OUT := my_compiler_tests
CLEAN := $(MAIN_OUT) $(TEST_OUT)
CHECK_FLAGS := $(shell pkg-config --cflags check) $(shell pkg-config --libs check) 

compiler:
	gcc src/*.c -Wno-pointer-to-int-cast -DCOMPILE_MAIN -o ${MAIN_OUT}

my-tests:
	gcc my_tests/*.c src/*.c ${CHECK_FLAGS} -o ${TEST_OUT}

clean:
	rm -f ${CLEAN}
