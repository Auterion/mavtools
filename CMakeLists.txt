cmake_minimum_required(VERSION 3.22)
project(mavtools)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(mavencode)
add_subdirectory(mavdecode)


# install the executables
install(TARGETS mavencode mavdecode
        RUNTIME DESTINATION bin)
