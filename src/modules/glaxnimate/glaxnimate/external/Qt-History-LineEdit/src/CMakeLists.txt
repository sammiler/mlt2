set(SOURCES
    history_line_edit.cpp
)

add_library(QtHistoryLineEdit STATIC ${SOURCES})
target_link_libraries(QtHistoryLineEdit PUBLIC Qt${QT_VERSION_MAJOR}::Widgets)

target_include_directories(QtHistoryLineEdit PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
