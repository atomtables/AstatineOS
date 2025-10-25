cmake_minimum_required(VERSION 3.28.1)
project(AstatineOS)

set(NASM nasm)
set(XXD xxd)

set(CMAKE_C_COMPILER clang)
set(CMAKE_ASM_COMPILER nasm)
set(UTILITIES_BASE_DIR "utilities")
set(MKABP_DIR "${UTILITIES_BASE_DIR}/mkabp")

file(GLOB_RECURSE MKABP_HEADERS "${MKABP_DIR}/*.h")
file(GLOB_RECURSE MKABP_SOURCES "${MKABP_DIR}/*.c")

message(bash -c "${XXD} -i ${PROJECT_SOURCE_DIR}/${MKABP_DIR}/default.bin -n default_bin>${PROJECT_SOURCE_DIR}/${MKABP_DIR}/default.h")

add_custom_target(MkabpPreBuild 
    COMMAND ${NASM} -fbin ${PROJECT_SOURCE_DIR}/${MKABP_DIR}/default.asm -o ${PROJECT_SOURCE_DIR}/${MKABP_DIR}/default.bin
    COMMAND ${CMAKE_COMMAND}
        -DINPUT=${PROJECT_SOURCE_DIR}/${MKABP_DIR}/default.bin
        -DOUTPUT=${PROJECT_SOURCE_DIR}/${MKABP_DIR}/default.h
        -DNAME=default_bin
        -P ${PROJECT_SOURCE_DIR}/cmake/xxd.cmake
    COMMENT "Generating mkabp default header from binary"
)

add_executable(mkabp ${MKABP_HEADERS} ${MKABP_SOURCES})
add_dependencies(mkabp MkabpPreBuild)

# Install mkabp executable
install(TARGETS mkabp
    RUNTIME DESTINATION bin
    COMPONENT utilities)