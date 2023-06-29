# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# Set rainmaker path
if(DEFINED ENV{RMAKER_PATH})
  set(RMAKER_PATH $ENV{RMAKER_PATH})
else()
  set(RMAKER_PATH $ENV{HOME}/esp/esp-rainmaker/)
endif(DEFINED ENV{RMAKER_PATH})

# Set rainmaker extra components path
set(EXTRA_COMPONENT_DIRS ${RMAKER_PATH}/components/esp-insights/components ${RMAKER_PATH}/components ${RMAKER_PATH}/examples/common)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ase-project)
