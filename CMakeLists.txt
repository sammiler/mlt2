cmake_minimum_required(VERSION 3.14...3.31)

project(MLT
  VERSION 7.33.0
  DESCRIPTION "Multimedia Framework"
  HOMEPAGE_URL "https://www.mltframework.org"
  LANGUAGES C CXX
)
# ====================================================================
# 全局输出目录设置 (强制统一所有目标的输出路径)
# ====================================================================
# 设置所有 Release 类型的库文件的输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin")

# 设置所有 Debug 类型的库文件的输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/debug/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/debug/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/debug/bin")

# 对于不区分配置的生成器（如 Ninja），直接设置通用变量
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/bin") # 改为 bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/bin") # 改为 bin
# ====================================================================
option(GPL "Enable GPLv2 components" ON)
option(GPL3 "Enable GPLv3 components" ON)
option(BUILD_TESTING "Enable tests" OFF)
option(BUILD_DOCS "Enable Doxygen documentation" OFF)
option(CLANG_FORMAT "Enable Clang Format" ON)
option(BUILD_TESTS_WITH_QT6 "Build test against Qt 6" OFF)

option(MOD_AVFORMAT "Enable avformat module" ON)
option(MOD_DECKLINK "Enable DeckLink module" ON)
option(MOD_FREI0R "Enable Frei0r module" ON)
option(MOD_GDK "Enable GDK module" ON)
option(MOD_GLAXNIMATE "Enable Glaxnimate module (Qt5)" OFF)
option(MOD_GLAXNIMATE_QT6 "Enable Glaxnimate module (Qt6)" ON)
option(MOD_JACKRACK "Enable JACK Rack module" ON)
option(USE_LV2 "Enable LV2 features" ON)
option(USE_VST2 "Enable LV2 features" ON)
option(MOD_KDENLIVE "Enable Kdenlive module" ON)
option(MOD_NDI "Enable NDI module" OFF)
option(MOD_NORMALIZE "Enable Normalize module (GPL)" ON)
option(MOD_OLDFILM "Enable Oldfilm module" ON)
option(MOD_OPENCV "Enable OpenCV module" ON)
option(MOD_MOVIT "Enable OpenGL module" ON)
option(MOD_PLUS "Enable Plus module" ON)
option(MOD_PLUSGPL "Enable PlusGPL module (GPL)" ON)
option(MOD_QT "Enable Qt5 module (GPL)" OFF)
option(MOD_QT6 "Enable Qt6 module (GPL)" ON)
option(MOD_RESAMPLE "Enable Resample module (GPL)" ON)
option(MOD_RTAUDIO "Enable RtAudio module" ON)
option(MOD_RUBBERBAND "Enable Rubberband module (GPL)" ON)
option(MOD_SDL1 "Enable SDL1 module" ON)
option(MOD_SDL2 "Enable SDL2 module" ON)
option(MOD_SOX "Enable SoX module" ON)
option(MOD_SPATIALAUDIO "Enable SpatialAudio module" ON)
option(MOD_VIDSTAB "Enable vid.stab module (GPL)" ON)
option(MOD_VORBIS "Enable Vorbis module" ON)
option(MOD_XINE "Enable XINE module (GPL)" ON)
option(MOD_XML "Enable XML module" ON)

option(SWIG_CSHARP "Enable SWIG C# bindings" OFF)
option(SWIG_JAVA "Enable SWIG Java bindings" OFF)
option(SWIG_LUA "Enable SWIG Lua bindings" OFF)
option(SWIG_NODEJS "Enable SWIG Node.js bindings" OFF)
option(SWIG_PERL "Enable SWIG Perl bindings" OFF)
option(SWIG_PHP "Enable SWIG PHP bindings" OFF)
option(SWIG_PYTHON "Enable SWIG Python bindings" OFF)
option(SWIG_RUBY "Enable SWIG Ruby bindings" OFF)
option(SWIG_TCL "Enable SWIG Tcl bindings" OFF)

if(WIN32)
  option(WINDOWS_DEPLOY "Install exes/libs directly to prefix (no subdir /bin)" ON)
else()
  if(APPLE)
    option(RELOCATABLE "Use standard app bundle layout" ON)
  else()
    option(RELOCATABLE "Look for plugins relative to app" OFF)
  endif()
endif()


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(FeatureSummary)
include(GNUInstallDirs)

if(WINDOWS_DEPLOY)
  set(CMAKE_INSTALL_BINDIR .)
endif()
if (MSVC)
  find_package(PThreads4W REQUIRED)
endif ()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/lib")
set(MLT_MODULE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/lib/mlt")
set(MLT_DATA_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/share/mlt")

# https://gitlab.kitware.com/cmake/community/-/wikis/doc/cmake/RPATH-handling
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if(NOT "${isSystemDir}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "")
endif()


if(NOT EXISTS ${MLT_DATA_OUTPUT_DIRECTORY})
  if(WIN32) # symlinks require admin rights on Windows
    file(COPY "${CMAKE_SOURCE_DIR}/src/modules" DESTINATION "${CMAKE_BINARY_DIR}/out/share" FILES_MATCHING REGEX yml|txt)
    file(RENAME "${CMAKE_BINARY_DIR}/out/share/modules" "${MLT_DATA_OUTPUT_DIRECTORY}")
    file(COPY "${CMAKE_SOURCE_DIR}/presets" DESTINATION "${MLT_DATA_OUTPUT_DIRECTORY}")
    file(COPY "${CMAKE_SOURCE_DIR}/profiles" DESTINATION "${MLT_DATA_OUTPUT_DIRECTORY}")
  else()
    file(MAKE_DIRECTORY "${MLT_DATA_OUTPUT_DIRECTORY}")
    file(GLOB MOD_SUBDIRS "${CMAKE_SOURCE_DIR}/src/modules/*")
    foreach(MOD_SUBDIR ${MOD_SUBDIRS})
      file(RELATIVE_PATH MOD_NAME "${CMAKE_SOURCE_DIR}/src/modules" ${MOD_SUBDIR})
      file(CREATE_LINK "${CMAKE_SOURCE_DIR}/src/modules/${MOD_NAME}" "${MLT_DATA_OUTPUT_DIRECTORY}/${MOD_NAME}" SYMBOLIC)
    endforeach()
    file(CREATE_LINK "${CMAKE_SOURCE_DIR}/presets" "${MLT_DATA_OUTPUT_DIRECTORY}/presets" SYMBOLIC)
    file(CREATE_LINK "${CMAKE_SOURCE_DIR}/profiles" "${MLT_DATA_OUTPUT_DIRECTORY}/profiles" SYMBOLIC)
  endif()
endif()

set(MLT_SUBDIR mlt)
if(NOT (WIN32 OR APPLE))
  set(MLT_SUBDIR mlt-${MLT_VERSION_MAJOR})
  set(MLT_SUBDIR mlt-${MLT_VERSION_MAJOR})
endif()
set(MLT_INSTALL_MODULE_DIR ${CMAKE_INSTALL_LIBDIR}/${MLT_SUBDIR})
set(MLT_INSTALL_DATA_DIR ${CMAKE_INSTALL_DATADIR}/${MLT_SUBDIR})


set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if (MSVC)
  # 将 /Zc:C11Atomics 标志附加到所有 C 语言编译器的标志后面
  # 换成你的编译器版本可能认识的实验性标志
  add_compile_options("$<$<COMPILE_LANGUAGE:C>:/experimental:c11atomics>")
#  add_compile_options("/FI${CMAKE_SOURCE_DIR}/src/framework/msvc_posix_compat.h")
  # 为项目中所有的 target 添加 _USE_MATH_DEFINES 定义
  add_compile_definitions(_USE_MATH_DEFINES)
endif()
list(APPEND MLT_COMPILE_OPTIONS "")

# MSVC cl doesn't support GNU inline assembly (but MSVC-compatible clang-cl does)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "i686|x86|x86_64|AMD64")
    set(CPU_MMX ON)
    set(CPU_SSE ON)
    set(CPU_SSE2 ON)
    if(NOT MSVC) # also NOT clang-cl
      list(APPEND MLT_COMPILE_OPTIONS "-mmmx;-msse;-msse2")
    endif()
  endif()
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "i686" OR (WIN32 AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86"))
    set(CPU_X86_32 ON)
  endif()
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set(CPU_X86_64 ON)
  endif()
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  # Treat warnings as errors with some exceptions
  set(GCC_FLAGS "-Wall -Werror -Wno-deprecated-declarations")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_FLAGS} -Wno-class-memaccess -Wno-array-compare -Wno-unused-result -Wno-maybe-uninitialized")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_FLAGS} -Wno-discarded-qualifiers")
  # The following showed up in GCC 14, which I am using via msys2 on Windows.
  # Remove the following when Glaxnimate is fixed.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++20-compat -Wno-dangling-reference")
endif ()

if(MSVC)
  list(APPEND MLT_COMPILE_OPTIONS "$<$<CONFIG:Release>:/fp:fast>")
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  list(APPEND MLT_COMPILE_OPTIONS "$<$<CONFIG:Release>:-ffast-math>")
endif()

if(NOT GPL)
  set(MOD_NORMALIZE OFF)
  set(MOD_PLUSGPL OFF)
  set(MOD_QT OFF)
  set(MOD_QT6 OFF)
  set(MOD_RESAMPLE OFF)
  set(MOD_RUBBERBAND OFF)
  set(MOD_VIDSTAB OFF)
  set(MOD_XINE OFF)
endif()

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

if(WIN32)
  find_package(Iconv REQUIRED)
  if(NOT CMAKE_DL_LIBS)
    find_package(dlfcn-win32 REQUIRED)
    set(CMAKE_DL_LIBS dlfcn-win32::dl)
  endif()
  if(MINGW)
    string(REPLACE "iconv" "pthread" MLT_PTHREAD_LIBS "${Iconv_LIBRARY}")
  endif()
endif()

pkg_check_modules(sdl2 IMPORTED_TARGET sdl2)

if(MOD_QT OR MOD_QT6 OR MOD_PLUS)
  pkg_check_modules(FFTW IMPORTED_TARGET fftw3)
  if(NOT FFTW_FOUND)
    pkg_check_modules(FFTW IMPORTED_TARGET fftw)
  endif()
endif()

if(MOD_QT OR MOD_QT6 OR MOD_GDK)
  pkg_check_modules(libexif IMPORTED_TARGET libexif)
endif()

if(MOD_GDK OR MOD_GTK)
  pkg_check_modules(fontconfig IMPORTED_TARGET fontconfig)
endif()

if(MOD_XML OR MOD_JACKRACK)
  pkg_check_modules(xml IMPORTED_TARGET libxml-2.0)
endif()

if(MOD_AVFORMAT)
  pkg_check_modules(libavformat IMPORTED_TARGET libavformat)
  pkg_check_modules(libswscale IMPORTED_TARGET libswscale)
  pkg_check_modules(libavutil IMPORTED_TARGET libavutil)
  if(TARGET PkgConfig::libavformat AND TARGET PkgConfig::libswscale AND TARGET PkgConfig::libavutil)
    pkg_check_modules(libavcodec IMPORTED_TARGET libavcodec)
    pkg_check_modules(libavfilter IMPORTED_TARGET libavfilter)
    pkg_check_modules(libavdevice IMPORTED_TARGET libavdevice)
    pkg_check_modules(libswresample IMPORTED_TARGET libswresample)
    list(APPEND MLT_SUPPORTED_COMPONENTS avformat)
  else()
    set(MOD_AVFORMAT OFF)
  endif()
endif()

if(MOD_DECKLINK)
  list(APPEND MLT_SUPPORTED_COMPONENTS decklink)
endif()

if(MOD_FREI0R)
  add_subdirectory(external/frei0r)
  set(FREI0R_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/external/frei0r/include)
  set(FREI0R_LIBRARIES frei0r)
  set(FREI0R_FOUND TRUE)
  list(APPEND MLT_SUPPORTED_COMPONENTS frei0r)
endif()

if(MOD_GDK)
  pkg_check_modules(GdkPixbuf IMPORTED_TARGET gdk-pixbuf-2.0)
  if(TARGET PkgConfig::GdkPixbuf)
    pkg_check_modules(pango IMPORTED_TARGET pango)
    pkg_check_modules(pangoft2 IMPORTED_TARGET pangoft2)
    list(APPEND MLT_SUPPORTED_COMPONENTS gdk)
  else()
    set(MOD_GDK OFF)
  endif()
endif()

if(MOD_JACKRACK)
  find_package(JACK)
  pkg_check_modules(glib IMPORTED_TARGET glib-2.0)
  check_include_file(ladspa.h ladspa_h_FOUND)
  if(NOT ladspa_h_FOUND)
    set(USE_LV2 OFF)
    set(USE_VST2 OFF)
  endif()
  list(APPEND MLT_SUPPORTED_COMPONENTS jackrack)
endif()

if(USE_LV2)
  pkg_check_modules(lilv IMPORTED_TARGET lilv-0)
  if(NOT lilv_FOUND)
    set(USE_LV2 OFF)
  endif()
endif()

if(MOD_KDENLIVE)
  list(APPEND MLT_SUPPORTED_COMPONENTS kdenlive)
endif()

if(MOD_NDI)
  find_package(NDI REQUIRED)
  list(APPEND MLT_SUPPORTED_COMPONENTS ndi)
endif()

if(MOD_NORMALIZE)
  list(APPEND MLT_SUPPORTED_COMPONENTS normalize)
endif()

if(MOD_OLDFILM)
  list(APPEND MLT_SUPPORTED_COMPONENTS oldfilm)
endif()

if(MOD_OPENCV)
  find_package(OpenCV REQUIRED COMPONENTS tracking)
  if(OpenCV_tracking_FOUND)
    list(APPEND MLT_SUPPORTED_COMPONENTS opencv)
  else()
    set(MOD_OPENCV OFF)
  endif()
endif()

if(MOD_MOVIT)
  pkg_check_modules(movit IMPORTED_TARGET movit)
  find_package(OpenGL)
  if(TARGET PkgConfig::movit AND TARGET OpenGL::GL)
    if(UNIX AND NOT APPLE)
      find_package(X11 REQUIRED)
    endif()
    list(APPEND MLT_SUPPORTED_COMPONENTS movit)
  else()
    set(MOD_MOVIT OFF)
  endif()
endif()

if(MOD_PLUS)
  pkg_check_modules(libebur128 IMPORTED_TARGET libebur128)
  list(APPEND MLT_SUPPORTED_COMPONENTS plus)
endif()

if(MOD_PLUSGPL)
  list(APPEND MLT_SUPPORTED_COMPONENTS plusgpl)
endif()

# It is necessary to look for Qt6 before Qt5, otherwise there will
# be a conflict with the targets in case both are enabled
if(MOD_QT6)
  find_package(Qt6 COMPONENTS Core Gui Xml SvgWidgets Core5Compat)
  if(Qt6_FOUND)
    list(APPEND MLT_SUPPORTED_COMPONENTS qt6)
  else()
    set(MOD_QT6 OFF)
  endif()
endif()


if(MOD_GLAXNIMATE_QT6)
  find_package(Qt6 COMPONENTS Core Gui Network Widgets Xml)
  if(Qt6_FOUND)
    list(APPEND MLT_SUPPORTED_COMPONENTS glaxnimate-qt6)
  else()
    set(MOD_GLAXNIMATE_QT6 OFF)
  endif()
endif()

if (BUILD_TESTS_WITH_QT6)
    set(QT_MAJOR_VERSION 6)
else()
    set(QT_MAJOR_VERSION 5)
endif()

if(BUILD_TESTING)
  find_package(Qt${QT_MAJOR_VERSION} REQUIRED COMPONENTS Core Test)
  find_package(Kwalify REQUIRED)
  enable_testing()
endif()

if(MOD_QT)
  find_package(Qt5 COMPONENTS Core Xml Gui Svg Widgets)
  if(Qt5_FOUND)
    list(APPEND MLT_SUPPORTED_COMPONENTS qt)
  else()
    set(MOD_QT OFF)
  endif()
endif()

if(MOD_GLAXNIMATE)
  find_package(Qt5 COMPONENTS Core Gui Network Widgets Xml)
  if(Qt5_FOUND)
    list(APPEND MLT_SUPPORTED_COMPONENTS glaxnimate)
  else()
    set(MOD_GLAXNIMATE OFF)
  endif()
endif()

if(MOD_RESAMPLE)
  pkg_check_modules(samplerate IMPORTED_TARGET samplerate)
  if(TARGET PkgConfig::samplerate)
    list(APPEND MLT_SUPPORTED_COMPONENTS resample)
  else()
    set(MOD_RESAMPLE OFF)
  endif()
endif()

if(MOD_RTAUDIO)
  pkg_check_modules(rtaudio IMPORTED_TARGET rtaudio)
  if(NOT TARGET PkgConfig::rtaudio AND UNIX AND NOT APPLE)
    pkg_check_modules(alsa IMPORTED_TARGET alsa)
    pkg_check_modules(libpulse-simple IMPORTED_TARGET libpulse-simple)
  endif()
  list(APPEND MLT_SUPPORTED_COMPONENTS rtaudio)
endif()

if(MOD_RUBBERBAND)
  pkg_check_modules(rubberband IMPORTED_TARGET rubberband)
  if(TARGET PkgConfig::rubberband)
    list(APPEND MLT_SUPPORTED_COMPONENTS rubberband)
  else()
    set(MOD_RUBBERBAND OFF)
  endif()
endif()

# --- 修改后的代码 (推荐方案) ---
if(MOD_SDL1)
  # 保持原来的尝试，以兼容 Linux 等环境
  pkg_check_modules(sdl IMPORTED_TARGET sdl)

  # 如果 pkg-config 失败了，我们启动 B 计划
  if(NOT TARGET PkgConfig::sdl)
    message(STATUS "pkg-config failed for SDL1, trying find_package...")
    # 使用 find_package 查找 SDL 1.2 或更高版本
    # QUIETLY: 查找成功或失败时，不要在控制台打印标准信息
    # 我们用自己的 message() 来控制输出
    find_package(SDL 1.2 )

    # find_package 会定义 SDL_FOUND, SDL_INCLUDE_DIRS, SDL_LIBRARIES
    if(SDL_FOUND)
      message(STATUS "Found SDL1 (via find_package): ${SDL_LIBRARIES}")
      # 这里是最关键的一步：手动创建它期望的 IMPORTED Target
      # 这样后面的代码逻辑就完全不需要改动了
      add_library(PkgConfig::sdl INTERFACE IMPORTED)
      target_include_directories(PkgConfig::sdl INTERFACE ${SDL_INCLUDE_DIRS})
      target_link_libraries(PkgConfig::sdl INTERFACE ${SDL_LIBRARIES})
    endif()
  endif()

  # 现在，再次检查 Target 是否存在 (无论是 pkg-config 创建的还是我们手动创建的)
  if(TARGET PkgConfig::sdl)
    list(APPEND MLT_SUPPORTED_COMPONENTS sdl)
  else()
    message(STATUS "Could not find SDL1. Disabling SDL1 module.")
    set(MOD_SDL1 OFF)
  endif()
endif()

if(MOD_SDL2)
  if(TARGET PkgConfig::sdl2)
    list(APPEND MLT_SUPPORTED_COMPONENTS sdl2)
  else()
    set(MOD_SDL2 OFF)
  endif()
endif()

if(MOD_SOX)
  pkg_check_modules(sox IMPORTED_TARGET sox)
  if(TARGET PkgConfig::sox)
    list(APPEND MLT_SUPPORTED_COMPONENTS sox)
  else()
    set(MOD_SOX OFF)
  endif()
endif()

if(MOD_SPATIALAUDIO)
  # 检查子模块目录是否存在
  if(NOT IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/external/spatialaudio)
    message(FATAL_ERROR "Spatialaudio module is enabled, but the directory 'external/spatialaudio' does not exist. Please add it as a git submodule from https://github.com/pippin/libspatialaudio.git")
  endif()

  message(STATUS "Spatialaudio module enabled. Building from submodule...")
  set(BUILD_STATIC_LIBS OFF CACHE BOOL "Disable building static spatialaudio library to avoid conflict")
  # 1. 将 libspatialaudio 作为子项目进行编译。
  #    这会创建一个普通的库 Target，我们假设它的名字就叫 `spatialaudio`。
  #    (你需要检查 libspatialaudio 的 CMakeLists.txt 来确认它创建的库 Target 的确切名称)
  add_subdirectory(external/spatialaudio)

  # 2. 【魔法步骤】在这里，我们手动创建一个 MLT 所期望的 IMPORTED 目标。
  #    这个新目标只是一个“别名”或“代理”，指向我们刚刚通过 add_subdirectory 编译好的真实目标。
  if(TARGET spatialaudio-shared)
    # 创建一个名为 PkgConfig::spatialaudio 的 INTERFACE IMPORTED 目标
    add_library(PkgConfig::spatialaudio INTERFACE IMPORTED)

    # 将真实 target 'spatialaudio' 的所有公开属性（头文件、链接库等）
    # 都传递给这个新的代理 target。
    # target_link_libraries 的这个用法可以直接传递依赖关系。
    target_link_libraries(PkgConfig::spatialaudio INTERFACE spatialaudio-shared)

    message(STATUS "Bridged 'spatialaudio' target to 'PkgConfig::spatialaudio' for MLT compatibility.")
  else()
    message(WARNING "Could not find the 'spatialaudio' target after add_subdirectory. The module might fail to link.")
  endif()


  # 3. 将 'spatialaudio' 标记为受支持的组件。
  #    因为我们已经创建了 PkgConfig::spatialaudio，后续的链接逻辑现在可以找到它了。
  list(APPEND MLT_SUPPORTED_COMPONENTS spatialaudio)
endif()

if(MOD_VIDSTAB)
  # 首先，尝试使用 pkg-config，以保持与 Linux 等环境的兼容性
  pkg_check_modules(vidstab IMPORTED_TARGET vidstab)

  # 如果 pkg-config 失败了 (在 Windows 上几乎总是如此)，我们启动 B 计划
  if(NOT TARGET PkgConfig::vidstab)
    message(STATUS "pkg-config failed for vidstab, trying to build from submodule...")

    # 检查子模块目录是否存在
    if(NOT IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/external/vid.stab)
      message(FATAL_ERROR "VidStab module is enabled, but the directory 'external/vid.stab' does not exist. Please add it as a git submodule from https://github.com/georgmartius/vid.stab.git")
    endif()

    # 1. 将 vid.stab 作为子项目进行编译。
    #    根据 vid.stab 的 CMakeLists.txt，它创建的库 Target 就叫 `vidstab`。
    add_subdirectory(external/vid.stab)

    # 2. 【桥接魔法】
    #    检查真实的 Target 是否被成功创建，然后为它创建代理。
    if(TARGET vidstab)
      # 创建 MLT 期望的 IMPORTED 目标
      add_library(PkgConfig::vidstab INTERFACE IMPORTED)

      # 将真实 target 'vidstab' 的所有公开属性（头文件、链接库等）
      # 都传递给这个新的代理 target。
      target_link_libraries(PkgConfig::vidstab INTERFACE vidstab)

      message(STATUS "Bridged 'vidstab' target to 'PkgConfig::vidstab' for MLT compatibility.")
    else()
      message(WARNING "Could not find the 'vidstab' target after add_subdirectory. The module might fail to link.")
    endif()
  endif()

  # 现在，再次检查 PkgConfig::vidstab 目标是否存在 (无论是 pkg-config 创建的还是我们手动创建的)
  if(TARGET PkgConfig::vidstab)
    list(APPEND MLT_SUPPORTED_COMPONENTS vidstab)
  else()
    message(STATUS "Could not find vidstab. Disabling vidstab module.")
    set(MOD_VIDSTAB OFF)
  endif()
endif()

if(MOD_VORBIS)
  pkg_check_modules(vorbis IMPORTED_TARGET vorbis)
  pkg_check_modules(vorbisfile IMPORTED_TARGET vorbisfile)
  if(TARGET PkgConfig::vorbis AND TARGET PkgConfig::vorbisfile)
    list(APPEND MLT_SUPPORTED_COMPONENTS vorbis)
  else()
    set(MOD_VORBIS OFF)
  endif()
endif()

if(MOD_XINE)
  list(APPEND MLT_SUPPORTED_COMPONENTS xine)
endif()

if(MOD_XML)
  if(TARGET PkgConfig::xml)
    list(APPEND MLT_SUPPORTED_COMPONENTS xml)
  else()
    set(MOD_XML OFF)
  endif()
endif()

find_package(SWIG)

if(SWIG_CSHARP)
  find_package(Mono REQUIRED)
endif()

if(SWIG_JAVA)
  find_package(JNI REQUIRED)
endif()

if(SWIG_LUA)
  find_package(Lua REQUIRED)
endif()

if(SWIG_NODEJS)
  find_package(Node REQUIRED)
  if(NODE_VERSION_MAJOR VERSION_GREATER_EQUAL 12 AND SWIG_VERSION VERSION_LESS 4.1)
    # https://github.com/swig/swig/issues/1520
    set(SWIG_NODEJS OFF)
  endif()
endif()

if(SWIG_PERL)
  find_package(PerlLibs REQUIRED)
endif()

if(SWIG_PHP)
  find_package(PHP REQUIRED)
endif()

if(SWIG_PYTHON)
  find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
endif()

if(SWIG_RUBY)
  find_package(Ruby REQUIRED)
endif()

if(SWIG_TCL)
  find_package(TCL REQUIRED)
endif()

if(BUILD_DOCS)
  find_package(Doxygen REQUIRED)

  set(DOXYGEN_OUTPUT_DIRECTORY "docs")
  set(DOXYGEN_ABBREVIATE_BRIEF "")
  set(DOXYGEN_STRIP_FROM_PATH "src/framework/")
  set(DOXYGEN_JAVADOC_AUTOBRIEF "YES")
  set(DOXYGEN_QT_AUTOBRIEF "YES")
  set(DOXYGEN_ALIASES "properties=\\xrefitem properties \\\"Property\\\" \\\"Properties Dictionary\\\"")
  list(APPEND DOXYGEN_ALIASES "event=\\xrefitem event \\\"Event\\\" \\\"Events Dictionary\\\"")
  list(APPEND DOXYGEN_ALIASES "envvar=\\xrefitem envvars \\\"Environment Variable\\\" \\\"Environment Variables\\\"")
  set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C "YES")
  set(DOXYGEN_SORT_BRIEF_DOCS "YES")
  set(DOXYGEN_EXTRACT_ALL "YES")
  set(DOXYGEN_EXTRACT_PRIVATE "YES")
  set(DOXYGEN_EXTRACT_STATIC "YES")
  set(DOXYGEN_FILE_PATTERNS "")
  set(DOXYGEN_RECURSIVE "NO")
  set(DOXYGEN_EXCLUDE_PATTERNS "")
  set(DOXYGEN_EXAMPLE_PATTERNS "")
  set(DOXYGEN_SEARCHENGINE "NO")

  doxygen_add_docs(docs src/framework)
endif()

if(CLANG_FORMAT)
  # Formatting may change with different versions of clang-format.
  # Test new versions before changing the allowed version here to avoid
  # accidental broad changes to formatting.
  find_package(ClangFormat 14 EXACT)
  if(CLANGFORMAT_FOUND)
    file(GLOB_RECURSE FORMAT_FILES "src/*.h" "src/*.c" "src/*.cpp")
    # exclude 3rd party source from format checking
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/glaxnimate/glaxnimate/")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*autogen")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/avformat/mmx.h")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/decklink/darwin")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/decklink/linux")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/decklink/win")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/gdk/pixops")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/jackrack/jack_rack.*")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/jackrack/lock_free_fifo.*")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/plugin_desc.*")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/plugin_mgr.*")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/plugin_settings.*")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/plugin.*")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/process.*")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/plus/ebur128")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/movit/optional_effect.h")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/opencv/filter_opencv_tracker.cpp")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/plusgpl/cJSON")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/plusgpl/image.*")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/plusgpl/utils.*")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/rtaudio/RtAudio.*")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/xine/attributes.h")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/xine/cpu_accel.c")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/xine/deinterlace.*")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/xine/vf_yadif_template.h")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/xine/xineutils.h")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/xine/yadif.*")
    list(FILTER FORMAT_FILES EXCLUDE REGEX "/.*/win32")
    add_custom_target(clang-format COMMAND
      ${CLANGFORMAT_EXECUTABLE} -style=file -i ${FORMAT_FILES}
    )
    add_custom_target(clang-format-check COMMAND
      ${CLANGFORMAT_EXECUTABLE} --dry-run --Werror -style=file -i ${FORMAT_FILES}
    )
  else()
    set(CLANG_FORMAT OFF)
  endif()
endif()

install(DIRECTORY presets profiles DESTINATION ${MLT_INSTALL_DATA_DIR})

if(UNIX AND NOT APPLE)
  install(FILES docs/melt.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1 RENAME melt-${MLT_VERSION_MAJOR}.1)
  install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink melt-${MLT_VERSION_MAJOR}.1 melt.1 \
                                WORKING_DIRECTORY ${CMAKE_INSTALL_FULL_MANDIR}/man1)"
  )
endif()

add_subdirectory(src)

install(EXPORT MltTargets
  FILE Mlt${MLT_VERSION_MAJOR}Targets.cmake
  NAMESPACE Mlt${MLT_VERSION_MAJOR}::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Mlt${MLT_VERSION_MAJOR}
)

include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/MltConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/Mlt${MLT_VERSION_MAJOR}Config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Mlt${MLT_VERSION_MAJOR}
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/Mlt${MLT_VERSION_MAJOR}ConfigVersion.cmake"
  COMPATIBILITY SameMajorVersion
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/Mlt${MLT_VERSION_MAJOR}Config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/Mlt${MLT_VERSION_MAJOR}ConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Mlt${MLT_VERSION_MAJOR}
)

add_feature_info("GPLv2" GPL "")
add_feature_info("GPLv3" GPL3 "")
add_feature_info("Tests" BUILD_TESTING "")
add_feature_info("Doxygen" BUILD_DOCS "")
add_feature_info("Clang Format" CLANG_FORMAT "")
add_feature_info("Module: avformat" MOD_AVFORMAT "")
add_feature_info("Module: DeckLink" MOD_DECKLINK "")
add_feature_info("Module: Frei0r" MOD_FREI0R "")
add_feature_info("Module: GDK" MOD_GDK "")
add_feature_info("Module: Glaxnimate (Qt5)" MOD_GLAXNIMATE "")
add_feature_info("Module: Glaxnimate (Qt6)" MOD_GLAXNIMATE_QT6 "")
add_feature_info("Module: JACKRack" MOD_JACKRACK "")
add_feature_info("Module: Kdenlive" MOD_KDENLIVE "")
add_feature_info("Module: NDI" MOD_NDI "")
add_feature_info("Module: Normalize" MOD_NORMALIZE "")
add_feature_info("Module: Oldfilm" MOD_OLDFILM "")
add_feature_info("Module: OpenCV" MOD_OPENCV "")
add_feature_info("Module: Movit" MOD_MOVIT "")
add_feature_info("Module: Plus" MOD_PLUS "")
add_feature_info("Module: PlusGPL" MOD_PLUSGPL "")
add_feature_info("Module: Qt (Qt5)" MOD_QT "")
add_feature_info("Module: Qt6" MOD_QT6 "")
add_feature_info("Module: Resample" MOD_RESAMPLE "")
add_feature_info("Module: RtAudio" MOD_RTAUDIO "")
add_feature_info("Module: Rubberband" MOD_RUBBERBAND "")
add_feature_info("Module: SDL1" MOD_SDL1 "")
add_feature_info("Module: SDL2" MOD_SDL2 "")
add_feature_info("Module: SoX" MOD_SOX "")
add_feature_info("Module: SpatialAudio" MOD_SPATIALAUDIO "")
add_feature_info("Module: vid.stab" MOD_VIDSTAB "")
add_feature_info("Module: Vorbis" MOD_VORBIS "")
add_feature_info("Module: XINE" MOD_XINE "")
add_feature_info("Module: XML" MOD_XML "")
add_feature_info("SWIG: C#" SWIG_CSHARP "")
add_feature_info("SWIG: Java" SWIG_JAVA "")
add_feature_info("SWIG: Lua" SWIG_LUA "")
add_feature_info("SWIG: Node.js" SWIG_NODEJS "")
add_feature_info("SWIG: Perl" SWIG_PERL "")
add_feature_info("SWIG: PHP" SWIG_PHP "")
add_feature_info("SWIG: Python" SWIG_PYTHON "")
add_feature_info("SWIG: Ruby" SWIG_RUBY "")
add_feature_info("SWIG: Tcl" SWIG_TCL "")
add_feature_info("lv2: LV2 Plugins support" USE_LV2 "")
add_feature_info("vst2: VST2 Plugins support" USE_VST2 "")

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
