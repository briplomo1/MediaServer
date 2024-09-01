# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/bripl/source/repos/MediaServer/out/build/x64-debug/_deps/ffmpeg-src"
  "C:/Users/bripl/source/repos/MediaServer/out/build/x64-debug/_deps/ffmpeg-build"
  "C:/Users/bripl/source/repos/MediaServer/out/build/x64-debug/_deps/ffmpeg-subbuild/ffmpeg-populate-prefix"
  "C:/Users/bripl/source/repos/MediaServer/out/build/x64-debug/_deps/ffmpeg-subbuild/ffmpeg-populate-prefix/tmp"
  "C:/Users/bripl/source/repos/MediaServer/out/build/x64-debug/_deps/ffmpeg-subbuild/ffmpeg-populate-prefix/src/ffmpeg-populate-stamp"
  "C:/Users/bripl/source/repos/MediaServer/out/build/x64-debug/_deps/ffmpeg-subbuild/ffmpeg-populate-prefix/src"
  "C:/Users/bripl/source/repos/MediaServer/out/build/x64-debug/_deps/ffmpeg-subbuild/ffmpeg-populate-prefix/src/ffmpeg-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/bripl/source/repos/MediaServer/out/build/x64-debug/_deps/ffmpeg-subbuild/ffmpeg-populate-prefix/src/ffmpeg-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/bripl/source/repos/MediaServer/out/build/x64-debug/_deps/ffmpeg-subbuild/ffmpeg-populate-prefix/src/ffmpeg-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
