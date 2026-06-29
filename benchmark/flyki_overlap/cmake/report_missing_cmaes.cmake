if(NOT DEFINED FLYKI_MISSING_CMAES_FILES)
    set(FLYKI_MISSING_CMAES_FILES "unknown CMA-ES files")
endif()
if(NOT DEFINED FLYKI_CMAES_TARGET_NAME)
    set(FLYKI_CMAES_TARGET_NAME "cbocco")
endif()

message(FATAL_ERROR
    "${FLYKI_CMAES_TARGET_NAME} target is unavailable because the original external CMA-ES dependency is missing.\n"
    "Missing: ${FLYKI_MISSING_CMAES_FILES}\n"
    "Expected layout:\n"
    "  third_party/cmaes/include/cmaes_interface.h\n"
    "  third_party/cmaes/include/boundary_transformation.h\n"
    "  third_party/cmaes/src/<cmaes implementation source>\n"
    "  third_party/cmaes/src/<boundary transformation source>\n"
    "Example configure command:\n"
    "  cmake -S benchmark/flyki_overlap -B build/flyki -DCMAKE_BUILD_TYPE=Release -DCMAES_ROOT=third_party/cmaes\n"
    "If sources use different names, pass -DCMAES_SOURCES=\"src/cmaes.c;src/boundary_transformation.c\".\n"
    "Do not stub, rewrite, or silently replace CMA-ES. flyki_core and CWVIG grouping targets do not require CMA-ES."
)
