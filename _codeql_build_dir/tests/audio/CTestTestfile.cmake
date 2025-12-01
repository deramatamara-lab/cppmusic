# CMake generated Testfile for 
# Source directory: /home/runner/work/cppmusic/cppmusic/tests/audio
# Build directory: /home/runner/work/cppmusic/cppmusic/_codeql_build_dir/tests/audio
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[AudioTests]=] "/home/runner/work/cppmusic/cppmusic/_codeql_build_dir/tests/audio/audio_tests")
set_tests_properties([=[AudioTests]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/cppmusic/cppmusic/tests/audio/CMakeLists.txt;30;add_test;/home/runner/work/cppmusic/cppmusic/tests/audio/CMakeLists.txt;0;")
subdirs("engine")
subdirs("dsp")
