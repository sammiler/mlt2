add_library(mltspatialaudio MODULE
    factory.c
    filter_ambisonic-decoder.cpp
    filter_ambisonic-encoder.cpp
)

file(GLOB YML "*.yml")
add_custom_target(Other_spatialaudio_Files SOURCES
  ${YML}
)

target_compile_options(mltspatialaudio PRIVATE ${MLT_COMPILE_OPTIONS})

target_link_libraries(mltspatialaudio PRIVATE mlt PkgConfig::spatialaudio)
target_include_directories(mltspatialaudio PRIVATE
        ${CMAKE_SOURCE_DIR}/external/spatialaudio/include
        ${CMAKE_SOURCE_DIR}/external/spatialaudio/include/hrtf
        ${CMAKE_SOURCE_DIR}/external/spatialaudio/include/normal
        ${CMAKE_SOURCE_DIR}/external/spatialaudio/source/kiss_fft
        ${CMAKE_BINARY_DIR}/external/spatialaudio
)
set_target_properties(mltspatialaudio PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MLT_MODULE_OUTPUT_DIRECTORY}")

install(TARGETS mltspatialaudio LIBRARY DESTINATION ${MLT_INSTALL_MODULE_DIR})

install(FILES
    filter_ambisonic-decoder.yml
    filter_ambisonic-encoder.yml
    DESTINATION ${MLT_INSTALL_DATA_DIR}/spatialaudio
)
