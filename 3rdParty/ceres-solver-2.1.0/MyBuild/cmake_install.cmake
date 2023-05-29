# Install script for directory: D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/Ceres")
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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/Ceres" TYPE FILE FILES
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/cmake/FindSuiteSparse.cmake"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/cmake/FindMETIS.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ceres" TYPE FILE FILES
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/autodiff_cost_function.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/autodiff_first_order_function.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/autodiff_local_parameterization.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/autodiff_manifold.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/c_api.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/ceres.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/conditioned_cost_function.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/context.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/cost_function.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/cost_function_to_functor.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/covariance.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/crs_matrix.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/cubic_interpolation.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/dynamic_autodiff_cost_function.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/dynamic_cost_function.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/dynamic_cost_function_to_functor.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/dynamic_numeric_diff_cost_function.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/evaluation_callback.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/first_order_function.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/gradient_checker.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/gradient_problem.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/gradient_problem_solver.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/iteration_callback.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/jet.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/jet_fwd.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/line_manifold.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/local_parameterization.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/loss_function.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/manifold.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/manifold_test_utils.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/normal_prior.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/numeric_diff_cost_function.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/numeric_diff_first_order_function.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/numeric_diff_options.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/ordered_groups.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/problem.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/product_manifold.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/rotation.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/sized_cost_function.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/solver.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/sphere_manifold.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/tiny_solver.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/tiny_solver_autodiff_function.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/tiny_solver_cost_function_adapter.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/types.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/version.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ceres/internal" TYPE FILE FILES
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/internal/array_selector.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/internal/autodiff.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/internal/disable_warnings.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/internal/eigen.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/internal/fixed_array.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/internal/householder_vector.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/internal/integer_sequence_algorithm.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/internal/jet_traits.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/internal/line_parameterization.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/internal/memory.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/internal/numeric_diff.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/internal/parameter_dims.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/internal/port.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/internal/reenable_warnings.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/internal/sphere_manifold_functions.h"
    "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/include/ceres/internal/variadic_evaluate.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE DIRECTORY FILES "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/MyBuild/include/")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/Ceres/CeresTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/Ceres/CeresTargets.cmake"
         "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/MyBuild/CMakeFiles/Export/9a3bb6344a10c987f9c537d2a0e39364/CeresTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/Ceres/CeresTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/Ceres/CeresTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/Ceres" TYPE FILE FILES "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/MyBuild/CMakeFiles/Export/9a3bb6344a10c987f9c537d2a0e39364/CeresTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/Ceres" TYPE FILE FILES "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/MyBuild/CMakeFiles/Export/9a3bb6344a10c987f9c537d2a0e39364/CeresTargets-debug.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/Ceres" TYPE FILE FILES "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/MyBuild/CMakeFiles/Export/9a3bb6344a10c987f9c537d2a0e39364/CeresTargets-minsizerel.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/Ceres" TYPE FILE FILES "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/MyBuild/CMakeFiles/Export/9a3bb6344a10c987f9c537d2a0e39364/CeresTargets-relwithdebinfo.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/Ceres" TYPE FILE FILES "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/MyBuild/CMakeFiles/Export/9a3bb6344a10c987f9c537d2a0e39364/CeresTargets-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/Ceres" TYPE FILE RENAME "CeresConfig.cmake" FILES "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/MyBuild/CeresConfig-install.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/Ceres" TYPE FILE FILES "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/MyBuild/CeresConfigVersion.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/MyBuild/internal/ceres/cmake_install.cmake")
  include("D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/MyBuild/examples/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "D:/RTPS/RTPSv0.5/3rdParty/ceres-solver-2.1.0/MyBuild/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
