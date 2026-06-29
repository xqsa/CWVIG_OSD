if(NOT DEFINED FLYKI_MISSING_CMAES_FILES)
    set(FLYKI_MISSING_CMAES_FILES "unknown CMA-ES files")
endif()

message(FATAL_ERROR
    "cbocco target is unavailable because the original external CMA-ES dependency is missing.\n"
    "Missing: ${FLYKI_MISSING_CMAES_FILES}\n"
    "Do not stub or rewrite CMA-ES for Phase 0. Provide the original CMA-ES C/C++ distribution with "
    "-DFLYKI_CMAES_DIR=<path> or pass -DFLYKI_CMAES_SOURCES=<semicolon-separated-sources>."
)
