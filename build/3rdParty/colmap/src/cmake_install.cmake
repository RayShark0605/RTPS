# Install script for directory: D:/RTPS/RTPSv0.5/3rdParty/colmap/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/RTPS")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/colmap" TYPE STATIC_LIBRARY FILES "D:/RTPS/RTPSv0.5/build/3rdParty/colmap/src/Debug/colmap.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/colmap" TYPE STATIC_LIBRARY FILES "D:/RTPS/RTPSv0.5/build/3rdParty/colmap/src/Release/colmap.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/colmap" TYPE STATIC_LIBRARY FILES "D:/RTPS/RTPSv0.5/build/3rdParty/colmap/src/Debug/colmap_cuda.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/colmap" TYPE STATIC_LIBRARY FILES "D:/RTPS/RTPSv0.5/build/3rdParty/colmap/src/Release/colmap_cuda.lib")
  endif()
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("D:/RTPS/RTPSv0.5/build/3rdParty/colmap/src/base/cmake_install.cmake")
  include("D:/RTPS/RTPSv0.5/build/3rdParty/colmap/src/controllers/cmake_install.cmake")
  include("D:/RTPS/RTPSv0.5/build/3rdParty/colmap/src/estimators/cmake_install.cmake")
  include("D:/RTPS/RTPSv0.5/build/3rdParty/colmap/src/exe/cmake_install.cmake")
  include("D:/RTPS/RTPSv0.5/build/3rdParty/colmap/src/feature/cmake_install.cmake")
  include("D:/RTPS/RTPSv0.5/build/3rdParty/colmap/src/mvs/cmake_install.cmake")
  include("D:/RTPS/RTPSv0.5/build/3rdParty/colmap/src/optim/cmake_install.cmake")
  include("D:/RTPS/RTPSv0.5/build/3rdParty/colmap/src/retrieval/cmake_install.cmake")
  include("D:/RTPS/RTPSv0.5/build/3rdParty/colmap/src/sfm/cmake_install.cmake")
  include("D:/RTPS/RTPSv0.5/build/3rdParty/colmap/src/tools/cmake_install.cmake")
  include("D:/RTPS/RTPSv0.5/build/3rdParty/colmap/src/util/cmake_install.cmake")
  include("D:/RTPS/RTPSv0.5/build/3rdParty/colmap/src/ui/cmake_install.cmake")

endif()

