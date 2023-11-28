file(GLOB CHUGL_UTIL_SOURCES "*.cpp")
file(GLOB CHUGL_UTIL_HEADERS "*.h")

message(STATUS "CHUGL_UTIL_SOURCES: ${CHUGL_UTIL_SOURCES}")
message(STATUS "CHUGL_UTIL_HEADERS: ${CHUGL_UTIL_HEADERS}")

add_library(chugl_util
    ${CHUGL_UTIL_SOURCES}
    ${CHUGL_UTIL_HEADERS}
)

target_link_libraries(chugl_util
    PRIVATE chugl_compiler_flags
)
