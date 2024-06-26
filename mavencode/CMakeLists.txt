
project(mavencode)
add_executable(mavencode mavencode.cpp)


target_include_directories(mavencode PRIVATE ../dependencies)
target_include_directories(mavencode PRIVATE ../dependencies/libmav/include)
target_include_directories(mavencode PRIVATE ../dependencies/rapidjson/include)
