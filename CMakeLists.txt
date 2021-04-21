cmake_minimum_required(VERSION 3.10)

project (VoyaMedia)

# Voya Media depends on the following libraries:
# - curl, ffmpeg, FreeImage, freetype, libxml2, mjson, openssl, sdl2, sdl2_ttf, sqlite, zlib
# You can find the library sources in SOURCEFORGE_URL below:

set(SOURCEFORGE_URL "https://sourceforge.net/projects/voyamedia/files/VoyaMedia/3.x")
set(EXT_LIB_DIR     "__EXT_LIB_DIR__")
set(INC_DIR         "${EXT_LIB_DIR}/include")
set(LIB_DIR         "${EXT_LIB_DIR}/lib")

if (APPLE)
    file(GLOB LIBS_DYLIB "${LIB_DIR}/*.dylib")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch x86_64 -O3 -ObjC++ -std=c++__CPP_VERSION__ -stdlib=libc++ -mmacosx-version-min=__OSX_MIN_VERSION__ -Wno-switch")
    set(LIBS            ${LIBS_DYLIB})

    execute_process(COMMAND xcrun --sdk macosx --show-sdk-path OUTPUT_VARIABLE OSX_SDK_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
	set(OSX_FRAMEWORKS_PATH "${OSX_SDK_PATH}/System/Library/Frameworks")

    add_compile_options(-D_macosx)
elseif (UNIX)
    file(GLOB LIBS_SO "${LIB_DIR}/*.so")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=x86-64 -m64 -O3 -std=c++__CPP_VERSION__ -pthread -Wno-unused-result")

    execute_process(COMMAND pkg-config --cflags gtk+-3.0 OUTPUT_VARIABLE GTK_CFLAGS)
    string(REPLACE "-I" " -I " GTK_CFLAGS2 ${GTK_CFLAGS})
    separate_arguments(GTK UNIX_COMMAND ${GTK_CFLAGS2})

    execute_process(COMMAND pkg-config --libs gtk+-3.0 OUTPUT_VARIABLE GTK_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(LIBS ${LIBS_SO} ${GTK_LIBS})

    add_compile_options(-D_linux)
elseif (WIN32)
    file(GLOB LIBS_LIB RELATIVE ${LIB_DIR} "${LIB_DIR}/*.lib")
    
    set(DIRENT "__DIRENT__")
    set(LIBS   ${LIBS_LIB} IPHLPAPI.lib Ws2_32.lib)
    set(RES    "build/windows/VoyaMedia.rc")

    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT VoyaMedia)

    add_compile_options(-D_windows)
endif()

file(GLOB SOURCES_H   "src/*/*.h")
file(GLOB SOURCES_CPP "src/*/*.cpp")

include_directories(${INC_DIR} ${INC_DIR}/freetype ${INC_DIR}/freetype2 ${INC_DIR}/libxml2 ${INC_DIR}/mjson-1.7 ${DIRENT} ${GTK})
link_directories(${LIB_DIR})

add_executable(VoyaMedia ${SOURCES_CPP} ${SOURCES_H} ${RES})
target_link_libraries(VoyaMedia ${LIBS})

if (NOT EXISTS ./NotoSans-Merged.ttf)
    file(DOWNLOAD ${SOURCEFORGE_URL}/Fonts/NotoSans-Merged.ttf  ./NotoSans-Merged.ttf)
    file(DOWNLOAD ${SOURCEFORGE_URL}/Fonts/NotoSansCJK-Bold.ttc ./NotoSansCJK-Bold.ttc)
endif()

if (APPLE)
    set_target_properties(VoyaMedia PROPERTIES LINK_FLAGS "-Wl,-rpath,../Frameworks -F${OSX_FRAMEWORKS_PATH} -framework AppKit -framework SystemConfiguration")

    set(APP_DIR "VoyaMedia.app")
    set(OUT_DIR "${APP_DIR}/Contents")
    set(BIN_DIR "${OUT_DIR}/MacOS")
    set(FWK_DIR "${OUT_DIR}/Frameworks")
    set(RES_DIR "${OUT_DIR}/Resources")

    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mkdir -p VoyaMedia.app/Contents/Frameworks)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mkdir -p VoyaMedia.app/Contents/MacOS)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mkdir -p VoyaMedia.app/Contents/Resources/docs)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mkdir -p VoyaMedia.app/Contents/Resources/fonts)

    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ../LICENSE                  ${RES_DIR}/docs/LICENSE.txt)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ../README.md                ${RES_DIR}/docs/README.txt)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ${EXT_LIB_DIR}/*.txt        ${RES_DIR}/docs/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f  *.tt[cf]                   ${RES_DIR}/fonts/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -rf ../build/gui               ${RES_DIR}/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -rf ../build/img               ${RES_DIR}/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -rf ../build/lang              ${RES_DIR}/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -af ${LIB_DIR}/*.dylib         ${FWK_DIR}/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mv -f  VoyaMedia                  ${BIN_DIR}/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f  ../build/macosx/Info.plist ${OUT_DIR}/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f  ../build/macosx/PkgInfo    ${OUT_DIR}/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f  ../build/macosx/img/*      ${RES_DIR}/)

    if(CMAKE_BUILD_TYPE STREQUAL Release)
        add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND codesign --entitlements ../build/macosx/Entitlements.plist --deep -i __APP_IDENTITY__ -s "__OSX_SIGN_APP__" -fv ${APP_DIR})
        add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND productbuild --component ${APP_DIR} /Applications --sign "__OSX_SIGN_INSTALLER__" __OSX_PKG__)
    endif()
elseif (UNIX)
    set_target_properties(VoyaMedia PROPERTIES LINK_FLAGS "-Wl,-rpath,.")

    set(OUT_DIR "voyamedia")

    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mkdir -p ${OUT_DIR}/docs)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mkdir -p ${OUT_DIR}/fonts)

    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ../LICENSE            ${OUT_DIR}/docs/LICENSE.txt)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ../README.md          ${OUT_DIR}/docs/README.txt)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ${EXT_LIB_DIR}/*.txt  ${OUT_DIR}/docs/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f  *.tt[cf]             ${OUT_DIR}/fonts/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -rf ../build/gui         ${OUT_DIR}/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -rf ../build/img         ${OUT_DIR}/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -rf ../build/lang        ${OUT_DIR}/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -df ${LIB_DIR}/*.so*     ${OUT_DIR}/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mv -f  VoyaMedia            ${OUT_DIR}/)
elseif (WIN32)
    set_target_properties(VoyaMedia PROPERTIES LINK_FLAGS_DEBUG   "-SUBSYSTEM:CONSOLE,5.02")
    set_target_properties(VoyaMedia PROPERTIES LINK_FLAGS_RELEASE "-SUBSYSTEM:WINDOWS,5.02")

    string(REPLACE "/" "\\" LIB_DIR_WIN     ${LIB_DIR})
    string(REPLACE "/" "\\" EXT_LIB_DIR_WIN ${EXT_LIB_DIR})

    set(OUT_DIR "VoyaMedia")

    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND if not exist $(OutputPath)${OUT_DIR}\\docs  mkdir $(OutputPath)${OUT_DIR}\\docs)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND if not exist $(OutputPath)${OUT_DIR}\\fonts mkdir $(OutputPath)${OUT_DIR}\\fonts)

    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND copy /y ..\\LICENSE               $(OutputPath)${OUT_DIR}\\docs\\LICENSE.txt)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND copy /y ..\\README.md             $(OutputPath)${OUT_DIR}\\docs\\README.txt)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND copy /y ${EXT_LIB_DIR_WIN}\\*.txt $(OutputPath)${OUT_DIR}\\docs\\)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND copy /y *.ttc                     $(OutputPath)${OUT_DIR}\\fonts\\)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND copy /y *.ttf                     $(OutputPath)${OUT_DIR}\\fonts\\)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND xcopy /e /i /y ..\\build\\gui     $(OutputPath)${OUT_DIR}\\gui)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND xcopy /e /i /y ..\\build\\img     $(OutputPath)${OUT_DIR}\\img)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND xcopy /e /i /y ..\\build\\lang    $(OutputPath)${OUT_DIR}\\lang)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND copy /y ${LIB_DIR_WIN}\\*.dll     $(OutputPath)${OUT_DIR}\\)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND move /y $(OutputPath)*.exe        $(OutputPath)${OUT_DIR}\\)

    if(CMAKE_BUILD_TYPE STREQUAL Release)
        add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND makensis.exe ../build/windows/VoyaMedia.nsi)
    endif()
endif()
