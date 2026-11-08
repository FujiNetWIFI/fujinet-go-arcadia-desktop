# arc_session: the frontend contract (core/include/arcsession.h), included
# from core/CMakeLists.txt once SDL3 is found.

set(ARC_SESSION_SOURCES
  src/session.c
  src/settings.c
  src/paths.c
  src/media.c
  src/zipread.c
  src/keyboard.c
  src/fujinet_runtime.c
  src/audio_sdl.c
  src/bindings.c
  src/hid_keys.c
  src/gamepad_sdl.c
  src/debug.c
  src/help.c)

# The Help, embedded: data/help/help.md as topics of styled spans
# (core/include/archelp.h), which every frontend lays out with its own
# toolkit. help_data.h declares the table; the build generates it.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(ARC_HELP_C "${CMAKE_BINARY_DIR}/generated/help_content.c")
add_custom_command(
  OUTPUT "${ARC_HELP_C}"
  COMMAND "${Python3_EXECUTABLE}" -I "${CMAKE_SOURCE_DIR}/tools/help/make-help.py"
          --src "${CMAKE_SOURCE_DIR}/data/help/help.md"
          --out-c "${ARC_HELP_C}"
  DEPENDS "${CMAKE_SOURCE_DIR}/tools/help/make-help.py"
          "${CMAKE_SOURCE_DIR}/data/help/help.md"
  COMMENT "Help"
  VERBATIM)

add_library(arc_session STATIC ${ARC_SESSION_SOURCES} "${ARC_HELP_C}")
set_target_properties(arc_session PROPERTIES
  C_STANDARD 11
  POSITION_INDEPENDENT_CODE ON)
target_include_directories(arc_session PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(arc_session PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${CMAKE_CURRENT_SOURCE_DIR}/debugger")
target_link_libraries(arc_session PUBLIC arc_core SDL3::SDL3)
# setenv()/getaddrinfo() under a strict -std=c11
target_compile_definitions(arc_session PRIVATE _GNU_SOURCE _DEFAULT_SOURCE)
set_property(SOURCE ${ARC_SESSION_SOURCES} APPEND PROPERTY COMPILE_OPTIONS
  "$<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra>"
  # paths are bounded by ARC_PATH_MAX and joined with snprintf on purpose;
  # GCC's truncation analysis flags every such join
  "$<$<C_COMPILER_ID:GNU>:-Wno-format-truncation>")

# Where an uninstalled build and an installed copy find the FujiNet runtime
# (core/src/paths.c probes these after the executable's own directory).
target_compile_definitions(arc_session PRIVATE
  ARC_DEV_FUJINET_OUT="${CMAKE_SOURCE_DIR}/tools/fujinet/work/out"
  ARC_INSTALL_DATADIR="${CMAKE_INSTALL_FULL_DATADIR}/fujinet-go-arcadia"
  ARC_INSTALL_LIBDIR="${CMAKE_INSTALL_FULL_LIBDIR}/fujinet-go-arcadia")
if(WIN32)
  target_link_libraries(arc_session PUBLIC ws2_32)
else()
  target_link_libraries(arc_session PUBLIC ${CMAKE_DL_LIBS})
endif()
