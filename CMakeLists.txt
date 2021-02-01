# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)


set(CIVETWEB_ENABLE_SSL OFF CACHE BOOL "")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cv_test)

FILE(GLOB_RECURSE app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
target_include_directories(app PRIVATE src)


set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
foreach(inc_file
	ca.der
	server.der
	server_privkey.der
    )
  generate_inc_file_for_target(
    app
    src/${inc_file}
    ${gen_dir}/${inc_file}.inc
    )
endforeach()


file(GLOB_RECURSE web_sources web/*)
set(mfs_file ${ZEPHYR_BINARY_DIR}/include/generated/mfs_data.h)
add_custom_command(
  OUTPUT ${mfs_file}
  COMMAND
  bash
  mkmfs.sh > ${mfs_file}
  DEPENDS ${web_sources} mkmfs.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_target(mfs_file DEPENDS ${mfs_file})
add_dependencies(app mfs_file)
