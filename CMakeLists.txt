# minimum required version
if(WIN32)
  cmake_minimum_required(VERSION 3.8)
else()
  cmake_minimum_required(VERSION 2.6)
endif()

# main project name
project(storage_sas_sample)

# set compilers flags
if(NOT WIN32)
    set (CMAKE_CXX_FLAGS "-std=c++11 -pthread ${CMAKE_CXX_FLAGS}")
else()
    set (CMAKE_CXX_STANDARD 11)
    set (CMAKE_CXX_STANDARD_REQUIRED ON)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHa")
    set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set (CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
endif()

# import azure storage cpp module
find_path(WASTORAGE_INCLUDE_DIR was/blob.h)
include_directories(${WASTORAGE_INCLUDE_DIR})
if (NOT WIN32)
  find_library(WASTORAGE_LIBRARY azurestorage)
  find_library(CPPREST_LIBRARY cpprest)
  find_library(BOOST_LOG_LIBRARY boost_log)
  find_library(BOOST_THREAD_LIBRARY boost_thread)
  find_library(SSL_LIBRARY ssl)
  find_library(XML2_LIBRARY xml2)
  find_library(LZMA_LIBRARY lzma)
  find_library(UUID_LIBRARY uuid)
  find_library(Z_LIBRARY z)
  find_library(CRYPTO_LIBRARY crypto)
else()
  find_library(WASTORAGE_LIBRARY wastorage)
  find_library(CPPREST_LIBRARY
    NAMES cpprest cpprest_2_10)
endif()

# Build sample
add_executable(storage_sas_sample ./src/main.cc)
set_target_properties(storage_sas_sample PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(NOT WIN32)
  target_link_libraries(storage_sas_sample
    ${OPENSSL_LIBRARY}
    ${WASTORAGE_LIBRARY}
    ${CPPREST_LIBRARY}
    ${BOOST_LOG_LIBRARY}
    ${BOOST_THREAD_LIBRARY}
    ${SSL_LIBRARY}
    ${XML2_LIBRARY}
    ${LZMA_LIBRARY}
    ${UUID_LIBRARY}
    ${Z_LIBRARY}
    ${CRYPTO_LIBRARY}
    ${CMAKE_DL_LIBS}
  )
else()
  target_link_libraries(storage_sas_sample
    ${OPENSSL_LIBRARY}
    ${WASTORAGE_LIBRARY}
    ${CPPREST_LIBRARY}
  )
endif()
