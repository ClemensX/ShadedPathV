cmake_minimum_required(VERSION 3.27.0)
project(shadedpathv VERSION 0.1.0 LANGUAGES C CXX)

# determine compiler / settings
set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")

# test for OpenXR available:
include(CheckCXXSourceCompiles)


add_library(sp_compiler_flags INTERFACE)
target_compile_features(sp_compiler_flags INTERFACE cxx_std_20)
# limit compiler options to our own builds with BUILD_INTERFACE generator:
target_compile_options(sp_compiler_flags INTERFACE
  "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wall;-Wextra;-Wshadow;-Wformat=2;-Wunused>>"
  "$<${msvc_cxx}:$<BUILD_INTERFACE:-W3>>"
)
configure_file(tutorialConfig.h.in tutorialConfig.h)
add_subdirectory(shadedpath_lib)

add_executable(shadedpathv tutorialConfig.cpp)

target_link_libraries(shadedpathv PUBLIC shadedpath.lib sp_compiler_flags)

target_include_directories(shadedpathv PUBLIC
        "${CMAKE_CURRENT_BINARY_DIR}"
    )

check_cxx_source_compiles("
    int main(int argc, char* argv[])
    { return 0;
    }
" OPENXR_AVAILABLE)
    if (OPENXR_AVAILABLE)
      message(STATUS "openxr available")
      target_compile_definitions(shadedpath_lib PRIVATE "OPENXR_AVAILABLE")
    else()
      message(STATUS "openxr NOT available")
    endif()
    
#set(CPACK_PROJECT_NAME ${PROJECT_NAME})
#set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
#include(CPack)

# install:
install(TARGETS shadedpathv DESTINATION bin)
install(FILES "${PROJECT_BINARY_DIR}/tutorialConfig.h" DESTINATION include)

# tests:
enable_testing()
#include(CTestConfig.cmake)
include(CTest)
add_test(NAME RunsOk COMMAND shadedpathv 25)
add_test(NAME Usage COMMAND shadedpathv)
set_tests_properties(Usage PROPERTIES PASS_REGULAR_EXPRESSION "shadedpathv .*number")
add_test(NAME StandardUse COMMAND shadedpathv 49)
set_tests_properties(StandardUse PROPERTIES PASS_REGULAR_EXPRESSION "49 is 7")
# test generation:
function(do_test target arg result)
  add_test(NAME Comp${arg} COMMAND shadedpathv ${arg})
  set_tests_properties(Comp${arg}
    PROPERTIES PASS_REGULAR_EXPRESSION ${result}
    )
endfunction()
# do a bunch of result based tests
do_test(Tutorial 4 "4 is 2")
do_test(Tutorial 9 "9 is 3")
do_test(Tutorial 5 "5 is 2.236")
do_test(Tutorial 7 "7 is 2.645")
do_test(Tutorial 25 "25 is 5")
do_test(Tutorial -25 "-25 is (-nan|nan|0)")
do_test(Tutorial 0.0001 "0.0001 is 0.01")

# recompiling with changed options:
# cmake -S . -B ./build -DUSE_MYMATH=OFF
# cmake --build ./build
# ./build/shadedpathv 2
# run installation: e.g.:
# cmake --install ./build/ --prefix "./mybin"
# ctest --test-dir ./build/
# upload to cdash:
# ctest --test-dir ./build/ -D Experimental

# verbose:
# cmake --trace-expand -S . -B ./build > trace.txt 2>&1