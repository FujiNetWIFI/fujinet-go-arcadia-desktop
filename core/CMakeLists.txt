# arc_core: the toolkit-agnostic, SDL-free Emerson Arcadia 2001 core.
#
#   arc/                the emulator: MAME's Arcadia 2001 driver, the
#                      Signetics 2650 and its disassembler, the 2637 UVI
#                      (video, pots, beeper) and the cart mappers transposed
#                      to C; the paced host thread and the FujiNet cartridge
#                      device. There is no BIOS and no character ROM to
#                      import: the glyph table is MAME's source.
#
#   arc/fuji-generated/ the cartridge firmware's OWN sources -- fujimail.c
#                      (the mailbox), arcmap.c (the image planner), arcapp.h
#                      (APPBANK serving), fujibus.c (the SLIP + FujiBus wire
#                      codec) and the baked CONFIG client -- staged by
#                      cmake/StageFujiProto.cmake. Compiling these rather
#                      than reimplementing them is what makes the emulated
#                      cartridge and the real RP2040 one identical by
#                      construction.
#
#   debugger/          the debugger engine (core/include/arcdebug.h).
#
# The one port-specific cartridge file is arc/fujinet_cart.c, transposed from
# the firmware's MAME device (pico/arcadia/emu/fujinet.cpp). The transport is
# arc/fujitcp_host.c, this app's own portable twin of the staged fujitcp.c.
#
# Nothing here links SDL. The session layer (SDL3 audio + gamepad) is a
# separate target so that stays true; no_sdl_link guards it.

set(FUJI_GEN "${CMAKE_SOURCE_DIR}/core/arc/fuji-generated")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(NOT EXISTS "${FUJI_GEN}/fuji_mailbox.h")
  message(FATAL_ERROR
    "Staged FujiNet sources are missing. cmake/StageFujiProto.cmake should "
    "have produced them; re-run with -DFUJI_RESTAGE=ON.")
endif()

# The staged cartridge sources (the staged fujitcp.c is not compiled; see
# arc/fujitcp_host.c).
set(ARC_FUJI_SOURCES
  ${FUJI_GEN}/fujimail.c
  ${FUJI_GEN}/fujibus.c
  ${FUJI_GEN}/arcmap.c)

# The emulator's own modules. A module that has not landed yet is skipped, so
# the library builds at every step of the bring-up.
set(ARC_EMU_SOURCES)
foreach(_f s2650 dasm2650 uvi sound carts machine fujinet_cart fujitcp_host host)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/arc/${_f}.c")
    list(APPEND ARC_EMU_SOURCES arc/${_f}.c)
  endif()
endforeach()

# The debugger drives the machine, so it joins once the machine and the
# debugger's own Arcadia port (vdcview.c is its marker) exist.
option(ARC_CORE_ONLY "Build only the emulator and its machine-level tests (no debugger, no session)" OFF)
set(ARC_DEBUGGER_SOURCES)
if(NOT ARC_CORE_ONLY AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/arc/machine.c"
   AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/debugger/vdcview.c")
  foreach(_f debugger.c expr.c fujitrace.c prompt.c symbols.c symbols_builtin.c vdcview.c)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/debugger/${_f}")
      list(APPEND ARC_DEBUGGER_SOURCES debugger/${_f})
    endif()
  endforeach()
endif()

add_library(arc_core STATIC
  ${ARC_EMU_SOURCES} ${ARC_FUJI_SOURCES} ${ARC_DEBUGGER_SOURCES})
set_target_properties(arc_core PROPERTIES
  C_STANDARD 11
  POSITION_INDEPENDENT_CODE ON)

target_include_directories(arc_core PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/arc"
  # public because the debugger and the tests read the cart's state through
  # the staged fuji_mailbox.h, arcmap.h and arcapp.h
  "${FUJI_GEN}")
target_include_directories(arc_core PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/debugger")

# getaddrinfo/clock_gettime/nanosleep under a strict -std=c11.
target_compile_definitions(arc_core PRIVATE _GNU_SOURCE _DEFAULT_SOURCE)

target_link_libraries(arc_core PUBLIC Threads::Threads)
if(WIN32)
  target_link_libraries(arc_core PUBLIC ws2_32)
else()
  target_link_libraries(arc_core PUBLIC m)
endif()

# Our own modules are warning-clean; the staged tree is built with whatever
# warnings upstream is comfortable with.
set(ARC_WARN_SOURCES ${ARC_EMU_SOURCES} ${ARC_DEBUGGER_SOURCES})
list(REMOVE_ITEM ARC_WARN_SOURCES debugger/symbols_builtin.c)
set_property(SOURCE ${ARC_WARN_SOURCES} APPEND PROPERTY COMPILE_OPTIONS
  "$<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra>")

# ---- arc_session: the frontend contract ----------------------------------
#
# A separate target from arc_core so the core stays SDL-free: audio_sdl.c and
# gamepad_sdl.c live here. Found-or-skipped, exactly as the frontends are:
# `-DFRONTEND=none` on a machine (or a cross-toolchain) with no SDL3 must
# still configure and build the emulator and its tests. Not built until the
# machine and the host it drives exist.
if(NOT ARC_CORE_ONLY AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/session.c"
   AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/arc/host.c"
   AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/arc/machine.c")
  find_package(SDL3 CONFIG QUIET)
  if(NOT SDL3_FOUND)
    message(STATUS "arc_session: SDL3 not found -- building the core and its "
                   "tests only (no audio, no gamepads, no frontends)")
  else()
    include(session.cmake)
  endif()
endif()

add_subdirectory(tests)
