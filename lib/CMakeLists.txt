# Copyright 2011,2012 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

########################################################################
# Setup library
########################################################################
include(GrPlatform) #define LIB_SUFFIX

include_directories(
	${Boost_INCLUDE_DIR}
	${QT_INCLUDE_DIRS}
)
link_directories(
	${Boost_LIBRARY_DIRS}
  	${QT_LIBRARIES}
)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

list(APPEND avs4000_sources
    avs4000rx_impl.cc
    avs4000tx_impl.cc
    avs4000client.cpp
    RxSignalClient.cpp
    TcpRxSignalClient.cpp
    TxSignalClient.cpp
    TcpTxSignalClient.cpp
    SocketRxBuffer.cpp
    SocketTxBuffer.cpp
    BlockFifo.cpp
    v49packet.cpp
    TcpJSONClient.cpp
    avs4000stat_impl.cc
    avsapicon.cpp
    qml.qrc
)

set(avs4000_sources "${avs4000_sources}" PARENT_SCOPE)
if(NOT avs4000_sources)
	MESSAGE(STATUS "No C++ sources... skipping lib/")
	return()
endif(NOT avs4000_sources)

add_library(gnuradio-avs4000 SHARED ${avs4000_sources})
target_link_libraries(gnuradio-avs4000 ${Boost_LIBRARIES} ${GNURADIO_ALL_LIBRARIES} ${QT_LIBRARIES} )
set_target_properties(gnuradio-avs4000 PROPERTIES DEFINE_SYMBOL "gnuradio_avs4000_EXPORTS")

if(APPLE)
    set_target_properties(gnuradio-avs4000 PROPERTIES
        INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
    )
endif(APPLE)

########################################################################
# Install built library files
########################################################################
install(TARGETS gnuradio-avs4000
    LIBRARY DESTINATION lib${LIB_SUFFIX} # .so/.dylib file
    ARCHIVE DESTINATION lib${LIB_SUFFIX} # .lib file
    RUNTIME DESTINATION bin              # .dll file
)

########################################################################
# Build and register unit test
########################################################################
include(GrTest)

include_directories(
	${CPPUNIT_INCLUDE_DIRS}
	${QT_INCLUDE_DIRS}
)

list(APPEND test_avs4000_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/test_avs4000.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_avs4000.cc
)

add_executable(test-avs4000 ${test_avs4000_sources})

target_link_libraries(
  test-avs4000
  ${GNURADIO_RUNTIME_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CPPUNIT_LIBRARIES}
  ${QT_LIBRARIES}
  gnuradio-avs4000
)

GR_ADD_TEST(test_avs4000 test-avs4000)
