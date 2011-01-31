include_directories(${libclogger_SOURCE_DIR}/src)
include_directories(${libclogger_SOURCE_DIR}/include)
link_directories(${libclogger_BINARY_DIR}/libclogger)

add_executable(simple simple.c)

target_link_libraries(simple libclogger)
