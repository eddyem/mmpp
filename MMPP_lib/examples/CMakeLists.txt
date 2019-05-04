cmake_minimum_required(VERSION 3.9)
project(examples)
link_libraries(mmpp)
include_directories(../)

add_executable(testmove tmcmdlnopts.c tm.c)
target_link_libraries(testmove -lusefull_macros)
add_executable(wheels wheelscmdlnopts.c wheels.c)
target_link_libraries(wheels -lusefull_macros)
