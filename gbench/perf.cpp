/*

Copyright (c) 2017, Schizofreny s.r.o - info@schizofreny.com
All rights reserved.

See LICENSE.md file

*/

#include "benchmark/benchmark.h"
#include <cstring>
#include <vector>
#include "../scalar.hpp"
#ifdef USE_AVX512
#include "../avx512.hpp"
#endif
#include <unistd.h>

#include <fstream>
#include <random>
#include <iostream>
#include <typeinfo>

using namespace std;
using namespace middleout;

#ifdef USE_AVX512
#define ALG_CLASS Avx52
#else
#define ALG_CLASS Scalar
#endif

#define BENCHMARK_ARGS ->Arg(500000)->Arg(1000000)->Arg(200000000)

template <typename T>
auto getCompressor() {
	return &ALG_CLASS<T>::compress;
}

template <typename T>
auto getDecompressor() {
	return &ALG_CLASS<T>::decompress;
}

template <typename T>
std::vector<T>* generateSequeceRandRepeat(T from, T to, int repeatingProbabilityPercent) {
	auto data = new std::vector<T>(to - from);

	data->push_back(0);
	for (int64_t i = 1; i < (to - from); i++) {
		if (rand() % 100 < repeatingProbabilityPercent) {
			// make this value same as previous value
			(*data)[i] = data->at(i - 1);
		} else {
			if (typeid(T) == typeid(double)) {
				// double sequence growths by 0.1
				(*data)[i] = 0.1 * i;
			} else {
				// int64_t sequence growths by 1
				(*data)[i] = from + i;
			}
		}
	}

	return data;
}

vector<long>* generateSequece(long from, long to) {
	auto data = new vector<long>(to - from);

	for (long i = 0; i < (to - from); i++) {
		if (i % 2) {
			(*data)[i] = from + i;
		} else {
			(*data)[i] = from + i - 1;
		}
	}

	return data;
}

std::vector<double>* generateSequeceRandomRepeat(double from, double to) {
	auto data = new std::vector<double>(to - from);

	double val = 0.1;
	data->push_back(val);
	for (long i = 1; i < (to - from); i++) {
		if (rand() % 2) {
			(*data)[i] = val * i;
		} else {
			(*data)[i] = (*data)[i - 1];
		}
	}

	return data;
}

std::vector<long>* generateRandom() {
	long from = 0;
	long to = 100000000;

	size_t count = 3 * 1000 * 1000;

	auto data = new std::vector<long>(count);

	// better rand
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_real_distribution<double> dist(from, to);

	for (size_t i = 0; i < count; i++) {
		data->push_back((long)dist(mt));
	}

	return data;
}

template <typename T>
static void benchmarkCompress(benchmark::State& state, std::vector<T>& data) {
	// Scalar::maxCompressSize has same implementation as Avx52 (format is compatible so max length
	// is compatible too)
	std::vector<char> compressedData(Scalar<long>::maxCompressedSize(data.size()));

	int compressedSize = 0;
	while (state.KeepRunning()) {
		compressedSize = getCompressor<T>()(data, compressedData);
	}

	// std::cout << "ratio: " << (double)(data.size() * sizeof(long)) / compressedSize << std::endl;
	state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(data.size() * sizeof(long)));
}

template <typename T>
static void benchmarkDecompress(benchmark::State& state, std::vector<T>& data) {
	std::vector<char> compressedData(Scalar<long>::maxCompressedSize(data.size()));
	Scalar<T>::compress(data, compressedData);
	std::vector<T> outData(data.size());

	while (state.KeepRunning()) {
		getDecompressor<T>()(compressedData, data.size(), outData);
	}
	state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(data.size() * sizeof(long)));
}

static void BM_sequenceCompress(benchmark::State& state) {
	auto data = generateSequece(0, state.range(0));
	benchmarkCompress(state, *data);
	delete data;
}
BENCHMARK(BM_sequenceCompress) BENCHMARK_ARGS;

static void BM_sequenceDecompress(benchmark::State& state) {
	auto data = generateSequece(0, state.range(0));
	benchmarkDecompress(state, *data);
	delete data;
}
BENCHMARK(BM_sequenceDecompress) BENCHMARK_ARGS;

static void BM_RandRepeatCompress(benchmark::State& state) {
	auto data = generateSequeceRandomRepeat(0, state.range(0));
	benchmarkCompress(state, *data);
	delete data;
}
BENCHMARK(BM_RandRepeatCompress) BENCHMARK_ARGS;

static void BM_RandRepeatDecompress(benchmark::State& state) {
	auto data = generateSequeceRandomRepeat(0, state.range(0));
	benchmarkDecompress(state, *data);
	delete data;
}
BENCHMARK(BM_RandRepeatDecompress) BENCHMARK_ARGS;

static void BM_testRandomDistributionCompress(benchmark::State& state) {
	auto data = generateRandom();
	benchmarkCompress(state, *data);
	delete data;
}
BENCHMARK(BM_testRandomDistributionCompress) BENCHMARK_ARGS;

static void BM_testRandomDistributionDecompress(benchmark::State& state) {
	auto data = generateRandom();
	benchmarkDecompress(state, *data);
	delete data;
}
BENCHMARK(BM_testRandomDistributionDecompress) BENCHMARK_ARGS;

#define MAKE_PROB_REPEATING_COMPRESSION_TEST(PERCENT)                                  \
	static void BM_testRandomRepeatCompressP##PERCENT(benchmark::State& state) {       \
		auto data = generateSequeceRandRepeat<double>(0, 10000000.0, PERCENT);         \
		benchmarkCompress(state, *data);                                               \
		delete data;                                                                   \
	}                                                                                  \
	BENCHMARK(BM_testRandomRepeatCompressP##PERCENT) BENCHMARK_ARGS;                   \
                                                                                       \
	static void BM_testRandomRepeatDecompressP##PERCENT(benchmark::State& state) {     \
		auto data = generateSequeceRandRepeat<double>(0, 10000000.0, PERCENT);         \
		benchmarkDecompress(state, *data);                                             \
		delete data;                                                                   \
	}                                                                                  \
	BENCHMARK(BM_testRandomRepeatDecompressP##PERCENT) BENCHMARK_ARGS;                 \
                                                                                       \
	static void BM_testRandomRepeatLongCompressP##PERCENT(benchmark::State& state) {   \
		auto data = generateSequeceRandRepeat<long>(0, 10000000, PERCENT);             \
		benchmarkCompress(state, *data);                                               \
		delete data;                                                                   \
	}                                                                                  \
	BENCHMARK(BM_testRandomRepeatLongCompressP##PERCENT) BENCHMARK_ARGS;               \
                                                                                       \
	static void BM_testRandomRepeatLongDecompressP##PERCENT(benchmark::State& state) { \
		auto data = generateSequeceRandRepeat<long>(0, 10000000, PERCENT);             \
		benchmarkDecompress(state, *data);                                             \
		delete data;                                                                   \
	}                                                                                  \
	BENCHMARK(BM_testRandomRepeatLongDecompressP##PERCENT) BENCHMARK_ARGS;

MAKE_PROB_REPEATING_COMPRESSION_TEST(10);
MAKE_PROB_REPEATING_COMPRESSION_TEST(25);
MAKE_PROB_REPEATING_COMPRESSION_TEST(50);
MAKE_PROB_REPEATING_COMPRESSION_TEST(75);
MAKE_PROB_REPEATING_COMPRESSION_TEST(90);
MAKE_PROB_REPEATING_COMPRESSION_TEST(95);

static std::vector<double>* readFileData(const char* path, bool isDouble, int scale) {
	std::ifstream infile(path);

	auto data = new std::vector<double>();

	std::string line;
	while (std::getline(infile, line)) {
		double val;
		if (isDouble) {
			val = std::stod(line);
			if (scale) {
				long v1 = scale * val;
				val = reinterpret_cast<double&>(v1);
			}
		} else {
			long v = std::stol(line);
			val = reinterpret_cast<double&>(v);
		}
		data->push_back(val);
	}

	return data;
}

#define MAKE_COMPRESSION_TEST(NAME, isDouble, file, scale)              \
	static void BM_fileDataCompression##NAME(benchmark::State& state) { \
		auto data = readFileData(file, isDouble, scale);                \
		benchmarkCompress(state, *data);                                \
		delete data;                                                    \
	}

MAKE_COMPRESSION_TEST(A, true, "data/ibm.data", 0)
BENCHMARK(BM_fileDataCompressionA);

MAKE_COMPRESSION_TEST(B, true, "data/ibm.data", 10000)
BENCHMARK(BM_fileDataCompressionB);

MAKE_COMPRESSION_TEST(C, false, "data/writes.data", 0)
BENCHMARK(BM_fileDataCompressionC);

MAKE_COMPRESSION_TEST(D, false, "data/redis_memory.data", 0)
BENCHMARK(BM_fileDataCompressionD);

#define MAKE_DECOMPRESSION_TEST(NAME, isDouble, file, scale)              \
	static void BM_fileDataDecompression##NAME(benchmark::State& state) { \
		auto data = readFileData(file, isDouble, scale);                  \
		benchmarkDecompress(state, *data);                                \
		delete data;                                                      \
	}

MAKE_DECOMPRESSION_TEST(A, true, "data/ibm.data", 0)
BENCHMARK(BM_fileDataDecompressionA);

MAKE_DECOMPRESSION_TEST(B, true, "data/ibm.data", 10000)
BENCHMARK(BM_fileDataDecompressionB);

MAKE_DECOMPRESSION_TEST(C, false, "data/writes.data", 0)
BENCHMARK(BM_fileDataDecompressionC);

MAKE_DECOMPRESSION_TEST(D, false, "data/redis_memory.data", 0)
BENCHMARK(BM_fileDataDecompressionD);

BENCHMARK_MAIN();
