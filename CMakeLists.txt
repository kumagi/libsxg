# Copyright 2019 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.12)
project(sxg)

set(CMAKE_PROJECT_VERSION "0")
set(CMAKE_PROJECT_VERSION_MAJOR "0")
set(CMAKE_PROJECT_VERSION_MINOR "1")
set(CMAKE_PROJECT_VERSION_PATCH "0")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++11")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -std=c99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -ggdb")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -ggdb")

include(ExternalProject)
set_property(DIRECTORY ${PROJECT_SOURCE_DIR}
  PROPERTY EP_UPDATE_DISCONNECTED 1)

find_package(OpenSSL REQUIRED)

add_library(sxg
  src/sxg_buffer.c
  src/sxg_buffer_debug.c
  src/sxg_codec.c
  src/sxg_generate.c
  src/sxg_raw_response.c
  src/sxg_encoded_response.c
  src/sxg_header.c
  src/sxg_sig.c
  src/sxg_signer_list.c
)

target_include_directories(sxg
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(sxg
  INTERFACE
    ${OPENSSL_LIBRARIES}
)

########################################
## Testing
########################################

enable_testing()
file(COPY tests/testdata DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

ExternalProject_Add(
  gtest
  GIT_REPOSITORY https://github.com/google/googletest
  GIT_TAG master
  SOURCE_DIR ${PROJECT_BINARY_DIR}/third_party/gtest
  BINARY_DIR ${PROJECT_BINARY_DIR}/gtest
  INSTALL_COMMAND ""
)
add_library(libgtest IMPORTED STATIC GLOBAL)
add_dependencies(libgtest gtest)
set_target_properties(libgtest PROPERTIES
    "IMPORTED_LOCATION" "${PROJECT_BINARY_DIR}/gtest/lib/libgtest.a"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
)
add_library(libgtest_main IMPORTED STATIC GLOBAL)
add_dependencies(libgtest_main gtest)
set_target_properties(libgtest_main PROPERTIES
    "IMPORTED_LOCATION" "${PROJECT_BINARY_DIR}/gtest/lib/libgtest_main.a"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
)
set(GTEST_INCLUDE ${PROJECT_BINARY_DIR}/third_party/gtest/googletest/include)

function (add_plain_test test_name)
  set(target_name "${test_name}")
  add_executable(${target_name} tests/${test_name}.cc)
  add_test(
    NAME ${target_name}
    COMMAND ${target_name}
  )
  add_dependencies(${target_name} sxg libgtest libgtest_main gtest)
  target_link_libraries(${target_name} sxg libgtest libgtest_main pthread)
  target_include_directories(${target_name}
    PRIVATE
      ${PROJECT_SOURCE_DIR}/include
      ${GTEST_INCLUDE}
  )
endfunction()

set(SANITIZER_OPTIONS "-fsanitize=address,leak,undefined")
function (add_sanitizer_test test_name)
  set(target_name "${test_name}_sanitizer")
  add_executable(${target_name} tests/${test_name}.cc)
  target_compile_options(${target_name}
    PRIVATE
    ${SANITIZER_OPTIONS}
  )
  target_link_options(${target_name}
    PRIVATE
    ${SANITIZER_OPTIONS}
  )

  add_test(
    NAME ${target_name}
    COMMAND ${target_name}
  )
  add_dependencies(${target_name} sxg libgtest libgtest_main gtest)
  target_link_libraries(${target_name} sxg libgtest libgtest_main pthread)
  target_include_directories(${target_name}
    PRIVATE
      ${PROJECT_SOURCE_DIR}/include
      ${GTEST_INCLUDE}
  )
endfunction()

function(configure_test test_name)
  message("-- Configuring test: ${test_name}")
  add_plain_test(${test_name})
  add_sanitizer_test(${test_name})
endfunction()
configure_test(sxg_buffer_test)
configure_test(sxg_codec_test)
configure_test(sxg_header_test)
configure_test(sxg_sig_test)
configure_test(sxg_encoded_response_test)
configure_test(sxg_signer_list_test)
configure_test(toplevel_test)
configure_test(sxg_generate_test)
configure_test(nfail_malloc_test)



########################################
## Fuzzing
########################################

macro(configure_fuzzer fuzzer_name)
  message("-- Configuring fuzzer: ${fuzzer_name}")
  add_executable(${fuzzer_name} fuzzers/${fuzzer_name}.cc)
  target_compile_options(${fuzzer_name}
    PRIVATE
    -fsanitize=address,fuzzer
  )
  target_link_options(${fuzzer_name}
    PRIVATE
    -fsanitize=address,fuzzer
  )

  add_dependencies(${fuzzer_name} sxg)
  target_link_libraries(${fuzzer_name}
    PRIVATE
      sxg
      ${OPENSSL_LIBRARIES}
  )
  target_include_directories(${fuzzer_name}
    PRIVATE
      ${PROJECT_SOURCE_DIR}/include
  )
endmacro()

configure_fuzzer(signer_fuzzer)
