# Middle-out Compression for Time-series Data

We all know, how the Middle-out compression works at the HBO show [1], but how do we actually apply this to the time-series data?
Take a look at the following schema. There is an input vector of a data. This vector is divided to Middle-out segments. For simplicity, in the image there are only four of a segments, but compression actually uses eight. This is because the AVX-512 vector instructions can process up to 8 doubles at once. If we used a single precision numbers, then we’d have 16 Middle-out segments and proceed 16 elements at a time.
The first elements of each segment (denoted by blue) are referencing values. These values are stored as-is in the front of a compressed data.

![Middle-out overview](/images/middle-out-overview.svg)

As long as we have multiple referencing points, we can compute diffs between the following values in parallel. That allows us to iterate through all Middle-out segments simultaneously, in the same block of the SIMD instructions. Within each iteration, the current values are XORed with the previous ones and the results of this operation are stored in one data block. 
If there is a reminder of the division of the length of the input vector by the count of Middle-out blocks, values from that reminder are stored uncompressed at the end of compressed data.
Of course the uncompressed header and footer add some space overhead, but we assume  that the overhead would be amortized among at least a few thousands values.

![Middle-out data block](/images/middle-out-data-block.svg)

Each data block starts with a bit mask determining what values are the same as the previous ones. If the corresponding bit is set, then we know that the value didn’t change. If all of the bits are set, no further information is stored - all values are the same as the previous ones. In that case, we are able to store an unchanged value in just single bit.
Next, we store the right offsets (trailing zeros) rounded down to bytes. As long as we are addressing whole bytes, we need only 3 bits to store that offset. Only the offsets respective to the changed values are stored. Then the max length (within this block) of the non-zero XORed value is stored. This length holds bytes as well, and we know the max length could not be 0;  that would mean all the values are the same as the previous ones and we wouldn't store this information at all. Hence we need to store only lengths from 1 to 8 that can be stored in 3 bits. We store length only once per block, because we assume that all compressed data would have, in general, the same characteristics, i.e. changing by approximately the same value. All these values encoded in 3 bits (offsets and length) are conjoined and an optional padding is inserted if needed to reach the byte boundary.
Lastly, the XORs parts are stored at the end of the the data block.

## Scalar vs AVX-512 Implementation
This repository contains two implementations. The first is scalar implementation targeting pre-AVX-512 CPUs. Second implmementation is written in AVX-512 intrinsics and offers great speed up over the scalar implementation. The vectorized implementation can be build with `make` targets suffixed `-avx512`.

## Target Platforms
Both implementations in this repo targetting x86 (little endian). Works with `g++` compiler version **7.2+** in the Unix ecosystem.

## Performance
Throughput is measured on a single core of Skylake-X Xeon running at 2.0 GHz.
Compress ratios on example datasets vary from 1.3 to 3.3.

|            | Scalar                | AVX-512        |
|------------|-----------------------|----------------|
| Compress   | 0.7 - 1.7 GB/s        | 2.3 - 2.5 GB/s |
| Decompress | 2.3 - 2.9 GB/s        | 3.4 - 4.8 GB/s |

[More informations](https://medium.com/@vaclav.loffelmann/the-worlds-first-middle-out-compression-for-time-series-data-part-1-1e5ad5312757 "The World’s First Middle-Out Compression for Time-series Data — Part 1").

# Compile libs

```
make lib
```
or 
```
make lib-avx512
```

### Compile and Run Tests
```
make test
```
or
```
make test-avx512
```

### Compile and Run Performance Tests
```
make perf
```
or
```
make perf-avx512
```

### Compile and Run Example
```
make lib ; make lib-avx512
make install-libs-example
cd example
make
```

### Example code
```c++
#include "middleout.hpp" 

vector<double>& dataIn = ...data...;
size_t count = dataIn.size();

// COMPRESSION
vector<char> compressed(middleout::maxCompressedSize(count));
size_t compressedLength = middleout::compress(dataIn, compressed);
// "compressed" contains compressed data between 0 and compressedLength th element

// DECOMPRESSION
vector<double> dataOut(count);
middleout::decompress(compressed, count, dataOut);
// "dataOut" contains decompressed data

```

## Dev dependencies

*  [Google Test](https://github.com/google/googletest)
*  [Google Benchmark](https://github.com/google/benchmark)

### Debian/Ubuntu
install gtest
```
apt install libgtest-dev build-essential cmake
cd /usr/src/gtest
cmake .
make -j4
cp *.a /usr/lib
```

install google benchmark
```
cd /usr/lib
git clone https://github.com/google/benchmark.git
cd benchmark
cmake . -DCMAKE_BUILD_TYPE=Release
make -j4
make install
```


## License
Please see the LICENSE.md file.

## References

[1] https://vimeo.com/97107551

