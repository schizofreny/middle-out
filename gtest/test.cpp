/*

Copyright (c) 2017, Schizofreny s.r.o - info@schizofreny.com
All rights reserved.

See LICENSE.md file

*/

#include <gtest/gtest.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <random>
#include <iostream>
#include <array>
#include <fstream>

#include "../scalar.hpp"
#ifdef USE_AVX512
#include "../avx512.hpp"
#endif

using namespace std;

namespace middleout {

template <typename T, typename U, typename V>
void checkFunctions(vector<V>& dataIn, T& compress, U& decompress) {
	size_t count = dataIn.size();

	vector<char> compressed(Scalar<V>::maxCompressedSize(count));
	size_t compressLength = compress(dataIn, compressed);

	ASSERT_NE(compressLength, 0) << "Not compressed";

	// hard copy, guaranteed vector boundary
	vector<char> compressedExactLength(compressed.begin(), compressed.begin() + compressLength + 1);

	vector<V> dataOut(count);
	decompress(compressedExactLength, count, dataOut);

	for (size_t i = 0; i < count; i++) {
		ASSERT_EQ(dataIn[i], dataOut[i]) << "data do not match. Index: " << i;
	}

	// if (dataIn.size() > 2000) {
	// 	cout << "Orig. length: " << 8 * dataIn.size() << " Compressed: " << compressLength
	// 	     << " Ratio: " << 8.0 * dataIn.size() / compressLength << endl;
	// }
}

template <typename T>
void compressDecompressCheck(vector<T>& dataIn) {
	checkFunctions(dataIn, Scalar<T>::compress, Scalar<T>::decompress);

#ifdef USE_AVX512
	checkFunctions(dataIn, Avx52<T>::compress, Avx52<T>::decompress);

	// test implementation compatibility
	checkFunctions(dataIn, Scalar<T>::compress, Avx52<T>::decompress);
#endif
}

vector<long>* generateSequece(long from, long to) {
	auto data = new vector<long>(to - from);

	for (long i = 0; i < (to - from); i++) {
		(*data)[i] = from + i;
	}

	return data;
}

void testSequence(long from, long to) {
	auto data = generateSequece(from, to);
	compressDecompressCheck(*data);
	delete data;
}

std::vector<double>* generateSequeceDecimal(double from, double to, double step) {
	auto data = new std::vector<double>(to - from);

	(*data)[0] = 0.213;

	double val = 0.1;
	for (long i = 1; i < (to - from); i++) {
		if (i % 3) {
			(*data)[i] = (*data)[i - 1];
		} else {
			(*data)[i] = val * i;
		}
	}

	return data;
}

void testSequenceDecimal(double from, double to) {
	double steps[] = {0.1, 0.01, 0.001, 0.03, 0.000001, 0.000037, 0.000000000001};

	for (double step : steps) {
		auto data = generateSequeceDecimal(from, to, step);
		compressDecompressCheck(*data);
		delete data;
	}
}

void testRDRAndomDistribution(double from, double to) {
	size_t count = 3 * 1000 * 1000;

	vector<long> data(count);

	// better rand
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_real_distribution<double> dist(from, to);

	for (size_t i = 0; i < count; i++) {
		data[i] = static_cast<long>(dist(mt));
	}

	compressDecompressCheck(data);
}

TEST(CompressionTest, exampleCompressRatioTest) {
	std::ifstream infile("data/ibm.data");

	std::vector<double> data;

	std::string line;
	while (std::getline(infile, line)) {
		long price100x = 10000 * std::stod(line);
		double f = reinterpret_cast<double&>(price100x);
		data.push_back(f);
	}

	std::cout << "orig size: " << data.size() * 8 << std::endl;
	int64_t origSize = data.size() * sizeof(double);
	int64_t newSize = Scalar<double>::compressSimple(data)->size();
	std::cout << "new size:  " << newSize << std::endl;
	std::cout << "ratio:     " << (double)origSize / newSize << std::endl;
}

TEST(CompressionTest, basicTest) {
	vector<long> data;

	data = {1, 3, 4, 2, 1, 3, 7, 5, 16, 8, 8, 3, 7, 5, 16, 8, 8, 3};
	compressDecompressCheck(data);

	data = {1};
	compressDecompressCheck(data);

	data = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
	compressDecompressCheck(data);

	data = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
	compressDecompressCheck(data);

	data = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
	compressDecompressCheck(data);

	data = {1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1};
	compressDecompressCheck(data);

	data.assign(1000, 0xFF);
	compressDecompressCheck(data);

	data.assign(1000, -1);
	compressDecompressCheck(data);

	data.assign(1000, -129);
	compressDecompressCheck(data);

	data = {37, 15, 34, 47, 10, 83, 10, 87, 20, 1,  54, 80, 24, 82, 82, 0,  30, 11, 9,  7,
	        62, 25, 12, 71, 92, 7,  21, 57, 62, 52, 48, 39, 57, 62, 7,  97, 16, 92, 4,  77,
	        4,  57, 67, 59, 60, 15, 22, 87, 85, 72, 73, 15, 21, 10, 66, 45, 14, 42, 44, 58,
	        57, 74, 14, 16, 34, 90, 76, 8,  52, 34, 89, 47, 72, 94, 19, 62, 60, 19, 69, 21,
	        55, 75, 37, 70, 98, 88, 78, 96, 23, 60, 61, 49, 70, 71, 16, 51, 83, 49, 91, 13};
	compressDecompressCheck(data);

	data = {1583615825,
	        1104566865,
	        1 << 10,
	        151512957,
	        151512957151512957,
	        9223372036854775807,
	        2,
	        2,
	        2,
	        2,
	        2,
	        2,
	        2,
	        2,
	        2,
	        2,
	        2,
	        2,
	        2,
	        2,
	        2,
	        2,
	        2,
	        2,
	        2,
	        2,
	        2,
	        2,
	        2,
	        2,
	        2,
	        2,
	        2,
	        1};
	compressDecompressCheck(data);

	data = {581852851,  542359410,  1765458614, 184474295,  1462380312, 1509678522, 2091142647,
	        1491358325, 268670595,  1988704588, 104651035,  1879839687, 1294088155, 2070259996,
	        1194255693, 1142067077, 1033565988, 288841850,  656431651,  1523767369, 1407061474,
	        622864439,  1086922039, 2129958271, 1276318908, 979442622,  1321791732, 2813034,
	        1266963838, 1788621486, 1179015077, 1848816690, 183497248,  796990043,  2033290985,
	        1645877561, 159184917,  1976949985, 989752238,  427855512,  1818170925, 1094403273,
	        160211551,  964775432,  1017179621, 1354467245, 2106842510, 2050745610, 1643309095,
	        615790513,  1427029331, 902886921,  1238654952, 366467722,  885361544,  367490212,
	        1345910345, 59669629,   370303247,  465390535,  1848291115, 1549318324, 166723577,
	        2031788364, 198824719,  52530915,   1530182277, 358009636,  2029480900, 372450867,
	        785865148,  1700168177, 1466854141, 946076699,  517459962,  336550114,  153060296,
	        476818824,  239812076,  1796369391, 1092609337, 1666841407, 551772664,  183780641,
	        2033309130, 1437134209, 551270854,  1231735827, 1496803838, 921574101,  1697126362,
	        1197611305, 323408777,  1863849940, 1081916021, 522233496,  1916380855, 464614650,
	        880243132,  1798378107};
	compressDecompressCheck(data);
	data = {227165578,  386567434,  354437181,  1897514693, 972009470,  663111646,  1377740108,
	        189132208,  385295700,  814750726,  1883440200, 350197716,  1098359387, 1553174924,
	        474677958,  1483963562, 1845836834, 612078950,  1098907035, 973762126,  281222771,
	        495565493,  599600670,  449282427,  68654770,   556197552,  138769256,  1309961135,
	        289887917,  522165933,  569180592,  517053496,  908733367,  923617773,  267084541,
	        1880742837, 1586729420, 1644824650, 2069875045, 1972025120, 312091728,  1805831598,
	        174739188,  1410451115, 1211522874, 649417147,  746931030,  909876060,  1261496097,
	        1845838065, 1883638187, 1542718868, 193919911,  335755209,  1992001296, 262574681,
	        891952761,  2130770552, 1572535816, 1181840678};

	compressDecompressCheck(data);
}

TEST(CompressionTest, testSequences) {
	testSequence(0, 100);
	testSequence(0, 123);
	testSequence(0, 47);
	testSequence(0, 8000);
	testSequence(20, 50);
	testSequence(20, 23);
	testSequence(20, 29);
	testSequence(20, 30);
	testSequence(0, 1000 * 1000);
	testSequence(2147483647, 2147483647L + 1000);
	testSequence(9223372036854775807 - 1000, 9223372036854775807);
	testSequence(-9223372036854775800, -9223372036854774000);
	testSequence(0, 3 * 1000 * 1000);
}

TEST(CompressionTest, testDifferentInputDataLength) {
	for (size_t i = 1; i < 2000; i++) {
		testSequence(0, i);
	}
}

TEST(CompressionTest, testRandom) {
	size_t count = 3 * 1000 * 1000;

	vector<long> data(count);

	srand(time(NULL));
	for (size_t i = 0; i < count; i++) {
		data[i] = (long)rand();
	}
	compressDecompressCheck(data);
}

TEST(CompressionTest, testRDRandom) {
	testRDRAndomDistribution(0, 100);
	testRDRAndomDistribution(0, 10000);
	testRDRAndomDistribution(0, 10000000000);
	testRDRAndomDistribution(0, 9223372036854775807);
	testRDRAndomDistribution(1, 3);
}

TEST(CompressionTest, testVarLength) {
	for (size_t i = 1; i < 1000; i++) {
		testSequence(0, i);
	}
}

TEST(CompressionTest, testSteps) {
	std::array<long, 5> steps = {2, 254, 255 * 255, 255 * 255 * 255, 255L * 255 * 255 * 255};

	for (const auto& step : steps) {
		vector<long> data(10000);
		data[0] = 0;
		for (size_t i = 1; i < 10000; i++) {
			data[i] = data[i - 1] + step;
		}
		compressDecompressCheck(data);
	}
}
TEST(CompressionTest, testAlteringSeq) {
	vector<long> data(1 * 1000 * 1000);

	size_t p = 0;
	while (1) {
		// 20X repeat sequence
		for (size_t j = 200; j < 400; j++) {
			// 10 seqences
			for (size_t k = 0; k < 10; k++) {
				// length of seq run
				for (size_t i = 0; i < j; i++) {
					data[p++] = k;

					if (p == data.size()) {
						compressDecompressCheck(data);
						return;
					}
				}
			}
		}
	}
}

TEST(CompressionTest, test50PercentMonotonic) {
	size_t size = 1 * 1000 * 1000;
	vector<long> data(size);
	data[0] = 1256;
	for (size_t i = 1; i < size; i++) {
		for (size_t j = 0; i < size && j < 10; i++, j++) {
			data[i] = data[i - 1];
		}

		for (size_t j = 0; i < size && j < 10; i++, j++) {
			data[i] = i;
		}
	}

	compressDecompressCheck(data);
}

TEST(CompressionTest, testVarSEqLength) {
	vector<long> data;

	long val = 950263344478331918;
	for (size_t i = 0; i < 1000; i++) {
		for (size_t j = 0; j < i; j++) {
			data.push_back(val);
		}
		val++;
	}
	compressDecompressCheck(data);
}

TEST(CompressionTest, testConstantSequenceDiffLength) {
	vector<long> data;

	long val = 950263344478331918;
	for (size_t i = 0; i < 1024 + 8; i++) {
		for (size_t j = 0; j < i; j++) {
			data.push_back(val);
		}
	}
	compressDecompressCheck(data);
}

TEST(CompressionTest, testSimpleAPI) {
	size_t count = 10000;
	auto dataIn = generateSequece(0, count);
	auto compressed = Scalar<long>::compressSimple(*dataIn);

	ASSERT_NE(compressed->size(), 0) << "Not compressed";

	vector<long> dataOut(count);
	Scalar<long>::decompress(*compressed, count, dataOut);

	for (size_t i = 0; i < count; i++) {
		ASSERT_EQ((*dataIn)[i], dataOut[i]) << "data do not match";
	}

#ifdef USE_AVX512
	compressed = Avx52<long>::compressSimple(*dataIn);

	ASSERT_NE(compressed->size(), 0) << "Not compressed";

	Avx52<long>::decompress(*compressed, count, dataOut);
	for (size_t i = 0; i < count; i++) {
		ASSERT_EQ((*dataIn)[i], dataOut[i]) << "data do not match";
	}
#endif
	delete dataIn;
}

TEST(CompressionTest, testSignChangeAltering) {
	vector<long> data;

	for (size_t i = 0; i < 10000; i++) {
		if (i % 2) {
			data.push_back(i);
		} else {
			data.push_back(-1 * i);
		}
	}
	compressDecompressCheck(data);
}

TEST(CompressionTest, testSequencesDecimal) {
	testSequenceDecimal(0, 100);
	testSequenceDecimal(0, 123);
	testSequenceDecimal(0, 47);
	testSequenceDecimal(0, 8000);
	testSequenceDecimal(20, 50);
	testSequenceDecimal(20, 23);
	testSequenceDecimal(20, 29);
	testSequenceDecimal(20, 30);
	testSequenceDecimal(2147483647, 2147483647L + 1000);
	testSequenceDecimal(9223372036854775807 - 1000, 9223372036854775807);
	testSequenceDecimal(-9223372036854775800, -9223372036854774000);
	testSequenceDecimal(0, 3 * 1000 * 1000);
}

TEST(CompressionTest, testVersionByte) {
	auto data = generateSequece(0, 200);

	auto compressed = Scalar<long>::compressSimple(*data);
	ASSERT_EQ(compressed->at(compressed->size() - 7), 0x7E) << "Missing data header";

#ifdef USE_AVX512
	compressed = Avx52<long>::compressSimple(*data);
	ASSERT_EQ(compressed->at(compressed->size() - 7), 0x7E) << "Missing data header";
#endif
	delete data;
}

}  // end namespace middleout