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

    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ../LICENSE                 ${RES_DIR}/docs/LICENSE.txt)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ../README.md               ${RES_DIR}/docs/README.txt)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ${EXT_LIB_DIR}/*.txt       ${RES_DIR}/docs/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f *.tt[cf]                   ${RES_DIR}/fonts/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -rf ../build/gui              ${RES_DIR}/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -rf ../build/img              ${RES_DIR}/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ../build/macosx/img/*      ${RES_DIR}/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -rf ../build/lang             ${RES_DIR}/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -af ${LIB_DIR}/*.dylib        ${FWK_DIR}/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mv -f VoyaMedia                  ${BIN_DIR}/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ../build/macosx/Info.plist ${OUT_DIR}/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ../build/macosx/PkgInfo    ${OUT_DIR}/)

    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND codesign --entitlements ../build/macosx/Entitlements.plist --deep -i __APP_IDENTITY__ -s "__OSX_SIGN_APP__" -fv ${APP_DIR})
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND productbuild --component ${APP_DIR} /Applications --sign "__OSX_SIGN_INSTALLER__" __OSX_PKG__)
elseif (UNIX)
    set_target_properties(VoyaMedia PROPERTIES LINK_FLAGS "-Wl,-rpath,.")

    set(DIST_DIR "__LINUX_DIST__")
    set(OUT_DIR  "voyamedia")
    set(DEB_DIR  "debbuild")
    set(RPM_DIR  "rpmbuild")

    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mkdir -p ${OUT_DIR}/docs)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mkdir -p ${OUT_DIR}/fonts)

    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ../LICENSE               ${OUT_DIR}/docs/LICENSE.txt)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ../README.md             ${OUT_DIR}/docs/README.txt)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ${EXT_LIB_DIR}/*.txt     ${OUT_DIR}/docs/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ../build/linux/copyright ${OUT_DIR}/docs/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f *.tt[cf]                 ${OUT_DIR}/fonts/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -rf ../build/gui            ${OUT_DIR}/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -rf ../build/img            ${OUT_DIR}/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -rf ../build/lang           ${OUT_DIR}/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ../build/linux/img/*     ${OUT_DIR}/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -df ${LIB_DIR}/*.so*        ${OUT_DIR}/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mv -f VoyaMedia                ${OUT_DIR}/)

    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mkdir -p ${DEB_DIR}/${DIST_DIR}/DEBIAN)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mkdir -p ${DEB_DIR}/${DIST_DIR}/opt)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mkdir -p ${DEB_DIR}/${DIST_DIR}/usr/bin)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mkdir -p ${DEB_DIR}/${DIST_DIR}/usr/share/applications)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mkdir -p ${DEB_DIR}/${DIST_DIR}/usr/share/doc/voyamedia)

    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ../build/linux/control   ${DEB_DIR}/${DIST_DIR}/DEBIAN/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -rf ${OUT_DIR}              ${DEB_DIR}/${DIST_DIR}/opt/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ../build/linux/voyamedia ${DEB_DIR}/${DIST_DIR}/usr/bin/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ../build/linux/*.desktop ${DEB_DIR}/${DIST_DIR}/usr/share/applications/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ${OUT_DIR}/docs/*        ${DEB_DIR}/${DIST_DIR}/usr/share/doc/voyamedia/)

    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND dpkg-deb --build ${DEB_DIR}/${DIST_DIR}/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mv -f ${DEB_DIR}/*.deb .)

    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mkdir -p ${RPM_DIR}/BUILD/${DIST_DIR}/opt)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mkdir -p ${RPM_DIR}/BUILD/${DIST_DIR}/usr/bin)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mkdir -p ${RPM_DIR}/BUILD/${DIST_DIR}/usr/share/applications)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mkdir -p ${RPM_DIR}/BUILD/${DIST_DIR}/usr/share/doc/voyamedia)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mkdir -p ${RPM_DIR}/RPMS)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mkdir -p ${RPM_DIR}/SOURCES)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mkdir -p ${RPM_DIR}/SPECS)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mkdir -p ${RPM_DIR}/SRPMS)

    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -rf ${OUT_DIR}              ${RPM_DIR}/BUILD/${DIST_DIR}/opt/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ../build/linux/voyamedia ${RPM_DIR}/BUILD/${DIST_DIR}/usr/bin/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ../build/linux/*.desktop ${RPM_DIR}/BUILD/${DIST_DIR}/usr/share/applications/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ${OUT_DIR}/docs/*        ${RPM_DIR}/BUILD/${DIST_DIR}/usr/share/doc/voyamedia/)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND cp -f ../build/linux/*.spec    ${RPM_DIR}/SPECS/)

    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND rpmlint ${RPM_DIR}/SPECS/voyamedia.spec)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND QA_RPATHS=0x0006 rpmbuild -bb --define "_topdir ${WORKSPACE}/voyamedia/dist/${RPM_DIR}" --target=x86_64 --verbose ${RPM_DIR}/SPECS/voyamedia.spec)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND mv -f ${RPM_DIR}/RPMS/x86_64/*.rpm .)
elseif (WIN32)
    set_target_properties(VoyaMedia PROPERTIES LINK_FLAGS_DEBUG   "-SUBSYSTEM:CONSOLE,5.02")
    set_target_properties(VoyaMedia PROPERTIES LINK_FLAGS_RELEASE "-SUBSYSTEM:WINDOWS,5.02")

    string(REPLACE "/" "\\" LIB_DIR_WIN     ${LIB_DIR})
    string(REPLACE "/" "\\" EXT_LIB_DIR_WIN ${EXT_LIB_DIR})

    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND if not exist $(OutputPath)\\docs  mkdir $(OutputPath)\\docs)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND if not exist $(OutputPath)\\fonts mkdir $(OutputPath)\\fonts)

    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND copy /y ..\\LICENSE               $(OutputPath)docs\\LICENSE.txt)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND copy /y ..\\README.md             $(OutputPath)docs\\README.txt)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND copy /y ${EXT_LIB_DIR_WIN}\\*.txt $(OutputPath)docs\\)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND copy /y *.ttc                     $(OutputPath)fonts\\)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND copy /y *.ttf                     $(OutputPath)fonts\\)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND xcopy /e /i /y ..\\build\\gui     $(OutputPath)gui)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND xcopy /e /i /y ..\\build\\img     $(OutputPath)img)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND xcopy /e /i /y ..\\build\\lang    $(OutputPath)lang)
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND copy /y ${LIB_DIR_WIN}\\*.dll     $(OutputPath))

    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND copy /y $(VC_CppRuntimeFilesPath_x64)\\Microsoft.VC$(PlatformToolsetVersion).CRT\\*.dll    $(OutputPath))
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND copy /y $(VC_CppRuntimeFilesPath_x64)\\Microsoft.VC$(PlatformToolsetVersion).CXXAMP\\*.dll $(OutputPath))
    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND copy /y $(VC_CppRuntimeFilesPath_x64)\\Microsoft.VC$(PlatformToolsetVersion).OpenMP\\*.dll $(OutputPath))

    add_custom_command(TARGET VoyaMedia POST_BUILD COMMAND makensis.exe VoyaMedia.nsi)
endif()
