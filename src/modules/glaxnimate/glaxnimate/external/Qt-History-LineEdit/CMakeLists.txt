#
# Copyright (C) 2012-2020 Mattia Basaglia
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#


cmake_minimum_required (VERSION 3.1 FATAL_ERROR)

project(QtHistoryLineEdit CXX)

# C++
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Qt
set(QT_VERSION_MAJOR 5)
find_package(QT NAMES Qt5 Qt6 COMPONENTS Core QUIET)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Widgets REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Targets
add_subdirectory(src)
