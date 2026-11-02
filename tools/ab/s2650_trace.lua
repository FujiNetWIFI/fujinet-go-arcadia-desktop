-- s2650_trace.lua -- MAME side of the 2650 conformance test: logs the 2650's
-- state before every instruction of a probe cartridge on the arcadia driver,
-- then exits when the probe reaches its end marker.
--
-- Run as an -autoboot_script of `mame arcadia -cart probe.bin -debug -debugger
-- none ...` (tools/ab/s2650_ab.py does). The "none" debugger front end never
-- stops the CPU, so the trace is set up here with the debugger's `trace`
-- command and its `tracelog` action, which MAME runs before each instruction.
--
-- Environment:
--   S2650_TRACE   the trace file to write
--   S2650_DONE    the end marker address, hex (default FFD): a breakpoint
--                 there ends the run, and a frame limit backs it up
--   S2650_FRAMES  the frame limit (default 200)
--
-- One row per instruction, the state BEFORE it executes, all hex but the last:
--   pc ps r0 r1 r2 r3 totalcycles
-- (ps is PSU << 8 | PSL; r1-r3 are the active register set; totalcycles is
-- the CPU's clocks since power-on.)
--
-- SPDX-License-Identifier: GPL-3.0-or-later

local trace_file = os.getenv("S2650_TRACE") or "s2650.trace"
local done = os.getenv("S2650_DONE") or "FFD"
local limit = tonumber(os.getenv("S2650_FRAMES") or "200")

local dbg = manager.machine.debugger
dbg:command('trace ' .. trace_file ..
            ',maincpu,noloop,{tracelog "%04X %04X %02X %02X %02X %02X %d\\n"' ..
            ',pc,ps,r0,r1,r2,r3,totalcycles}')
-- the breakpoint's action quits MAME; the trace flushes after every line
dbg:command('bpset ' .. done .. ',1,{quit}')

local frames = 0
_G.S2650_FRAME = emu.add_machine_frame_notifier(function()
  frames = frames + 1
  if frames >= limit then
    manager.machine:exit()
  end
end)
