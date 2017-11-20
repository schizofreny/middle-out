/*

Copyright (c) 2017, Schizofreny s.r.o - info@schizofreny.com
All rights reserved.

See LICENSE.md file

*/

#include <stdlib.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <chrono>
#include <string>
#include <typeinfo>

#include "middleout.hpp"  // compression api

int BYTES_TO_GB = 1024 * 1024 * 1024;
// parameter for data generation
int PROBABILITY_OF_REPEATING = 80;  // percent

// alias time_point
using timep_t = std::chrono::time_point<std::chrono::system_clock>;

/*
    FUNCTIONS DECLARATIONS
*/
template <typename T>
void compressionWrapper(std::vector<T>& dataToCompress, std::vector<char>& compressedData);

template <typename T>
void decompressionWrapper(std::vector<char>& compressedData,
                          size_t itemsCount,
                          std::vector<T>& data);

template <typename T>
std::vector<T>* generateSequece(T from, T to);

template <typename T>
void testSequence(T from, T to);

void testStockData();
void testMyData();

/*
    FUNCTIONS DEFINITIONS
*/

/**
    Prints stats about compression
    Shows example of compression call

    @param dataToCompress can be vector of doubles or int64_t
    @param compressedData - compressed data will be writeen here
*/
template <typename T>
void compressionWrapper(std::vector<T>& dataToCompress, std::vector<char>& compressedData) {
	timep_t start, end;
	start = std::chrono::system_clock::now();

	// output is valid between 0 and compressedSize
	size_t compressedSize = middleout::compress(dataToCompress, compressedData);
	end = std::chrono::system_clock::now();

	std::chrono::duration<double> elapsedSeconds = end - start;

	size_t originalSize = dataToCompress.size() * sizeof(T);
	double throughput = originalSize / elapsedSeconds.count() / BYTES_TO_GB;

	// stats
	std::cout << "COMPRESSION:" << std::endl;
	std::cout << "elapsed time: " << elapsedSeconds.count() << "s" << std::endl;
	std::cout << "uncompressed size: " << originalSize << std::endl;
	std::cout << "compressed size: " << compressedSize << std::endl;
	std::cout << "compressed ratio: " << (double)originalSize / compressedSize << std::endl;
	std::cout << "throughput: " << throughput << " GB/s" << std::endl;
	std::cout << std::endl;
}

/**
    Prints stats about decompression
    Shows example of decompression call

    @param compressedData - container with compressed data
    @param itemsCount - number of elements to be decompressed
    @param data - write output to this vector
*/
template <typename T>
void decompressionWrapper(std::vector<char>& compressedData,
                          size_t itemsCount,
                          std::vector<T>& data) {
	timep_t start, end;
	start = std::chrono::system_clock::now();

	// decompress call
	middleout::decompress(compressedData, itemsCount, data);
	end = std::chrono::system_clock::now();

	std::chrono::duration<double> elapsedSeconds = end - start;
	double throughput = (itemsCount * sizeof(T)) / elapsedSeconds.count() / BYTES_TO_GB;

	// print stats
	std::cout << "DECOMPRESSION:" << std::endl;
	std::cout << "elapsed time: " << elapsedSeconds.count() << "s" << std::endl;
	std::cout << "throughput: " << throughput << " GB/s" << std::endl;
	std::cout << std::endl;
}

/**
    Generate mock data.
    Value will be same as previous with probability of PROBABILITY_OF_REPEATING

    For int data type it generates sequence like 0, 1, 2, 3, ...
    For double it generates 0, 0.1, 0.2, 0.3, ...


    @param from
    @param to
    @retun pointer to generated data. Caller must call destructor
*/
template <typename T>
std::vector<T>* generateSequence(T from, T to) {
	auto data = new std::vector<T>(to - from);

	data->push_back(0);
	for (int64_t i = 1; i < (to - from); i++) {
		if (rand() % 100 < PROBABILITY_OF_REPEATING) {
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

/**
    Generates sequence, compress and decompress data, check for correctness

    Type of sequence is determined by params type. See `generateSequence` function for more info

    @param from - first value of generated sequence
    @param to - max value of generated sequence
*/
template <typename T>
void testSequence(T from, T to) {
	std::vector<T>* data = generateSequence(from, to);

	// allocate output vector
	std::vector<char> compressedData(middleout::maxCompressedSize(data->size()));
	compressionWrapper(*data, compressedData);

	std::vector<T> uncompressedData(data->size());
	decompressionWrapper(compressedData, data->size(), uncompressedData);

	for (size_t i = 0; i < data->size(); i++) {
		// verify uncompressed data
		if (data->at(i) != uncompressedData[i]) {
			throw std::string("data not match");
		}
	}

	delete data;
}

/*
    Loads stock data from file, compress and decompres
*/
void testStockData() {
	std::ifstream infile("../data/ibm.data");

	std::vector<int64_t> data;

	std::string line;
	while (std::getline(infile, line)) {
		// scale to integer - with no decimal fraction
		int64_t price10000x = 10000 * std::stod(line);
		data.push_back(price10000x);
	}

	std::cout << std::endl << "13 MB of stock market data - IBM" << std::endl;
	std::vector<char> compressedData(middleout::maxCompressedSize(data.size()));

	std::vector<double> outData(data.size());
	compressionWrapper(data, compressedData);
	decompressionWrapper(compressedData, data.size(), outData);
}

/*
    You can use this function to test the algorithm on Your data
*/
/*
void testMyData() {
    //data is stored in text format in my.data file
    //each value on separate line
    std::ifstream infile("my.data");

    std::vector<int64_t> data;

    std::string line;
    while (std::getline(infile, line)) {
        data.push_back(std::strtol(line)); //for long values
        // data.push_back(std::stod(line)); //for float values
    }

    std::cout << std::endl << "My data" << std::endl;
    std::vector<char> compressedData(middleout::maxCompressedSize(data.size()));

    std::vector<double> outData(data.size());
    compressionWrapper(data, compressedData);
    decompressionWrapper(compressedData, data.size(), outData);
}
*/

int main(void) {
	auto msg = "Middle-out compression example";
	std::cout << msg << std::endl << std::endl;

	std::cout << "4 MB of sequence (0.1 * position). Randomly repeating." << std::endl;
	testSequence<double>(0, 500000.0);
	std::cout << "4 MB of sequence ascending values. Randomly repeating." << std::endl;
	testSequence<int64_t>(0, 500000);

	testStockData();

	// uncomment this to test algorithm on Your data
	// testMyData();

	return 0;
}
