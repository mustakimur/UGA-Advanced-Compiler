cmake_minimum_required(VERSION 2.8.8)
project(syssec-workshop)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
set(LLVM_LINK_COMPONENTS support)

add_clang_executable(clang-wrapper
  wrap-method.cpp
  )
target_link_libraries(clang-wrapper
  clangTooling
  clangBasic
  clangASTMatchers
  )

install(TARGETS clang-wrapper RUNTIME DESTINATION bin)