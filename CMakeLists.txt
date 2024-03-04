cmake_minimum_required(VERSION 3.20)

option(FORMAT_SOURCES "execute clang-format" ON)

project(joymouse C)
enable_testing()

if(FORMAT_SOURCES)
  file(GLOB sources LIST_DIRECTORIES false CONFIGURE_DEPENDS "*.h" "*.c")
  find_program(CLANG_FORMAT_EXE clang-format)
  add_custom_target(${PROJECT_NAME}-format ALL
    COMMAND ${CLANG_FORMAT_EXE} -style=file -i ${sources}
  )
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(JSL_PLATFORM x64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(JSL_PLATFORM x86)
else()
  message(FATAL_ERROR "failed to detect environment")
endif()

set(EXPORT_DIR_NAME "bin" CACHE STRING "export directory")
if ("${EXPORT_DIR_NAME}" STREQUAL "")
  set(EXPORT_DIR_NAME "bin" CACHE STRING "export directory" FORCE)
endif()
set(EXPORT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_DIR_NAME}")

set(JSL_URL "https://github.com/JibbSmart/JoyShockLibrary/releases/download/v3.0/JSL_3_0.zip")
set(JSL_DIR "${CMAKE_CURRENT_BINARY_DIR}/JSL")

string(REGEX MATCH "[^/]+$" JSL_ARCHIVE_NAME "${JSL_URL}")
set(JSL_ARCHIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${JSL_ARCHIVE_NAME}")
if(NOT EXISTS "${JSL_ARCHIVE_PATH}")
  file(DOWNLOAD "${JSL_URL}" "${JSL_ARCHIVE_PATH}")
endif()

if(NOT EXISTS "${JSL_DIR}")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xf ${JSL_ARCHIVE_PATH}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  )
  file(MAKE_DIRECTORY "${EXPORT_DIR}")
  file(COPY_FILE "${JSL_DIR}/${JSL_PLATFORM}/JoyShockLibrary.dll" "${EXPORT_DIR}/JoyShockLibrary.dll")
endif()

set(is_clang "$<C_COMPILER_ID:Clang>")
set(v16_or_later "$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,16>")
add_library(joymouse_intf INTERFACE)
target_compile_definitions(joymouse_intf INTERFACE
  $<$<BOOL:${WIN32}>:_WIN32_WINNT=0x0502>
  _WINDOWS
  $<$<CONFIG:Release>:NDEBUG>
)
target_compile_options(joymouse_intf INTERFACE
  $<$<AND:$<BOOL:${WIN32}>,$<BOOL:${USE_COMPILER_RT}>>:--rtlib=compiler-rt>
  -mstackrealign
  -Wall
  -Wextra
  -Werror
  -Weverything
  -Wshadow
  -Werror=return-type
  -pedantic-errors
  -Wno-declaration-after-statement
  -Wno-padded
  $<$<AND:${is_clang},${v16_or_later}>:-Wno-unsafe-buffer-usage>
  -ffunction-sections
  -fdata-sections
  $<$<CONFIG:Debug>:-O0>
  $<$<CONFIG:Release>:-O2>
  $<$<BOOL:${USE_ADDRESS_SANITIZER}>:-fsanitize=address>
  -flto
)
target_link_options(joymouse_intf INTERFACE
  -fuse-ld=lld
  -Wl,--gc-sections
  # -Wl,--print-gc-sections
  -Wl,--kill-at
  $<$<BOOL:${USE_ADDRESS_SANITIZER}>:-fsanitize=address>
  $<$<CONFIG:Release>:-s>
)
target_link_directories(joymouse_intf INTERFACE "${JSL_DIR}/${JSL_PLATFORM}")
target_link_libraries(joymouse_intf INTERFACE JoyShockLibrary.dll -Wl,-delayload,JoyShockLibrary.dll)

add_executable(joymouse main.c)
set_target_properties(joymouse PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${EXPORT_DIR}"
)
target_link_libraries(joymouse PRIVATE joymouse_intf ole32)
