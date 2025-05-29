cmake_minimum_required(VERSION 3.19)
project(fts5_mecab C)

# mecab lib 및 사전 빌드
add_custom_target(build_mecab_libs ALL
  COMMAND bash ${CMAKE_SOURCE_DIR}/build.sh
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Executing shell script to Build mecab lib"
)

# Mecab include/lib 디렉토리 지정
set(MECAB_PREFIX "${CMAKE_SOURCE_DIR}/build")

include_directories(${MECAB_PREFIX}/include)
link_directories(${MECAB_PREFIX}/lib)

add_library(fts5_mecab SHARED fts5_mecab.c)
target_link_libraries(fts5_mecab PRIVATE mecab)
add_dependencies(fts5_mecab build_mecab_libs)

if(APPLE)
  set_target_properties(fts5_mecab PROPERTIES
      INSTALL_NAME_DIR "@rpath"
      INSTALL_RPATH "@loader_path/lib"
      BUILD_WITH_INSTALL_RPATH TRUE
  )
  add_custom_command(TARGET fts5_mecab POST_BUILD
    COMMAND install_name_tool -change
            "${MECAB_PREFIX}/lib/libmecab.2.dylib" # Current absolute path in fts5_mecab
            "@loader_path/lib/libmecab.2.dylib"    # Desired relative path
            $<TARGET_FILE:fts5_mecab>
    COMMENT "Changing libmecab.2.dylib path in fts5_mecab.dylib"
  )
endif()

if(DEBUG)
  target_compile_definitions(fts5_mecab PRIVATE DEBUG)
endif()
if(STOP789)
  # 특수문자 indexing 제외
  target_compile_definitions(fts5_mecab PRIVATE STOP789)
endif()

