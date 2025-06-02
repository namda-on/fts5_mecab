cmake_minimum_required(VERSION 3.19)
project(fts5_mecab C)

# mecab lib 및 사전 빌드
add_custom_target(build_mecab_libs ALL
  COMMAND sh ${CMAKE_SOURCE_DIR}/build.sh
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Executing shell script to Build mecab lib"
)

# Mecab include/lib 디렉토리 지정
set(MECAB_PREFIX "${CMAKE_SOURCE_DIR}/build")
# Mecab dictionary를 상대경로로 지정 (working directory 기준)
set(MECAB_DIC_RELATIVE_PATH "./fts-jp-mecab/lib/dict")

include_directories(${MECAB_PREFIX}/include)
link_directories(${MECAB_PREFIX}/lib)

add_library(fts5_mecab SHARED fts5_mecab.c)
target_link_libraries(fts5_mecab PRIVATE mecab)
add_dependencies(fts5_mecab build_mecab_libs)

add_custom_command(TARGET fts5_mecab POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E echo "dicdir = ${MECAB_DIC_RELATIVE_PATH}" > ${MECAB_PREFIX}/mecabrc
  COMMENT "Create mecab rc file to set dictionary path"
)

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

  add_custom_command(TARGET build_mecab_libs POST_BUILD
    COMMAND install_name_tool -id "@rpath/libmecab.2.dylib" "${MECAB_PREFIX}/lib/libmecab.2.dylib"
    COMMENT "Changing install_name of libmecab.2.dylib to @rpath/lib/libmecab.2.dylib"
  )
elseif(UNIX)
  set_target_properties(fts5_mecab PROPERTIES
    INSTALL_RPATH "$ORIGIN/lib"
    INSTALL_RPATH_USE_LINK_PATH TRUE
    BUILD_WITH_INSTALL_RPATH TRUE
  )
endif()

if(DEBUG)
  target_compile_definitions(fts5_mecab PRIVATE DEBUG)
endif()
if(STOP789)
  # 특수문자 indexing 제외
  target_compile_definitions(fts5_mecab PRIVATE STOP789)
endif()

