cmake_minimum_required(VERSION 3.16)

project(FlykiOverlap LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(FLYKI_BENCHMARK_SOURCES
    Benchmarks.cpp
    F1.cpp
    F2.cpp
    F3.cpp
    F4.cpp
    F5.cpp
    F6.cpp
    F7.cpp
    F8.cpp
    F9.cpp
    F10.cpp
    F11.cpp
    F12.cpp
)

set(FLYKI_BENCHMARK_HEADERS
    Benchmarks.h
    F1.h
    F2.h
    F3.h
    F4.h
    F5.h
    F6.h
    F7.h
    F8.h
    F9.h
    F10.h
    F11.h
    F12.h
)

add_library(flyki_core STATIC
    ${FLYKI_BENCHMARK_SOURCES}
    ${FLYKI_BENCHMARK_HEADERS}
    Header.cpp
    Header.h
)
target_include_directories(flyki_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(flyki_core PUBLIC cxx_std_11)

add_library(grouping_core STATIC
    grouping/GroupingData.cpp
    grouping/GroupingData.h
    grouping/GroupingIO.cpp
    grouping/GroupingIO.h
    grouping/GroupingMetrics.cpp
    grouping/GroupingMetrics.h
)
target_include_directories(grouping_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(grouping_core PUBLIC cxx_std_17)

add_executable(grouping_metrics_cli
    grouping/grouping_metrics_cli.cpp
)
target_link_libraries(grouping_metrics_cli PRIVATE grouping_core)
target_compile_features(grouping_metrics_cli PRIVATE cxx_std_17)

add_executable(grouping_metrics_tests
    grouping/GroupingMetricsTests.cpp
)
target_link_libraries(grouping_metrics_tests PRIVATE grouping_core)
target_compile_features(grouping_metrics_tests PRIVATE cxx_std_17)

set(FLYKI_CMAES_DIR "" CACHE PATH "Path to the original CMA-ES C/C++ source distribution.")
set(FLYKI_CMAES_SOURCES "" CACHE STRING "Semicolon-separated CMA-ES implementation sources, if they are not discoverable from FLYKI_CMAES_DIR.")

set(_flyki_cmaes_search_dirs "${CMAKE_CURRENT_SOURCE_DIR}")
if(FLYKI_CMAES_DIR)
    list(APPEND _flyki_cmaes_search_dirs "${FLYKI_CMAES_DIR}")
endif()

find_path(FLYKI_CMAES_INCLUDE_DIR
    NAMES cmaes_interface.h
    PATHS ${_flyki_cmaes_search_dirs}
    PATH_SUFFIXES . include src c-cmaes
    NO_DEFAULT_PATH
)

find_path(FLYKI_BOUNDARY_INCLUDE_DIR
    NAMES boundary_transformation.h
    PATHS ${_flyki_cmaes_search_dirs}
    PATH_SUFFIXES . include src c-cmaes
    NO_DEFAULT_PATH
)

set(FLYKI_MISSING_CMAES_FILES)
if(NOT FLYKI_CMAES_INCLUDE_DIR)
    list(APPEND FLYKI_MISSING_CMAES_FILES "cmaes_interface.h")
endif()
if(NOT FLYKI_BOUNDARY_INCLUDE_DIR)
    list(APPEND FLYKI_MISSING_CMAES_FILES "boundary_transformation.h")
endif()

set(FLYKI_CMAES_SOURCE_FILES)
if(FLYKI_CMAES_SOURCES)
    foreach(_source IN LISTS FLYKI_CMAES_SOURCES)
        if(IS_ABSOLUTE "${_source}")
            set(_resolved_source "${_source}")
        elseif(FLYKI_CMAES_DIR)
            set(_resolved_source "${FLYKI_CMAES_DIR}/${_source}")
        else()
            set(_resolved_source "${CMAKE_CURRENT_SOURCE_DIR}/${_source}")
        endif()

        if(EXISTS "${_resolved_source}")
            list(APPEND FLYKI_CMAES_SOURCE_FILES "${_resolved_source}")
        else()
            list(APPEND FLYKI_MISSING_CMAES_FILES "${_resolved_source}")
        endif()
    endforeach()
elseif(FLYKI_CMAES_DIR)
    foreach(_candidate IN ITEMS cmaes.c cmaes.cpp cmaes_interface.c cmaes_interface.cpp)
        unset(_flyki_cmaes_impl CACHE)
        find_file(_flyki_cmaes_impl
            NAMES ${_candidate}
            PATHS "${FLYKI_CMAES_DIR}"
            PATH_SUFFIXES . src c-cmaes
            NO_DEFAULT_PATH
        )
        if(_flyki_cmaes_impl)
            list(APPEND FLYKI_CMAES_SOURCE_FILES "${_flyki_cmaes_impl}")
            break()
        endif()
    endforeach()

    foreach(_candidate IN ITEMS boundary_transformation.c boundary_transformation.cpp)
        unset(_flyki_boundary_impl CACHE)
        find_file(_flyki_boundary_impl
            NAMES ${_candidate}
            PATHS "${FLYKI_CMAES_DIR}"
            PATH_SUFFIXES . src c-cmaes
            NO_DEFAULT_PATH
        )
        if(_flyki_boundary_impl)
            list(APPEND FLYKI_CMAES_SOURCE_FILES "${_flyki_boundary_impl}")
            break()
        endif()
    endforeach()
endif()

if(NOT FLYKI_CMAES_SOURCE_FILES)
    list(APPEND FLYKI_MISSING_CMAES_FILES "CMA-ES implementation source files")
endif()

if(NOT FLYKI_MISSING_CMAES_FILES)
    foreach(_source IN LISTS FLYKI_CMAES_SOURCE_FILES)
        if(_source MATCHES "\\.c$")
            enable_language(C)
            break()
        endif()
    endforeach()

    add_library(flyki_cmaes STATIC ${FLYKI_CMAES_SOURCE_FILES})
    target_include_directories(flyki_cmaes PUBLIC
        "${FLYKI_CMAES_INCLUDE_DIR}"
        "${FLYKI_BOUNDARY_INCLUDE_DIR}"
    )

    add_executable(cbocco
        CBOCC.cpp
        CBOG_CBD.cpp
        CMAESO.cpp
        ${FLYKI_BENCHMARK_SOURCES}
        ${FLYKI_BENCHMARK_HEADERS}
    )
    target_include_directories(cbocco PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_features(cbocco PRIVATE cxx_std_11)
    target_link_libraries(cbocco PRIVATE flyki_cmaes)
else()
    string(REPLACE ";" ", " FLYKI_MISSING_CMAES_FILES_TEXT "${FLYKI_MISSING_CMAES_FILES}")
    add_custom_target(cbocco
        COMMAND "${CMAKE_COMMAND}"
            -DFLYKI_MISSING_CMAES_FILES="${FLYKI_MISSING_CMAES_FILES_TEXT}"
            -P "${CMAKE_CURRENT_LIST_DIR}/cmake/report_missing_cmaes.cmake"
        COMMENT "Reporting missing external CMA-ES dependency for cbocco"
        VERBATIM
    )
endif()
