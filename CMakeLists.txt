cmake_minimum_required (VERSION 2.8.11)
project (Moondust)

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" insource)
if(insource)
    message(FATAL_ERROR "
==== Build in source directory is not allowed! ====
Please run cmake configure from another directory, for example, create a 'build' sub-directory, and run 'cmake ..' from it."
)
endif()

include(ExternalProject)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_DEBUG_POSTFIX "d")

set(DEPENDENCIES_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR})

set(PGE_INSTALL_DIRECTORY "Moondust" CACHE STRING "Name of directory where all binary and data files are will be installed")

# Allow install into space-containing paths
string(REPLACE " " "\\ " CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
string(REPLACE "(" "\\(" CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
string(REPLACE ")" "\\)" CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})

# Get the current working branch
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the latest abbreviated commit hash of the working branch
execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message("== Current GIT hash [${GIT_COMMIT_HASH}], branch [${GIT_BRANCH}]")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
    message("== Using default build configuration which is a Release!")
endif()

include_directories(
    ${DEPENDENCIES_INSTALL_DIR}/include
    ${DEPENDENCIES_INSTALL_DIR}/include/freetype2
    ${DEPENDENCIES_INSTALL_DIR}/include/lua_includes
)
link_directories(${DEPENDENCIES_INSTALL_DIR}/lib)

if(APPLE)
    # Prevent "directory not exists" warnings when building XCode as project
    file(MAKE_DIRECTORY ${DEPENDENCIES_INSTALL_DIR}/lib/Debug)
    file(MAKE_DIRECTORY ${DEPENDENCIES_INSTALL_DIR}/lib/Release)
    # Don't store built executables into "Debug" and "Release" folders
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_BUNDLE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_BUNDLE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
endif()

set(PGE_ENABLE_QT_ENABLED_BY_DEFAULT ON)
if(HAIKU OR EMSCRIPTEN OR ANDROID)
    set(PGE_ENABLE_QT_ENABLED_BY_DEFAULT OFF)
    message("== Qt-dependent components are not supported for this platform")
endif()

if(PGE_ENABLE_QT_ENABLED_BY_DEFAULT)
    find_package(Qt5 COMPONENTS Core)
    if(Qt5_FOUND)
        option(PGE_ENABLE_QT "Enable build of Qt-dependent components" ${PGE_ENABLE_QT_ENABLED_BY_DEFAULT})
        message("== Qt 5 has been found in ${Qt5_DIR}")
        get_filename_component(QT5_BINARY_DIR "${Qt5_DIR}/../../bin" ABSOLUTE)
        find_program(_QT_LRELEASE_PROGRAM lrelease PATHS ${QT5_BINARY_DIR}/bin/)
        if(_QT_LRELEASE_PROGRAM)
            message(STATUS "Found ${_QT_LRELEASE_PROGRAM}, locales will be compiled!")
        else()
            message(WARNING "Unable to find lrelease, locales will NOT be built!")
        endif()
    else()
        message("== Qt 5 NOT FOUND! Qt-dependent components will be disabled!")
    endif()
endif()

# FreeType to render TTF fonts
ExternalProject_Add(
    FREETYPE_Local
    PREFIX ${CMAKE_BINARY_DIR}/external/FreeType
    URL ${CMAKE_SOURCE_DIR}/_Libs/_sources/freetype-2.7.1.tar.gz
    CMAKE_ARGS
        "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
        "-DCMAKE_INSTALL_PREFIX=${DEPENDENCIES_INSTALL_DIR}"
        "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
        -DWITH_ZLIB=OFF -DWITH_BZip2=OFF -DWITH_PNG=OFF -DWITH_HarfBuzz=OFF
)

# SQLite a small database system
ExternalProject_Add(
    SQLite_Local
    PREFIX ${CMAKE_BINARY_DIR}/external/SQLite
    DOWNLOAD_COMMAND ""
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/_Libs/sqlite3
    CMAKE_ARGS
        "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
        "-DCMAKE_INSTALL_PREFIX=${DEPENDENCIES_INSTALL_DIR}"
        "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
)

if(WIN32)
    # UNIX to DOS text format converter utility. Is used to deploy text files on Windows.
    ExternalProject_Add(
        Unix2Dos_Local
        PREFIX ${CMAKE_BINARY_DIR}/external/Unix2Dos
        DOWNLOAD_COMMAND ""
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/_Libs/Unix2Dos
        CMAKE_ARGS
            "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
            "-DCMAKE_INSTALL_PREFIX=${DEPENDENCIES_INSTALL_DIR}"
            "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
    )
endif()

include(_common/unix2dos_copy.cmake)


# FreeType is an image management library
ExternalProject_Add(
    FreeImage_Local
    PREFIX ${CMAKE_BINARY_DIR}/external/FreeImage
    DOWNLOAD_COMMAND ""
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/_Libs/FreeImage
    CMAKE_ARGS
        "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
        "-DCMAKE_INSTALL_PREFIX=${DEPENDENCIES_INSTALL_DIR}"
        "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
)

set(PGE_USE_LUAJIT_ENABLED_BY_DEFAULT ON)
if(ANDROID OR EMSCRIPTEN OR MSVC)
    # Disable LuaJIT for unsupported platforms
    set(PGE_USE_LUAJIT_ENABLED_BY_DEFAULT OFF)
endif()

option(PGE_USE_LUAJIT "Use LuaJIT lua engine" ${PGE_USE_LUAJIT_ENABLED_BY_DEFAULT})

if(PGE_USE_LUAJIT)
    if(WIN32)
        set(MAKECMD "mingw32-make")
    else()
        set(MAKECMD "make")
    endif()
    set(LUAJIT_LIBNAME libluajit$<$<CONFIG:Debug>:d>.a)
    string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)
    if(${CMAKE_BUILD_TYPE_LOWER} STREQUAL "debug")
        message("== LuaJIT will be built in debug mode")
        set(BUILD_FLAGS "CCDEBUG=-g -O0")
    endif()

    if(WIN32)
        set(LUAJIT_INSTALL_COMMAND "")
    else()
        set(LUAJIT_INSTALL_COMMAND
            ${MAKECMD}
            install
            "INSTALL_ANAME=${LUAJIT_LIBNAME}"
            "BUILDMODE=static"
            "PREFIX=${CMAKE_BINARY_DIR}"
        )
    endif()

    set(LUAJIT_INSTALL_DIR ${CMAKE_BINARY_DIR})
    set(LUAJIT_LOCAL_SOURCE_DIR ${CMAKE_BINARY_DIR}/external/luajit/LuaJIT-src)

    ExternalProject_Add(
        LuaJIT_local
        PREFIX ${CMAKE_BINARY_DIR}/external/luajit
        URL ${CMAKE_SOURCE_DIR}/_Libs/_sources/luajit.tar.gz
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND ""
        SOURCE_DIR ${LUAJIT_LOCAL_SOURCE_DIR}
        BUILD_IN_SOURCE 1
        BUILD_COMMAND
            ${MAKECMD} -s
            "BUILDMODE=static"
            "PREFIX=${CMAKE_BINARY_DIR}"
            "CCWARN+=-Wno-unused-function"
            ${BUILD_FLAGS}
        INSTALL_COMMAND "${LUAJIT_INSTALL_COMMAND}"
    )
    if(WIN32)
        set(LUAJIT_BINARY_FILES "luajit.exe")

        ExternalProject_Add_Step(
            LuaJIT_local CreateLuaJitInstallFolder
            COMMAND ${CMAKE_COMMAND} -E make_directory ${LUAJIT_INSTALL_DIR}/include/luajit-2.1 ${LUAJIT_INSTALL_DIR}/lib ${LUAJIT_INSTALL_DIR}/bin
            COMMENT "Creating install folder"
            DEPENDEES BUILD
        )

        ExternalProject_Add_Step(
            LuaJIT_local InstallLuaJitHeaders
            COMMAND ${CMAKE_COMMAND} -E copy lua.h lua.hpp luaconf.h luajit.h lualib.h lauxlib.h ${LUAJIT_INSTALL_DIR}/include/luajit-2.1
            WORKING_DIRECTORY ${LUAJIT_LOCAL_SOURCE_DIR}/src
            COMMENT "Installing luajit headers"
            DEPENDEES BUILD CreateLuaJitInstallFolder
        )

        ExternalProject_Add_Step(
            LuaJIT_local InstallLuaJitLib
            COMMAND ${CMAKE_COMMAND} -E copy libluajit.a "${LUAJIT_INSTALL_DIR}/lib/${LUAJIT_LIBNAME}"
            WORKING_DIRECTORY ${LUAJIT_LOCAL_SOURCE_DIR}/src
            COMMENT "Installing luajit lib"
            DEPENDEES BUILD CreateLuaJitInstallFolder
        )

        ExternalProject_Add_Step(
            LuaJIT_local InstallLuaJitDLL
            COMMAND ${CMAKE_COMMAND} -E copy ${LUAJIT_BINARY_FILES} ${LUAJIT_INSTALL_DIR}/bin
            WORKING_DIRECTORY ${LUAJIT_LOCAL_SOURCE_DIR}/src
            COMMENT "Installing luajit shared library"
            DEPENDEES BUILD CreateLuaJitInstallFolder
        )
    endif() #NOT WIN32
endif()

# LuaBind is a powerful lua binding library for C++
ExternalProject_Add(
    LuaBind_Local
    PREFIX ${CMAKE_BINARY_DIR}/external/luabind
    DOWNLOAD_COMMAND ""
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/_Libs/luabind
    CMAKE_ARGS
        "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
        "-DCMAKE_INSTALL_PREFIX=${DEPENDENCIES_INSTALL_DIR}"
        "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
        "-DCMAKE_DEBUG_POSTFIX=d"
        $<$<BOOL:PGE_USE_LUAJIT>:-DUSE_LUAJIT=ON>
)
if(PGE_USE_LUAJIT)
    add_dependencies(LuaBind_Local LuaJIT_local)
endif()

if(NOT WIN32)
    set(PATCH_CMD "patch")
else()
    # Use one from Git Bash toolchain, otherwise one from MinGW toolchain will crash
    set(PATCH_CMD "C:/Program Files/Git/usr/bin/patch.exe")
endif()

# Simple Direct-Media Layer library, dependency of AudioCodecs and SDL Mixer X
ExternalProject_Add(
    SDL2_Local
    PREFIX ${CMAKE_BINARY_DIR}/external/SDL2
    URL ${CMAKE_SOURCE_DIR}/_Libs/_sources/SDL-default.tar.gz
    PATCH_COMMAND ${PATCH_CMD} -p1 "<" "${CMAKE_SOURCE_DIR}/_Libs/_sources/patches/SDL2_cmake_no_wasapi.patch"
    CMAKE_ARGS
        "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
        "-DCMAKE_INSTALL_PREFIX=${DEPENDENCIES_INSTALL_DIR}"
        "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
        -DSDL_SHARED=OFF
        $<$<BOOL:WIN32>:-DENABLE_WASAPI_SUPPORT=OFF>
        $<$<BOOL:LINUX>:-DSNDIO=OFF>
        $<$<STREQUAL:${CMAKE_SYSTEM_NAME},Emscripten>:-DEXTRA_CFLAGS=-s USE_PTHREADS=1>
        $<$<STREQUAL:${CMAKE_SYSTEM_NAME},Emscripten>:-DPTHREADS=ON>
        $<$<STREQUAL:${CMAKE_SYSTEM_NAME},Emscripten>:-DPTHREADS_SEM=ON>
)

# A collection of audio codecs libraries, dependency of SDL Mixer X
ExternalProject_Add(
    AudioCodecs_Local
    PREFIX ${CMAKE_BINARY_DIR}/external/AudioCodecs
    DOWNLOAD_COMMAND ""
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/_Libs/AudioCodecs
    CMAKE_ARGS
        "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
        "-DCMAKE_INSTALL_PREFIX=${DEPENDENCIES_INSTALL_DIR}"
        "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
        "-DSDL2_REPO_PATH=${DEPENDENCIES_INSTALL_DIR}"
        "-DCMAKE_DEBUG_POSTFIX=d"
        $<$<STREQUAL:${CMAKE_SYSTEM_NAME},Emscripten>:-DADLMIDI_USE_DOSBOX_EMULATOR=ON>
    DEPENDS SDL2_Local
)

InstallTextFile(FILE "${CMAKE_SOURCE_DIR}/_Libs/AudioCodecs/libFLAC/COPYING.Xiph" RENAME "License.FLAC.txt" DESTINATION "${PGE_INSTALL_DIRECTORY}/licenses")
InstallTextFile(FILE "${CMAKE_SOURCE_DIR}/_Libs/AudioCodecs/libogg/COPYING" RENAME "License.OGG.txt" DESTINATION "${PGE_INSTALL_DIRECTORY}/licenses")
InstallTextFile(FILE "${CMAKE_SOURCE_DIR}/_Libs/AudioCodecs/libvorbis/COPYING" RENAME "License.Vorbis.txt" DESTINATION "${PGE_INSTALL_DIRECTORY}/licenses")
InstallTextFile(FILE "${CMAKE_SOURCE_DIR}/_Libs/AudioCodecs/libtimidity/COPYING" RENAME "License.Timidity.txt" DESTINATION "${PGE_INSTALL_DIRECTORY}/licenses")
InstallTextFile(FILE "${CMAKE_SOURCE_DIR}/_Libs/AudioCodecs/libmad/COPYING" RENAME "License.libMAD.txt" DESTINATION "${PGE_INSTALL_DIRECTORY}/licenses")
InstallTextFile(FILE "${CMAKE_SOURCE_DIR}/_Libs/AudioCodecs/libid3tag/COPYING" RENAME "License.libID3Tag.txt" DESTINATION "${PGE_INSTALL_DIRECTORY}/licenses")

# SDL Mixer X - an audio library, fork of SDL Mixer
ExternalProject_Add(
    SDLMixerX_Local
    PREFIX ${CMAKE_BINARY_DIR}/external/SDLMixerX
    DOWNLOAD_COMMAND ""
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/_Libs/SDL_Mixer_X/
    CMAKE_ARGS
        "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
        "-DCMAKE_INSTALL_PREFIX=${DEPENDENCIES_INSTALL_DIR}"
        "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
        "-DAUDIO_CODECS_REPO_PATH=${DEPENDENCIES_INSTALL_DIR}"
        "-DCMAKE_DEBUG_POSTFIX=d"
        "-DSDL_MIXER_X_SHARED=OFF"
        $<$<STREQUAL:${CMAKE_SYSTEM_NAME},Emscripten>:-DUSE_FLAC=OFF>
    DEPENDS AudioCodecs_Local SDL2_Local
)

# Append licenses of libraries
InstallTextFile(FILE "${CMAKE_SOURCE_DIR}/_Libs/SDL_Mixer_X/COPYING.txt" RENAME "License.SDL2_mixer_ext.txt" DESTINATION "${PGE_INSTALL_DIRECTORY}/licenses")


option(PGE_BUILD_ENGINE "Build the PGE Engine component" ON)
if(PGE_BUILD_ENGINE)
    add_subdirectory(Engine)
    # Append changelog file
    InstallTextFile(FILE "${CMAKE_SOURCE_DIR}/changelog.engine.txt" RENAME "changelog.engine.txt" DESTINATION "${PGE_INSTALL_DIRECTORY}")
    # Append license files
    InstallTextFile(FILE "${CMAKE_SOURCE_DIR}/Engine/LICENSE.mit" RENAME "pge_engine.license.mit.txt" DESTINATION "${PGE_INSTALL_DIRECTORY}")
    InstallTextFile(FILE "${CMAKE_SOURCE_DIR}/Engine/LICENSE.gpl3" RENAME "pge_engine.license.gpl3.txt" DESTINATION "${PGE_INSTALL_DIRECTORY}")
    InstallTextFile(FILE "${CMAKE_SOURCE_DIR}/Content/readmes/Engine.Readme.txt" RENAME "pge_engine.readme.txt" DESTINATION "${PGE_INSTALL_DIRECTORY}")
endif()

if(NOT EMSCRIPTEN AND NOT ANDROID)
    option(PGE_BUILD_GIFS2PNG "Build the GIFs2PNG utility component" ON)
    if(PGE_BUILD_GIFS2PNG)
        add_subdirectory(GIFs2PNG)
        InstallTextFile(FILE "${CMAKE_SOURCE_DIR}/Content/readmes/GIFs2PNG_Readme.txt" RENAME "GIFs2PNG.readme.txt" DESTINATION "${PGE_INSTALL_DIRECTORY}")
    endif()

    option(PGE_BUILD_PNG2GIFS "Build the GIFs2PNG utility component" ON)
    if(PGE_BUILD_PNG2GIFS)
        add_subdirectory(PNG2GIFs)
        InstallTextFile(FILE "${CMAKE_SOURCE_DIR}/Content/readmes/PNG2GIFs_Readme.txt" RENAME "PNG2GIFs.readme.txt" DESTINATION "${PGE_INSTALL_DIRECTORY}")
    endif()

    option(PGE_BUILD_LAZYFIXTOOL "Build the LazyFixTool utility component" ON)
    if(PGE_BUILD_LAZYFIXTOOL)
        add_subdirectory(LazyFixTool)
        InstallTextFile(FILE "${CMAKE_SOURCE_DIR}/Content/readmes/LazyFixTool_Readme.txt" RENAME "LazyFixTool.readme.txt" DESTINATION "${PGE_INSTALL_DIRECTORY}")
    endif()
endif()

if(PGE_ENABLE_QT AND NOT ANDROID)
    option(PGE_BUILD_EDITOR "Build the PGE Editor component (Qt5 required)" ON)
    if(PGE_BUILD_EDITOR)
        add_subdirectory(Editor)
        # Append changelog file
        InstallTextFile(FILE "${CMAKE_SOURCE_DIR}/changelog.editor.txt" RENAME "changelog.editor.txt" DESTINATION "${PGE_INSTALL_DIRECTORY}")
    endif()

    option(PGE_BUILD_PLAYERCALIBRATOR "Build the Player Calibrator utility component (Qt5 required)" ON)
    if(PGE_BUILD_PLAYERCALIBRATOR)
        add_subdirectory(PlayableCalibrator)
    endif()

    option(PGE_BUILD_MUSICPLAYER "Build the SDL Mixer X Music Player component (Qt5 required)" ON)
    if(PGE_BUILD_MUSICPLAYER)
        add_subdirectory(MusicPlayer)
    endif()

    option(PGE_BUILD_MAINTAINER "Build the Maintainer utility component (Qt5 required)" ON)
    if(PGE_BUILD_MAINTAINER)
        add_subdirectory(Maintainer)
    endif()
endif()

# Append Project License file
InstallTextFile(FILE "${CMAKE_SOURCE_DIR}/LICENSE" RENAME "LICENSE.txt" DESTINATION "${PGE_INSTALL_DIRECTORY}")

# Process UNIX2DOS if needed
InstallTextFiles()
