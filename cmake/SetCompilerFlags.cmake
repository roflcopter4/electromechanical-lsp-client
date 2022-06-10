#####################################################################################################
## General flags ##
#####################################################################################################


set (WARNS
        -Wall -Wextra
        -Werror=format-extra-args
        -Wimplicit-fallthrough
        -Werror=invalid-pch
)
set (C_ONLY_WARNS
        -Werror=implicit
        -Werror=implicit-function-declaration
        -Werror=incompatible-pointer-types
        -Werror=int-conversion
        -Werror=pointer-to-int-cast
)


if (NOT BUILD_DIST)
    set (MARCH_SETTING -mtune=native -march=native)
endif()

if (SANITIZE)
    set (SANIT -fsanitize=undefined -fsanitize=bounds -fsanitize=bool)
    if ("${SANITIZE}" STREQUAL "thread")
        set (SANIT ${SANIT} -fsanitize=thread)
    elseif ("${SANITIZE}" STREQUAL "memory")
        set (SANIT -fsanitize=undefined -fsanitize=memory)
    else ()
        set (SANIT ${SANIT} -fsanitize=address -fsanitize-address-use-after-scope)
    endif()
endif()

set (BASE ${MARCH_SETTING} ${SANIT}
    -pipe -fdiagnostics-color=always
    -mprefer-vector-width=512 -U_FORTIFY_SOURCE
    -fno-strict-aliasing #-fopenmp
)
          
set (CFLAGS_DEBUG_COMMON
     -Og -g -UNDEBUG -D_FORTIFY_SOURCE=3
     -Wextra -Wpedantic -Wformat
)
set (CFLAGS_RELWITHDEBINFO_COMMON
     -O2 -g -D_FORTIFY_SOURCE=2
     -Wextra -ftree-vectorize -Wextra
)
set (CFLAGS_RELEASE_COMMON
     -Ofast -ftree-vectorize -g
     -DNDEBUG
     -fno-strict-aliasing
     -fno-stack-protector
)


################################################################################
# Compiler specific flags. Currently these override $CFLAGS.


if (NOT MSVC)
    if (SANITIZE)
        set (CFLAGS_DEBUG_COMMON ${CFLAGS_DEBUG_COMMON}
            -fno-omit-frame-pointer -fno-optimize-sibling-calls)
    endif()
else()
    message(WARNING "Can't possibly sanitize MSVC. Try nuking from orbit")
endif()


#-----------------------------------------------------------------------------------------
#-- CLANG --
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

    set (WARNS ${WARNS}
        #-Weverything
        -Wno-c++98-compat -Wno-c++98-compat-pedantic
        -Wno-zero-as-null-pointer-constant
        -Wno-disabled-macro-expansion -Wno-reserved-macro-identifier -Wno-unused-macros
        -Wno-weak-vtables
        -Wno-shorten-64-to-32
        -Wno-ctad-maybe-unsupported
        -Wno-missing-prototypes
        # -fsanitize=address,undefined
    )
    set (WARNS ${WARNS}
         -Wno-gnu -Wno-gnu-zero-variadic-macro-arguments
         -Wno-gnu-statement-expression -Werror=return-type
         -Werror=inline-namespace-reopened-noninline
    )

    set (_debug_flags 
        -g -gdwarf-5
    )

    if (${CMAKE_BUILD_TYPE} STREQUAL "Release" OR ${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
        set (LTO_STR -flto=full
                     -fwhole-program-vtables
                     # -fvirtual-function-elimination
                     -fsplit-lto-unit
            )

        if (MINGW OR WIN32)
            set (LTO_LINK_STR -Wl,--lto-O3 ${LTO_STR})
        else()
            set (LTO_LINK_STR ${LTO_STR} -Wl,--lto-O3 -Wl,--lto-whole-program-visibility)
        endif()

        set (__EXTRA_C_CXX_RELEASE_FLAGS
            # -fprofile-generate
            -fprofile-use
            -mllvm -march=native
            -mllvm --aggressive-ext-opt
            -mllvm --extra-vectorizer-passes
            -mllvm --exhaustive-register-search
            -mllvm --enable-unsafe-fp-math
            -mllvm --optimize-regalloc
            -mllvm --scalar-evolution-use-expensive-range-sharpening
            -mllvm --slp-max-vf=0
            -mllvm --slp-vectorize-hor
            -mllvm --x86-indirect-branch-tracking
            -mllvm --interleave-loops
            # -mllvm --ir-outliner
            -mllvm --enable-post-misched
            -mllvm --enable-nontrivial-unswitch
            -mllvm --enable-nonnull-arg-prop
        )
        set (LTO_LINK_STR ${LTO_STR}
            -Wl,--mllvm=-march=native
            -Wl,--mllvm=--aggressive-ext-opt
            -Wl,--mllvm=--extra-vectorizer-passes
            -Wl,--mllvm=--exhaustive-register-search
            -Wl,--mllvm=--enable-unsafe-fp-math
            -Wl,--mllvm=--optimize-regalloc
            -Wl,--mllvm=--scalar-evolution-use-expensive-range-sharpening
            -Wl,--mllvm=--slp-max-vf=0
            -Wl,--mllvm=--slp-vectorize-hor
            -Wl,--mllvm=--whole-program-visibility
            -Wl,--mllvm=--x86-indirect-branch-tracking
            -Wl,--mllvm=--interleave-loops
            -Wl,--mllvm=--ir-outliner
            -Wl,--mllvm=--enable-post-misched
            -Wl,--mllvm=--enable-nontrivial-unswitch
            -Wl,--mllvm=--enable-nonnull-arg-prop
        )
    else()
        set (LTO_STR #-flto=full
                     #-fwhole-program-vtables
                     #-fvirtual-function-elimination
                     #-fsplit-lto-unit
                     )

        if (MINGW OR WIN32)
            set (LTO_LINK_STR ${LTO_STR})
        else()
            set (LTO_LINK_STR ${LTO_STR} #-Wl,--lto-O1 -Wl,--lto-whole-program-visibility
            )
        endif()
    endif()

    #------------------------------------------------------------------------------------
    if (WIN32 OR MINGW OR MSYS)
        if (FALSE)
            set (_EXTRA ${_EXTRA} -fansi-escape-codes -target x86_64-w64-windows -fc++-abi=microsoft)
            string (JOIN " " CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}"
                    ${CLANG_STDLIB} ${LTO_STR}
                    -target x86_64-w64-windows -fc++-abi=microsoft
            )
        else()
            set (CLANG_STDLIB -stdlib=libstdc++)
            set (_EXTRA ${_EXTRA} -fansi-escape-codes)
            string (JOIN " " CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}"
                    ${CLANG_STDLIB}
                    ${LTO_LINK_STR}
                    -fuse-ld=lld
                    --rtlib=compiler-rt
                    --unwindlib=libgcc
                    #-Wl,--pdb=
            )
        endif()

    #------------------------------------------------------------------------------------
    else() # NOT WIN32

        set (CLANG_STDLIB -stdlib=libc++)
        # set (CLANG_STDLIB -stdlib=libstdc++)
        string (JOIN " " CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}"
                -fuse-ld=lld
                #-rtlib=libgcc
                -rtlib=compiler-rt
                -unwindlib=libgcc
                # -unwindlib=libunwind
                # -rdynamic
                -Wl,--as-needed
                ${CLANG_STDLIB}
                ${LTO_LINK_STR}
        )
    endif()
    #------------------------------------------------------------------------------------

    set (__EXTRA_C_CXX_RELEASE_FLAGS ${__EXTRA_C_CXX_RELEASE_FLAGS} -fslp-vectorize)

    set (_EXTRA ${_EXTRA} ${WARNS}
              -fintegrated-as -fintegrated-cc1 -fno-legacy-pass-manager
              ${_debug_flags}
    )
    string (JOIN " " CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}"
            ${_debug_flags})

    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti ${CLANG_STDLIB}")

    find_program(LLVM_AR "llvm-ar" REQUIRED)
    find_program(LLVM_NM "llvm-nm" REQUIRED)
    find_program(LLVM_RANLIB "llvm-ranlib" REQUIRED)
    set (CMAKE_AR "${LLVM_AR}")
    set (CMAKE_NM "${LLVM_NM}")
    set (CMAKE_RANLIB "${LLVM_RANLIB}")

#-----------------------------------------------------------------------------------------
#-- GCC --

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

    set (WARNS ${WARNS}
         -Wsuggest-attribute=noreturn
         -Wno-suggest-attribute=format
         -Wsuggest-attribute=const -Wsuggest-attribute=pure
         -Wsuggest-attribute=cold
         -Wsuggest-attribute=malloc
         -Wattributes -fdiagnostics-show-option
    )
    
    set (_EXTRA ${_EXTRA}
        -g3
        -gdwarf-4
        -ggdb
        -gstatement-frontiers
        # -gstrict-dwarf
        -gas-loc-support 
        -gas-locview-support
        # -fno-dwarf2-cfi-asm
        -gcolumn-info
        -ginline-points
        -fvar-tracking
        -fvar-tracking-assignments
        -gvariable-location-views
        -fdebug-types-section
        -gdescribe-dies
        -fconcepts-diagnostics-depth=100
        # -fanalyzer
    )

    set (__EXTRA_C_CXX_RELEASE_FLAGS
         -ftree-slp-vectorize
         -fvect-cost-model=unlimited
         -mprefer-vector-width=512
         -fno-stack-protector
         -ffast-math

         -floop-nest-optimize
         -floop-parallelize-all
         -fgraphite
         -fgraphite-identity

         -fdevirtualize-speculatively
         -fdevirtualize-at-ltrans

         # -fauto-profile
         # -fprofile-generate
         -fprofile-use
         -fprofile-correction

         -fsched-pressure
         -fsched-spec-load
         -fsched-spec-load-dangerous

         -ftree-loop-im
         -floop-nest-optimize
         -ftree-loop-ivcanon
         -fivopts

         -fgcse-sm
         -fgcse-las
         -fipa-pta
         -fipa-cp
    )
    
    if (${CMAKE_BUILD_TYPE} STREQUAL "Release" OR ${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
        set (LTO_STR "-flto=auto -fno-fat-lto-objects -fuse-linker-plugin")
    endif()

    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti")

    string (JOIN " " CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}"
        -fuse-ld=bfd -Wl,-O3
        ${LTO_STR}
    )
    if (MINGW)
    else ()
        # string (JOIN " " CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}"
        #     -rdynamic
        # )
    endif()

    find_program(GCC_AR "gcc-ar" REQUIRED)
    find_program(GCC_NM "gcc-nm" REQUIRED)
    find_program(GCC_RANLIB "gcc-ranlib" REQUIRED)
    set (CMAKE_AR     "${GCC_AR}")
    set (CMAKE_NM     "${GCC_NM}")
    set (CMAKE_RANLIB "${GCC_RANLIB}")

#-----------------------------------------------------------------------------------------
#-- MSVC --
elseif (MSVC)

   # Allow use of "deprecated" function names in MSVC (read/write)
   add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
   set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Zc:preprocessor /std:c17")
   set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:preprocessor /Zc:__cplusplus /std:c++20")

endif()

set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_SHARED_LINKER_FLAGS}")


#####################################################################################################
# Misc

if (MINGW)
    FIX_WINDOWS_PATHS(CMAKE_C_FLAGS)
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-allow-multiple-definition")
endif()

if (MSVC)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP /utf-8")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /utf-8")
else()
    list(APPEND CMAKE_C_FLAGS_DEBUG
        ${BASE} ${CFLAGS_DEBUG_COMMON} ${WARNS} ${_EXTRA} ${CMAKE_C_FLAGS} ${LTO_STR})
    list(APPEND CMAKE_C_FLAGS_MINSIZEREL
        ${BASE} ${CFLAGS_RELEASE_COMMON} -Os -s ${WARNS} ${_EXTRA} ${CMAKE_C_FLAGS})
    list(APPEND CMAKE_C_FLAGS_RELEASE
        ${CMAKE_C_FLAGS_RELEASE} ${BASE} ${CFLAGS_RELEASE_COMMON}
        ${LTO_STR} ${WARNS} ${_EXTRA} ${CMAKE_C_FLAGS} ${__EXTRA_C_CXX_RELEASE_FLAGS})
    list(APPEND CMAKE_C_FLAGS_RELWITHDEBINFO
        ${CMAKE_C_FLAGS_RELWITHDEBINFO} ${BASE} ${CFLAGS_RELWITHDEBINFO_COMMON}
        ${LTO_STR} ${WARNS} ${_EXTRA} ${CMAKE_C_FLAGS} ${__EXTRA_C_CXX_RELEASE_FLAGS})

    string(JOIN " " CMAKE_CXX_FLAGS "-std=gnu++${CMAKE_CXX_STANDARD}" "${CMAKE_CXX_FLAGS}")

    set(CMAKE_CXX_FLAGS_DEBUG          ${CMAKE_C_FLAGS_DEBUG}          ${CMAKE_CXX_FLAGS})
    set(CMAKE_CXX_FLAGS_RELEASE        ${CMAKE_C_FLAGS_RELEASE}        ${CMAKE_CXX_FLAGS})
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO} ${CMAKE_CXX_FLAGS})
    set(CMAKE_CXX_FLAGS_MINSIZEREL     ${CMAKE_C_FLAGS_MINSIZEREL}     ${CMAKE_CXX_FLAGS})

    list(APPEND CMAKE_C_FLAGS ${CMAKE_C_FLAGS} ${C_ONLY_WARNS})
    list(JOIN CMAKE_C_FLAGS " " CMAKE_C_FLAGS)

    string(TOUPPER ${CMAKE_BUILD_TYPE} _upper_CMAKE_BUILD_TYPE)
    set (ALL_THE_C_FLAGS   ${CMAKE_C_FLAGS_${_upper_CMAKE_BUILD_TYPE}})
    set (ALL_THE_CXX_FLAGS ${CMAKE_CXX_FLAGS_${_upper_CMAKE_BUILD_TYPE}})

    list(JOIN CMAKE_C_FLAGS_DEBUG            " " CMAKE_C_FLAGS_DEBUG            )
    list(JOIN CMAKE_C_FLAGS_RELEASE          " " CMAKE_C_FLAGS_RELEASE          )
    list(JOIN CMAKE_C_FLAGS_RELWITHDEBINFO   " " CMAKE_C_FLAGS_RELWITHDEBINFO   )
    list(JOIN CMAKE_C_FLAGS_MINSIZEREL       " " CMAKE_C_FLAGS_MINSIZEREL       )
    list(JOIN CMAKE_CXX_FLAGS_DEBUG          " " CMAKE_CXX_FLAGS_DEBUG          )
    list(JOIN CMAKE_CXX_FLAGS_RELEASE        " " CMAKE_CXX_FLAGS_RELEASE        )
    list(JOIN CMAKE_CXX_FLAGS_RELWITHDEBINFO " " CMAKE_CXX_FLAGS_RELWITHDEBINFO )
    list(JOIN CMAKE_CXX_FLAGS_MINSIZEREL     " " CMAKE_CXX_FLAGS_MINSIZEREL     )
endif()

unset (BASE)
unset (WARNS)
unset (_EXTRA)
