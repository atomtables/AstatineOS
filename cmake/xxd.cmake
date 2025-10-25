# cmake/xxd.cmake
# gpt removed the gpt header lmao :pray:
# Usage: cmake -DINPUT=<path/to/file.bin> -DOUTPUT=<path/to/file.h> -DNAME=<varname> -P xxd.cmake

# Read binary as HEX
file(READ "${INPUT}" _hex HEX)

# Convert hex string to list of bytes
string(REGEX MATCHALL "[0-9A-Fa-f][0-9A-Fa-f]" _bytes "${_hex}")

# Format bytes with proper line breaks
set(_formatted "")
set(_count 0)
foreach(_byte ${_bytes})
    if(_count EQUAL 0)
        string(APPEND _formatted "  ")
    endif()
    
    string(APPEND _formatted "0x${_byte}")
    
    math(EXPR _count "${_count} + 1")
    
    list(LENGTH _bytes _total)
    if(NOT _count EQUAL _total)
        string(APPEND _formatted ", ")
    endif()
    
    if(_count EQUAL 12)
        string(APPEND _formatted "\n")
        set(_count 0)
    endif()
endforeach()

# Write header file
file(WRITE "${OUTPUT}" "unsigned char ${NAME}[] = {\n${_formatted}\n};\n")
file(APPEND "${OUTPUT}" "unsigned int ${NAME}_len = sizeof(${NAME});\n")
