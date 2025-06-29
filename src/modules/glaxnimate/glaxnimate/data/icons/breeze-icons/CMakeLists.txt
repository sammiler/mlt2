cmake_minimum_required(VERSION 3.16)

set(KF_VERSION "5.105.0") # handled by release scripts
project(BreezeIcons VERSION ${KF_VERSION})

# Disallow in-source build
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
   message(FATAL_ERROR "Breeze Icons requires an out-of-source build. Please create a separate build directory and run 'cmake path_to_breeze_icons [options]' there.")
endif()

# ECM setup
include(FeatureSummary)
find_package(ECM 5.104.0  NO_MODULE)
set_package_properties(ECM PROPERTIES TYPE REQUIRED DESCRIPTION "Extra CMake Modules." URL "https://commits.kde.org/extra-cmake-modules")
feature_summary(WHAT REQUIRED_PACKAGES_NOT_FOUND FATAL_ON_MISSING_REQUIRED_PACKAGES)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEGitCommitHooks)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(GtkUpdateIconCache)

# Dependencies
set(REQUIRED_QT_VERSION 5.15.2)

# Find Python 3
find_package (Python 3 COMPONENTS Interpreter)

# FIXME: This is a temporary solution to unbreak KDE's Windows CI.
# Remove "NOT WIN32" when icon generation works on Windows.
if(Python_FOUND AND NOT WIN32)
    # Find lxml Python 3 module
    exec_program(${Python_EXECUTABLE}
        ARGS "-c \"import lxml; from lxml import etree; print(lxml.get_include())\""
        RETURN_VALUE LXML_NOT_FOUND # Returns 0 or false if successful
    )

    # Generating icons is not required, but strongly recommended in most cases
    if(LXML_NOT_FOUND)
        message(WARNING "lxml or lxml.etree not found; icon generation disabled")
        set(WITH_ICON_GENERATION OFF)
    else()
        option(WITH_ICON_GENERATION "Icon generation" ON)
    endif()
else()
    set(WITH_ICON_GENERATION OFF)
endif()

add_feature_info("Icon generation" ${WITH_ICON_GENERATION} "for 24x24 icons.
   This feature requires Python 3 and the lxml Python 3 module."
) # The exact amount of indentation used in the line(s) above is intentional

# When cross-compiling, we should compile qrcAlias natively (against a native Qt build)
if (CMAKE_CROSSCOMPILING OR QT_MAJOR_VERSION GREATER 5)
    set(BINARY_ICONS_RESOURCE_OPTION_DEFAULT OFF)
else()
    set(BINARY_ICONS_RESOURCE_OPTION_DEFAULT ON)
endif()

option(BINARY_ICONS_RESOURCE "Install Qt binary resource files containing breeze icons (breeze-icons.rcc, breeze-icons-dark.rcc)" ${BINARY_ICONS_RESOURCE_OPTION_DEFAULT})
option(SKIP_INSTALL_ICONS "Skip installing the icons files" OFF)

if(BINARY_ICONS_RESOURCE)
    find_package(Qt${QT_MAJOR_VERSION} NO_MODULE REQUIRED Core)
    add_executable(qrcAlias qrcAlias.cpp)
    target_link_libraries(qrcAlias PUBLIC Qt${QT_MAJOR_VERSION}::Core)

function(generate_binary_resource target outfile)
    set(RESOURCES_WORKING_DIR ${CMAKE_CURRENT_BINARY_DIR}/res)
    set(RESOURCE_FILE ${RESOURCES_WORKING_DIR}/breeze-${target}.qrc)
    set(BINARY_RESOURCE_FILE ${CMAKE_CURRENT_BINARY_DIR}/breeze-${target}.rcc)

    # Use $<IF:$<BOOL:${MSVC}>,PATH,LD_LIBRARY_PATH> instead of ${pathVarName} once CMake 3.8 is minimum
    if(MSVC)
        set(pathVarName PATH)
    else()
        set(pathVarName LD_LIBRARY_PATH)
    endif()

    get_target_property(QT_RCC_EXECUTABLE Qt${QT_MAJOR_VERSION}::rcc LOCATION)

    add_custom_target(breeze-${target}-mkdir
        COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCES_WORKING_DIR}
    )
    add_custom_command(OUTPUT ${RESOURCE_FILE}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR} ${RESOURCES_WORKING_DIR}
        COMMAND ${CMAKE_COMMAND} -E remove
            ${RESOURCE_FILE}
            ${RESOURCE_FILE}.depends
            ${RESOURCES_WORKING_DIR}/.gitignore
            ${RESOURCES_WORKING_DIR}/CMakeLists.txt
        COMMAND ${QT_RCC_EXECUTABLE} --project -o ${CMAKE_CURRENT_BINARY_DIR}/tmp.qrc
        COMMAND ${CMAKE_COMMAND} -E env
            ${pathVarName}=$<TARGET_FILE_DIR:Qt${QT_MAJOR_VERSION}::Core>
            $<TARGET_FILE:qrcAlias> -i ${CMAKE_CURRENT_BINARY_DIR}/tmp.qrc -o ${RESOURCE_FILE}

        WORKING_DIRECTORY ${RESOURCES_WORKING_DIR}
        DEPENDS breeze-${target}-mkdir
    )
    add_custom_command(OUTPUT ${BINARY_RESOURCE_FILE}
        COMMAND ${QT_RCC_EXECUTABLE} --binary
                    -o ${BINARY_RESOURCE_FILE}
                    ${RESOURCE_FILE}
        DEPENDS ${RESOURCE_FILE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    add_custom_target(breeze-${target}-rcc ALL DEPENDS ${BINARY_RESOURCE_FILE})

    set(${outfile} ${BINARY_RESOURCE_FILE} PARENT_SCOPE)
endfunction()
endif()

if(NOT WIN32 AND NOT CMAKE_CROSSCOMPILING)
    # validate
    find_program(SH_EXE sh)
    find_program(XMLLINT_EXE xmllint)
    if(SH_EXE AND XMLLINT_EXE)
        file(GLOB_RECURSE SVGS "icons/*.svg")
        file(GLOB_RECURSE DARK_SVGS "icons-dark/*.svg")
        # TODO: also make sure this is always run on generated icons
        # not just by accident if the build directory is a subdir of the root source dir
        add_custom_target(breeze-validate-svg ALL
            COMMENT "Validating SVG"
            COMMAND ${SH_EXE} ${CMAKE_SOURCE_DIR}/validate_svg.sh ${CMAKE_CURRENT_SOURCE_DIR}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            SOURCES ${SVGS} ${DARK_SVGS}
        )
    endif()
endif()


add_subdirectory(autotests)
add_subdirectory(icons)
add_subdirectory(icons-dark)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES INCLUDE_QUIET_PACKAGES)

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
