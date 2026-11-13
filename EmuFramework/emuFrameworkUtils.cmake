file(REAL_PATH ${CMAKE_CURRENT_LIST_FILE} EMUFRAMEWORK_PATH)
cmake_path(GET EMUFRAMEWORK_PATH PARENT_PATH EMUFRAMEWORK_PATH)
message("EmuFramework Path: ${EMUFRAMEWORK_PATH}")

function(addMednafenFlags target)
	message(FATAL_ERROR "mednafen_common was removed from NEO-Duo; ${target} does not use it")
endfunction()