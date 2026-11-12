target_include_directories(neoemu PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}/musashi"
	"${CMAKE_CURRENT_SOURCE_DIR}/z80"
)

target_sources(neoemu PRIVATE FILE_SET CXX_MODULES
	BASE_DIRS main
	FILES main/system.ccm
)

target_sources(neoemu PRIVATE
	main/Main.cc
	main/AppMeta.cc
	main/input.cc
	main/options.cc
	main/state.cc
	main/unzip.cc
	main/EmuMenuViews.cc
	main/DuoGamepad.cc
	z80/z80_core.cc
	gngeo/mamez80_interf.cc
	gngeo/ym2610/2610intf.c
	gngeo/ym2610/ym2610.c
	gngeo/debug.c
	gngeo/emu.c
	gngeo/fileio.c
	gngeo/mame_layer.c
	gngeo/memory.c
	gngeo/neoboot.c
	gngeo/neocrypt.c
	gngeo/pd4990a.c
	gngeo/roms.c
	gngeo/timer.c
	gngeo/video.c
)

target_sources(neoemu PRIVATE
	musashi/m68kcpu.c
	musashi/m68kops.c
	musashi/m68kdasm.c
	musashi/softfloat/softfloat.c
	gngeo/musashi_interf.cc
)
set_source_files_properties(
	musashi/m68kcpu.c musashi/m68kops.c musashi/m68kdasm.c musashi/softfloat/softfloat.c
	PROPERTIES COMPILE_OPTIONS "-Wno-unused-parameter;-Wno-unused-function;-Wno-sign-compare"
)