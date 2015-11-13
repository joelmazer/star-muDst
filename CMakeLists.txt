cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(star-muDst)


# Add to path in order to pick up the FindXXX.cmake files included in this project
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# This project depends on ROOT
find_package(ROOT)

if(NOT ROOT_FOUND)
	message(FATAL_ERROR "Fatal error: ROOT package not found")
endif()


add_definitions(-D__ROOT__)

include_directories(
	${ROOT_INCLUDE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${STAR_BASE_DIR}
	${STAR_STEVENT_DIR}
	${STAR_SIM_DIR}
	${STAR_TABLES_DIR}
	${STAR_EMC_DIR}
	${STAR_DAQ_DIR}
	${STAR_FGT_DIR}
	${STAR_PAMS_DIR}/global/inc
)


# StMuDSTMaker library
file(GLOB StMuDSTMaker_SOURCES "StMuDSTMaker/COMMON/*.cxx" "StMuDSTMaker/EZTREE/*.cxx")
root_generate_linkdef_and_dictionary(StMuDSTMaker HEADERS "StMuDSTMaker/COMMON/*.h" "StMuDSTMaker/COMMON/*.hh" "StMuDSTMaker/EZTREE/*.h")
add_library (StMuDSTMaker SHARED ${StMuDSTMaker_SOURCES} StMuDSTMaker_Cint)


# StStrangeMuDstMaker library
file(GLOB StStrangeMuDstMaker_SOURCES "StStrangeMuDstMaker/*.cxx" "StStrangeMuDstMaker/*.cc")
root_generate_dictionary(StStrangeMuDstMaker_Cint StStrangeMuDstMaker/*.h StStrangeMuDstMaker/*.hh LINKDEF StStrangeMuDstMaker/StStrangeMuDstMakerLinkDef.h OPTIONS -D__ROOT__)
add_library (StStrangeMuDstMaker SHARED ${StStrangeMuDstMaker_SOURCES} StStrangeMuDstMaker_Cint)


install(TARGETS StMuDSTMaker StStrangeMuDstMaker DESTINATION "lib")
