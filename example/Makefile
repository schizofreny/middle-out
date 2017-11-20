CPP = g++
CPP_ARGS = -O3 -march=native -o demo
TARGET = example.cpp

HAVE_AVX512 = $(shell grep avx512 /proc/cpuinfo)


all:
	$(CPP) $(CPP_ARGS) $(TARGET) libmiddleout.a 
	./demo

avx512:
ifeq ($(HAVE_AVX512),)
	@echo Your CPU does not support AVX-512 instruction set
else
	$(CPP) $(CPP_ARGS) $(TARGET) libmiddleout-avx512.a 
	./demo
endif

PHONY: all avx512