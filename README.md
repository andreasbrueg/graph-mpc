# graph-mpc

## External Dependencies

In order to successfully build the source code, the following external libraries need to be installed on the system.

* [GMP](https://gmplib.org)
* [Boost](https://www.boost.org)
* [OpenSSL](https://github.com/openssl/openssl)
* [EMP Tool](https://github.com/emp-toolkit/emp-tool)
* [nlohmann_json](https://github.com/nlohmann/json)
* [OpenMP](https://www.openmp.org)

## Compiling
``mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8``
