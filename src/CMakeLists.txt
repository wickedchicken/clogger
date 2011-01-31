include_directories(${libclogger_SOURCE_DIR}/include)
add_library(libclogger STATIC SHARED clogger.c)

install (TARGETS libclogger DESTINATION lib)
install (FILES "${libclogger_SOURCE_DIR}/include/clogger.h" DESTINATION include)
