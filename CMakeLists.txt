project (coroutine CXX)
add_executable(coroutine main.cpp Logging.cpp Exception.cpp Buffer.cpp Coroutine.cpp)
add_executable(example ucontext_example.cpp)
set (CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -Wconversion -Wunused -Wno-unused-parameter -Wold-style-cast -Woverloaded-virtual -Wpointer-arith -Wshadow -Wwrite-strings -march=native" ) 
set (CMAKE_CXX_COMPILER "g++")
set (CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set (CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

