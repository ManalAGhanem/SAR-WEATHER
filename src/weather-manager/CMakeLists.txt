# Check for standard C++ headers
check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

# Add examples as tests, if enabled
set(examples_as_tests_sources)
if(${ENABLE_EXAMPLES})
    set(examples_as_tests_sources
        #test/weather-manager-examples-test-suite.cc
        )
endif()

# Include the directories containing your header files
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/model)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/helper)

# Build the main weather library with all relevant source and header files
build_lib(
    LIBNAME weather-manager
    SOURCE_FILES model/weather-manager.cc
                 model/weather-attenuation-model.cc
                 #helper/weather-manager-helper.cc
                 #helper/weather-attenuation-model-helper.cc
    HEADER_FILES model/weather-manager.h
                 model/weather-attenuation-model.h
                 #helper/weather-manager-helper.h
                 #helper/weather-attenuation-model-helper.h
    LIBRARIES_TO_LINK ${libcore}
    TEST_SOURCES test/weather-manager-test-suite.cc
                 ${examples_as_tests_sources}
)

# Add the module for use in scratch scripts or other executables
add_library(ns3dev-weather-module STATIC
    model/weather-manager.cc
    model/weather-attenuation-model.cc
    #helper/weather-manager-helper.cc
    #helper/weather-attenuation-model-helper.cc
)

# Link the weather module to the necessary ns-3 core libraries
target_link_libraries(ns3dev-weather-module
    ${libcore}
    ${libmobility}
    ${libpropagation}
    ${libnetwork}
)
