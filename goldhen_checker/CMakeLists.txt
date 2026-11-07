cmake_minimum_required(VERSION 3.10)
project(goldhen_checker)

set(CMAKE_C_STANDARD 11)

add_executable(${PROJECT_NAME}.elf src/main.c)

# verify import lib names against the OpenORBIS SDK version in the builder image
target_link_libraries(${PROJECT_NAME}.elf
  stdc++
  SceLibKernel
  SceSysmem
  SceIofilemgr
  SceThreadmgr
  SceSystemService
)
