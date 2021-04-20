cmake_minimum_required(VERSION 3.10)

project (VoyaMedia)

# Voya Media depends on the following libraries:
# - curl, ffmpeg, FreeImage, freetype, libxml2, mjson, openssl, sdl2, sdl2_ttf, sqlite, zlib
# You can find the library sources in SOURCEFORGE_URL below:

set(SOURCEFORGE_URL "https://sourceforge.net/projects/voyamedia/files/VoyaMedia/3.x")
set(EXT_LIB_DIR     "__EXT_LIB_DIR__")
set(INC_DIR         "${EXT_LIB_DIR}/include")
set(LIB_DIR         "${EXT_LIB_DIR}/lib")
set(CPP_VERSION     "14")

if (APPLE)
    file(GLOB LIBS_DYLIB "${LIB_DIR}/*.dylib")

    set(OS                  "_macosx")
    set(CMAKE_CXX_FLAGS     "${CMAKE_CXX_FLAGS} -ObjC++ -std=c++${CPP_VERSION}")
    set(LIBS                ${LIBS_DYLIB})

    execute_process(COMMAND xcrun --sdk macosx --show-sdk-path OUTPUT_VARIABLE OSX_SDK_PATH)
	set(OSX_FRAMEWORKS_PATH "${OSX_SDK_PATH}/System/Library/Frameworks")
elseif (UNIX)
    file(GLOB LIBS_SO "${LIB_DIR}/*.so")

    set(OS "_linux")

    execute_process(COMMAND pkg-config --cflags gtk+-3.0 OUTPUT_VARIABLE GTK_CFLAGS)
    string(REPLACE "-I" " -I " GTK_CFLAGS2 ${GTK_CFLAGS})
    separate_arguments(GTK UNIX_COMMAND ${GTK_CFLAGS2})

    execute_process(COMMAND pkg-config --libs gtk+-3.0 OUTPUT_VARIABLE GTK_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(LIBS ${LIBS_SO} ${GTK_LIBS})
elseif (WIN32)
    file(GLOB LIBS_LIB RELATIVE ${LIB_DIR} "${LIB_DIR}/*.lib")
    
    set(OS     "_windows")
    set(DIRENT "__DIRENT__")
    set(LIBS   ${LIBS_LIB} IPHLPAPI.lib Ws2_32.lib)
    set(RES    "build/windows/VoyaMedia.rc")

    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT VoyaMedia)
endif()

file(GLOB SOURCES_H   "src/*/*.h")
file(GLOB SOURCES_CPP "src/*/*.cpp")

add_definitions(-D_${OS})

include_directories(${INC_DIR} ${INC_DIR}/freetype ${INC_DIR}/freetype2 ${INC_DIR}/libxml2 ${INC_DIR}/mjson-1.7 ${DIRENT} ${GTK})
link_directories(${LIB_DIR})

add_executable(VoyaMedia ${SOURCES_CPP} ${SOURCES_H} ${RES})
target_link_libraries(VoyaMedia ${LIBS})

if (NOT EXISTS ./NotoSans-Merged.ttf)
    file(DOWNLOAD ${SOURCEFORGE_URL}/Fonts/NotoSans-Merged.ttf  ./NotoSans-Merged.ttf)
    file(DOWNLOAD ${SOURCEFORGE_URL}/Fonts/NotoSansCJK-Bold.ttc ./NotoSansCJK-Bold.ttc)
endif()

if (APPLE OR UNIX)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mkdir -p dist/fonts)

    if (APPLE)
        set_target_properties(VoyaMedia PROPERTIES LINK_FLAGS "-Wl,-rpath,../Frameworks -F${OSX_FRAMEWORKS_PATH} -framework AppKit -framework SystemConfiguration")
        add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -af ${LIB_DIR}/*.dylib dist/)
    elseif (UNIX)
        add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -df ${LIB_DIR}/*.so* dist/)
    endif()

    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f  *.tt[cf]      dist/fonts/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -rf ../build/gui  dist/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -rf ../build/img  dist/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -rf ../build/lang dist/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mv -f  VoyaMedia     dist/)
elseif (WIN32)
    set_target_properties(VoyaMedia PROPERTIES LINK_FLAGS_DEBUG   "-SUBSYSTEM:CONSOLE,5.02")
    set_target_properties(VoyaMedia PROPERTIES LINK_FLAGS_RELEASE "-SUBSYSTEM:WINDOWS,5.02")

    string(REPLACE "/" "\\" LIB_DIR_WIN ${LIB_DIR})

    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND if not exist $(OutputPath)fonts mkdir $(OutputPath)fonts)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND copy /y *.ttc                 $(OutputPath)fonts\\)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND copy /y *.ttf                 $(OutputPath)fonts\\)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND xcopy /e /i /y ..\\build\\gui $(OutputPath)gui)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND xcopy /e /i /y ..\\build\\img $(OutputPath)img)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND xcopy /e /i /y ..\\build\\lang $(OutputPath)lang)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND copy /y ${LIB_DIR_WIN}\\*.dll $(OutputPath))
endif()
