
set(SPARSEHASH_PATHS ${SPARSEHASH_ROOT} $ENV{SPARSEHASH_ROOT})

find_path(SPARSEHASH_INCLUDE_DIR
        sparse_hash_map
        PATH_SUFFIXES include google
        PATHS ${SPARSEHASH_PATHS}
)

find_package_handle_standard_args(SPARSEHASH REQUIRED_VARS SPARSEHASH_INCLUDE_DIR)
