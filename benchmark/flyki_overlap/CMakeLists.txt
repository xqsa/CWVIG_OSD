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

add_library(probe_core STATIC
    probe/BenchmarkEvaluator.cpp
    probe/BenchmarkEvaluator.h
    probe/CWVIGEdgeData.cpp
    probe/CWVIGEdgeData.h
    probe/CWVIGEdgeMetrics.cpp
    probe/CWVIGEdgeMetrics.h
    probe/CWVIGEstimator.cpp
    probe/CWVIGEstimator.h
    probe/FiniteDifferenceProbe.cpp
    probe/FiniteDifferenceProbe.h
    probe/InteractionGroundTruth.cpp
    probe/InteractionGroundTruth.h
    probe/SyntheticFunctions.cpp
    probe/SyntheticFunctions.h
)
target_include_directories(probe_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(probe_core PUBLIC flyki_core)
target_compile_features(probe_core PUBLIC cxx_std_17)

add_library(grouping_core STATIC
    grouping/CBOCCCommandLine.cpp
    grouping/CBOCCCommandLine.h
    grouping/CBOCCGroupingLoader.cpp
    grouping/CBOCCGroupingLoader.h
    grouping/GroupingData.cpp
    grouping/GroupingData.h
    grouping/GroupingCoverageAudit.cpp
    grouping/GroupingCoverageAudit.h
    grouping/GroupingIO.cpp
    grouping/GroupingIO.h
    grouping/HardOverlapCompatibilityAudit.cpp
    grouping/HardOverlapCompatibilityAudit.h
    grouping/GroupingMetrics.cpp
    grouping/GroupingMetrics.h
    grouping/LegacyGroupingAdapter.cpp
    grouping/LegacyGroupingAdapter.h
)
target_include_directories(grouping_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(grouping_core PUBLIC cxx_std_17)

add_library(soft_overlap_core STATIC
    grouping/CWVIGGroupingPipeline.cpp
    grouping/CWVIGGroupingPipeline.h
    grouping/SoftOverlapCalibration.cpp
    grouping/SoftOverlapCalibration.h
    grouping/SoftOverlapDecomposition.cpp
    grouping/SoftOverlapDecomposition.h
    grouping/WeightedInteractionGraph.cpp
    grouping/WeightedInteractionGraph.h
)
target_include_directories(soft_overlap_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(soft_overlap_core PUBLIC grouping_core probe_core)
target_compile_features(soft_overlap_core PUBLIC cxx_std_17)

add_executable(grouping_metrics_cli
    grouping/grouping_metrics_cli.cpp
)
target_link_libraries(grouping_metrics_cli PRIVATE grouping_core)
target_compile_features(grouping_metrics_cli PRIVATE cxx_std_17)

add_executable(grouping_provider_cli
    grouping/grouping_provider_cli.cpp
)
target_link_libraries(grouping_provider_cli PRIVATE grouping_core)
target_compile_features(grouping_provider_cli PRIVATE cxx_std_17)

add_executable(cbocco_grouping_loader_cli
    grouping/cbocco_grouping_loader_cli.cpp
)
target_link_libraries(cbocco_grouping_loader_cli PRIVATE grouping_core)
target_compile_features(cbocco_grouping_loader_cli PRIVATE cxx_std_17)

add_executable(grouping_source_cli
    grouping/grouping_source_cli.cpp
)
target_link_libraries(grouping_source_cli PRIVATE grouping_core)
target_compile_features(grouping_source_cli PRIVATE cxx_std_17)

add_executable(grouping_coverage_cli
    grouping/grouping_coverage_cli.cpp
)
target_link_libraries(grouping_coverage_cli PRIVATE grouping_core)
target_compile_features(grouping_coverage_cli PRIVATE cxx_std_17)

add_executable(hard_overlap_audit_cli
    grouping/hard_overlap_audit_cli.cpp
)
target_link_libraries(hard_overlap_audit_cli PRIVATE grouping_core)
target_compile_features(hard_overlap_audit_cli PRIVATE cxx_std_17)

add_executable(hard_overlap_sanitize_cli
    grouping/hard_overlap_sanitize_cli.cpp
)
target_link_libraries(hard_overlap_sanitize_cli PRIVATE grouping_core)
target_compile_features(hard_overlap_sanitize_cli PRIVATE cxx_std_17)

add_executable(probe_smoke_cli
    probe/probe_smoke_cli.cpp
)
target_link_libraries(probe_smoke_cli PRIVATE probe_core)
target_compile_features(probe_smoke_cli PRIVATE cxx_std_17)

add_executable(cwvig_estimator_cli
    probe/cwvig_estimator_cli.cpp
)
target_link_libraries(cwvig_estimator_cli PRIVATE probe_core)
target_compile_features(cwvig_estimator_cli PRIVATE cxx_std_17)

add_executable(cwvig_edge_eval_cli
    probe/cwvig_edge_eval_cli.cpp
)
target_link_libraries(cwvig_edge_eval_cli PRIVATE probe_core grouping_core)
target_compile_features(cwvig_edge_eval_cli PRIVATE cxx_std_17)

add_executable(soft_overlap_cli
    grouping/soft_overlap_cli.cpp
)
target_link_libraries(soft_overlap_cli PRIVATE soft_overlap_core)
target_compile_features(soft_overlap_cli PRIVATE cxx_std_17)

add_executable(soft_overlap_calibration_cli
    grouping/soft_overlap_calibration_cli.cpp
)
target_link_libraries(soft_overlap_calibration_cli PRIVATE soft_overlap_core)
target_compile_features(soft_overlap_calibration_cli PRIVATE cxx_std_17)

add_executable(cwvig_grouping_pipeline_cli
    grouping/cwvig_grouping_pipeline_cli.cpp
)
target_link_libraries(cwvig_grouping_pipeline_cli PRIVATE soft_overlap_core)
target_compile_features(cwvig_grouping_pipeline_cli PRIVATE cxx_std_17)

add_executable(grouping_metrics_tests
    grouping/GroupingMetricsTests.cpp
)
target_link_libraries(grouping_metrics_tests PRIVATE grouping_core)
target_compile_features(grouping_metrics_tests PRIVATE cxx_std_17)

add_executable(grouping_provider_tests
    grouping/GroupingProviderTests.cpp
)
target_link_libraries(grouping_provider_tests PRIVATE grouping_core)
target_compile_features(grouping_provider_tests PRIVATE cxx_std_17)

add_executable(cbocco_grouping_loader_tests
    grouping/CBOCCGroupingLoaderTests.cpp
)
target_link_libraries(cbocco_grouping_loader_tests PRIVATE grouping_core)
target_compile_features(cbocco_grouping_loader_tests PRIVATE cxx_std_17)

add_executable(grouping_source_tests
    grouping/GroupingSourceTests.cpp
)
target_link_libraries(grouping_source_tests PRIVATE grouping_core)
target_compile_features(grouping_source_tests PRIVATE cxx_std_17)

add_executable(grouping_coverage_tests
    grouping/GroupingCoverageTests.cpp
)
target_link_libraries(grouping_coverage_tests PRIVATE grouping_core)
target_compile_features(grouping_coverage_tests PRIVATE cxx_std_17)

add_executable(hard_overlap_compatibility_tests
    grouping/HardOverlapCompatibilityTests.cpp
)
target_link_libraries(hard_overlap_compatibility_tests PRIVATE grouping_core)
target_compile_features(hard_overlap_compatibility_tests PRIVATE cxx_std_17)

add_executable(cbocco_argument_tests
    grouping/CBOCCCommandLineTests.cpp
)
target_link_libraries(cbocco_argument_tests PRIVATE grouping_core)
target_compile_features(cbocco_argument_tests PRIVATE cxx_std_17)

add_executable(probe_harness_tests
    probe/ProbeHarnessTests.cpp
)
target_link_libraries(probe_harness_tests PRIVATE probe_core)
target_compile_features(probe_harness_tests PRIVATE cxx_std_17)

add_executable(cwvig_estimator_tests
    probe/CWVIGEstimatorTests.cpp
)
target_link_libraries(cwvig_estimator_tests PRIVATE probe_core)
target_compile_features(cwvig_estimator_tests PRIVATE cxx_std_17)

add_executable(cwvig_edge_eval_tests
    probe/CWVIGEdgeEvalTests.cpp
)
target_link_libraries(cwvig_edge_eval_tests PRIVATE probe_core grouping_core)
target_compile_features(cwvig_edge_eval_tests PRIVATE cxx_std_17)

add_executable(soft_overlap_tests
    grouping/SoftOverlapDecompositionTests.cpp
)
target_link_libraries(soft_overlap_tests PRIVATE soft_overlap_core)
target_compile_features(soft_overlap_tests PRIVATE cxx_std_17)

add_executable(soft_overlap_calibration_tests
    grouping/SoftOverlapCalibrationTests.cpp
)
target_link_libraries(soft_overlap_calibration_tests PRIVATE soft_overlap_core)
target_compile_features(soft_overlap_calibration_tests PRIVATE cxx_std_17)

add_executable(soft_overlap_pruning_tests
    grouping/SoftOverlapPruningTests.cpp
)
target_link_libraries(soft_overlap_pruning_tests PRIVATE soft_overlap_core)
target_compile_features(soft_overlap_pruning_tests PRIVATE cxx_std_17)

add_executable(cwvig_grouping_pipeline_tests
    grouping/CWVIGGroupingPipelineTests.cpp
)
target_link_libraries(cwvig_grouping_pipeline_tests PRIVATE soft_overlap_core)
target_compile_features(cwvig_grouping_pipeline_tests PRIVATE cxx_std_17)

get_filename_component(FLYKI_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(CMAES_ROOT "${FLYKI_REPO_ROOT}/third_party/cmaes" CACHE PATH "Path to the original CMA-ES C/C++ dependency root.")
set(CMAES_SOURCES "" CACHE STRING "Semicolon-separated CMA-ES implementation sources, relative to CMAES_ROOT unless absolute.")
set(FLYKI_CMAES_DIR "" CACHE PATH "Deprecated alias for CMAES_ROOT.")
set(FLYKI_CMAES_SOURCES "" CACHE STRING "Deprecated alias for CMAES_SOURCES.")

set(_flyki_cmaes_search_dirs "${CMAKE_CURRENT_SOURCE_DIR}")
if(CMAES_ROOT)
    list(APPEND _flyki_cmaes_search_dirs "${CMAES_ROOT}")
endif()
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
if(CMAES_SOURCES)
    set(_flyki_explicit_cmaes_sources ${CMAES_SOURCES})
elseif(FLYKI_CMAES_SOURCES)
    set(_flyki_explicit_cmaes_sources ${FLYKI_CMAES_SOURCES})
else()
    set(_flyki_explicit_cmaes_sources)
endif()

if(_flyki_explicit_cmaes_sources)
    foreach(_source IN LISTS _flyki_explicit_cmaes_sources)
        if(IS_ABSOLUTE "${_source}")
            set(_resolved_source "${_source}")
        elseif(CMAES_ROOT)
            set(_resolved_source "${CMAES_ROOT}/${_source}")
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
else()
    foreach(_candidate IN ITEMS cmaes.c cmaes.cpp cmaes_interface.c cmaes_interface.cpp)
        unset(_flyki_cmaes_impl CACHE)
        find_file(_flyki_cmaes_impl
            NAMES ${_candidate}
            PATHS ${_flyki_cmaes_search_dirs}
            PATH_SUFFIXES . src c-cmaes
            NO_DEFAULT_PATH
        )
        if(_flyki_cmaes_impl)
            list(APPEND FLYKI_CMAES_SOURCE_FILES "${_flyki_cmaes_impl}")
            break()
        endif()
    endforeach()
    if(NOT _flyki_cmaes_impl)
        list(APPEND FLYKI_MISSING_CMAES_FILES "one of: src/cmaes.c, src/cmaes.cpp, src/cmaes_interface.c, src/cmaes_interface.cpp")
    endif()

    foreach(_candidate IN ITEMS boundary_transformation.c boundary_transformation.cpp)
        unset(_flyki_boundary_impl CACHE)
        find_file(_flyki_boundary_impl
            NAMES ${_candidate}
            PATHS ${_flyki_cmaes_search_dirs}
            PATH_SUFFIXES . src c-cmaes
            NO_DEFAULT_PATH
        )
        if(_flyki_boundary_impl)
            list(APPEND FLYKI_CMAES_SOURCE_FILES "${_flyki_boundary_impl}")
            break()
        endif()
    endforeach()
    if(NOT _flyki_boundary_impl)
        list(APPEND FLYKI_MISSING_CMAES_FILES "one of: src/boundary_transformation.c, src/boundary_transformation.cpp")
    endif()
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

    add_library(cmaes_dependency_check OBJECT
        cmaes_dependency_check.cpp
    )
    target_include_directories(cmaes_dependency_check PRIVATE
        "${FLYKI_CMAES_INCLUDE_DIR}"
        "${FLYKI_BOUNDARY_INCLUDE_DIR}"
    )
    target_compile_features(cmaes_dependency_check PRIVATE cxx_std_17)

    add_executable(cbocco
        CBOCC.cpp
        CBOG_CBD.cpp
        CMAESO.cpp
        ${FLYKI_BENCHMARK_SOURCES}
        ${FLYKI_BENCHMARK_HEADERS}
    )
    target_include_directories(cbocco PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_features(cbocco PRIVATE cxx_std_17)
    target_link_libraries(cbocco PRIVATE flyki_cmaes grouping_core)
else()
    string(REPLACE ";" ", " FLYKI_MISSING_CMAES_FILES_TEXT "${FLYKI_MISSING_CMAES_FILES}")
    add_custom_target(cmaes_dependency_check
        COMMAND "${CMAKE_COMMAND}"
            "-DFLYKI_CMAES_TARGET_NAME=cmaes_dependency_check"
            "-DFLYKI_MISSING_CMAES_FILES=${FLYKI_MISSING_CMAES_FILES_TEXT}"
            -P "${CMAKE_CURRENT_LIST_DIR}/cmake/report_missing_cmaes.cmake"
        COMMENT "Reporting missing external CMA-ES dependency for cmaes_dependency_check"
        VERBATIM
    )
    add_custom_target(cbocco
        COMMAND "${CMAKE_COMMAND}"
            "-DFLYKI_CMAES_TARGET_NAME=cbocco"
            "-DFLYKI_MISSING_CMAES_FILES=${FLYKI_MISSING_CMAES_FILES_TEXT}"
            -P "${CMAKE_CURRENT_LIST_DIR}/cmake/report_missing_cmaes.cmake"
        COMMENT "Reporting missing external CMA-ES dependency for cbocco"
        VERBATIM
    )
endif()
