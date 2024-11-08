cmake_minimum_required (VERSION 2.8.5)
cmake_policy (SET CMP0054 NEW)
project (Sponge)


# link_directories(/usr/local/lib)

# # 链接 libpcap 库
# target_link_libraries(Sponge pcap)

include (etc/build_defs.cmake)
include (etc/build_type.cmake)
include (etc/cflags.cmake)

include (etc/doxygen.cmake)

include (etc/clang_format.cmake)
include (etc/clang_tidy.cmake)
include (etc/cppcheck.cmake)

include_directories ("${PROJECT_SOURCE_DIR}/libsponge/util")
include_directories ("${PROJECT_SOURCE_DIR}/libsponge/tcp_helpers")
include_directories ("${PROJECT_SOURCE_DIR}/libsponge")

add_subdirectory ("${PROJECT_SOURCE_DIR}/libsponge")

add_subdirectory ("${PROJECT_SOURCE_DIR}/apps")

add_subdirectory ("${PROJECT_SOURCE_DIR}/tests")

add_subdirectory ("${PROJECT_SOURCE_DIR}/doctests")


include (etc/tests.cmake)

add_compile_definitions(
	-DCMAKE_BUILD_TYPE=Release   # optimizations
	-DCMAKE_BUILD_TYPE=Debug     # debug symbols and -Og
	-DCMAKE_BUILD_TYPE=RelASan   # release build with ASan and UBSan
	-DCMAKE_BUILD_TYPE=RelTSan   # release build with ThreadSan (私以为这个大概率用不到)
	-DCMAKE_BUILD_TYPE=DebugASan # debug build with ASan and UBSan
	-DCMAKE_BUILD_TYPE=DebugTSan # debug build with ThreadSan
)

