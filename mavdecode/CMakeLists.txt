
project(mavdecode)
add_executable(mavdecode mavdecode.cpp)

target_include_directories(mavdecode PRIVATE ../dependencies)
target_include_directories(mavdecode PRIVATE ../dependencies/libmav/include)

