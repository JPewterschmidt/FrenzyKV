# FrenzyKV

A LSM KV database.

## Credits

- Implementation and tests of Bloom filter from [LevelDB](https://github.com/google/leveldb).
- [nlohmann json](https://nlohmann.github.io/json/): JSON for Modern C++.
- [Zstandard](https://facebook.github.io/zstd/): Zstandard - Fast real-time compression algorithm 
- [crc32c](https://github.com/google/crc32c): CRC32C implementation with support for CPU-specific acceleration instructions.
- [smhasher](https://github.com/aappleby/smhasher): A test suite designed to test the distribution, collision, and performance properties of non-cryptographic hash functions. 
- [Magic Enum](https://github.com/Neargye/magic_enum): Static reflection for enums (to string, from string, iteration) for modern C++, work with any enum type without any macro or boilerplate code.
- [libuuid](https://libuuid.sourceforge.io/): Portable uuid C library.
- [GoogleTest](https://google.github.io/googletest/): GoogleTest - Google Testing and Mocking Framework.
- [benchmark](https://github.com/google/benchmark): A microbenchmark support library.
- [xmake](https://xmake.io): 🔥 A cross-platform build utility based on Lua 
- [Koios](https://github.com/JPewterschmidt/koios): A C++ coroutine library.
- [toolpex](https://github.com/JPewterschmidt/toolpex): A C++ Utility Toolbox.

## Acknowledgement

System API design and system structure were inspired by Google's LevelDB, 
since I have not yet get enough experience to design such a system.
But There's no code copied directly from LevelDB except for bloom filter and it's tests.
Both two files were marked Google's copyright declaration.
