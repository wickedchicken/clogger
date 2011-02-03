include_directories(${libclogger_SOURCE_DIR}/include)
add_library(clogger STATIC SHARED clogger.c)

install (TARGETS clogger DESTINATION lib)
install (FILES "${libclogger_SOURCE_DIR}/include/clogger.h" DESTINATION include)
