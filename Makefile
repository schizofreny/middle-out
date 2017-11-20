CC = g++

###
#	COMPILE AND RUN TESTS
###
GOOGLE_TEST_LIB = gtest
CC_TEST_FLAGS = -O2 -g -Wall -Wno-strict-aliasing -fsanitize=address -D_GLIBCXX_DEBUG_PEDANTIC
LD_TEST_FLAGS = -l $(GOOGLE_TEST_LIB) -l pthread -l gtest_main

TEST_OBJECTS = gtest/test.cpp scalar.cpp
TEST_TARGET = test

BUILD_DIR = dist

test:
	$(CC) -o $(TEST_TARGET) $(TEST_OBJECTS) $(CC_TEST_FLAGS)  $(LD_TEST_FLAGS) \
	&& ./$(TEST_TARGET)

test-avx512:
	$(CC) -o $(TEST_TARGET) $(TEST_OBJECTS) avx512.cpp $(CC_TEST_FLAGS) \
	-march=skylake-avx512 $(LD_TEST_FLAGS) -D USE_AVX512 && ./$(TEST_TARGET)

###
#	COMPILE STATIC LIBS
###
CC_LIB_FLAGS = -c -O3 -s

lib:
	mkdir -p $(BUILD_DIR)
	$(CC) $(CC_LIB_FLAGS) middleout.cpp scalar.cpp helpers.hpp -march=native
	ar -rcs libmiddleout.a middleout.o scalar.o helpers.hpp.gch
	mv libmiddleout.a $(BUILD_DIR)/
	cp middleout.hpp $(BUILD_DIR)/

lib-avx512:
	mkdir -p $(BUILD_DIR)
	$(CC) $(CC_LIB_FLAGS) middleout.cpp avx512.cpp helpers.hpp -march=skylake-avx512 -D USE_AVX512
	ar -rcs libmiddleout-avx512.a middleout.o avx512.o helpers.hpp.gch
	mv libmiddleout-$(BUILD_DIR).a $(BUILD_DIR)/
	cp middleout.hpp $(BUILD_DIR)/

install-libs-example:
	cp $(BUILD_DIR)/*.a example
	cp middleout.hpp example/

clean-lib:
	-rm libmiddleout.a
	-rm libmiddleout-avx512.a
	-rm $(BUILD_DIR)/*.a

###
#	COMPILE AND RUN GOOGLE BENCHMARK TESTS
###
CC_GBENCH_FLAGS = -O3
LD_GBENCH_FLAGS = -l gtest -l benchmark -l pthread

GBENCH_OBJECTS = gbench/perf.cpp scalar.cpp
GBENCH_TARGET = perf

bench:
	$(CC) -o $(GBENCH_TARGET) $(GBENCH_OBJECTS) $(CC_GBENCH_FLAGS) -march=native $(LD_GBENCH_FLAGS)
	./$(GBENCH_TARGET)

bench-avx512:
	$(CC) -o $(GBENCH_TARGET) $(GBENCH_OBJECTS) avx512.cpp $(CC_GBENCH_FLAGS) -march=skylake-avx512 $(LD_GBENCH_FLAGS) -D USE_AVX512
	./$(GBENCH_TARGET)

#aliases for bench
perf:
	make bench

perf-avx512:
	make bench-avx512

clean:
	-rm *.o
	-rm *.a
	-rm *.gch
	-rm gtest/*.o
	-rm gbench/*.o
	-rm $(BUILD_DIR) -r
	-rm $(TEST_TARGET)
	-rm $(GBENCH_TARGET)

.PHONY: clean test test-avx512 lib lib-avx512 clean-lib bench bench-avx512 perf perf-avx512