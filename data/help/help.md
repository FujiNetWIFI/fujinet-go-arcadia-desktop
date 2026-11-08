This is the text of FujiNet Go Arcadia 2001's built-in Help (Help ▸ FujiNet Go
Arcadia 2001 Help, or Ctrl+Shift+H; ⌘? on a Mac). The app shows each part only
to the frontend it is about; here you see them all. Every topic below is one
page in the app.

# Getting started

FujiNet Go Arcadia 2001 is an Emerson Arcadia 2001 with a FujiNet cartridge
in its slot. The console is MAME's: the Signetics 2650 processor, the 2637
UVI video and sound chip, and the two hand controllers. The FujiNet runs
inside the app, so CONFIG comes up at once and can mount games and programs
from the network.

1. **Start the app.** The console powers on and runs FujiNet's CONFIG. The
   Arcadia 2001 has no BIOS and no character ROM to import: nothing but your
   own games is needed.
2. **Pick something in CONFIG.** Choose a host (the SD folder or a server on
   the network) and a program on it. FujiNet sends it to the cartridge and
   it starts. See [FujiNet](#fujinet).
3. **Or open a cartridge of your own.** **Open Cartridge…**
   (<kbd>Ctrl+O</kbd>) takes an image of up to 8 KB: a `.bin`, a `.rom` or an
   `.arc`, or a zip of one. See [Cartridges](#cartridges).
4. **Play.** The keys, the gamepads and the Controllers window
   (<kbd>F9</kbd>) work the two hand controllers and the console's buttons.
   See [Controllers and keys](#controllers-and-keys).
5. **Change the console.** [Preferences](#preferences) picks the machine,
   the picture, the sound, the controls and FujiNet.

## This Help

The list on the left has a topic for each part of the app; the search box
above it finds every section that has all the words you type. A **Help**
button in Preferences, the Debugger, the Controllers window and the FujiNet
console log opens the topic for what it shows, and
<kbd>Ctrl+Shift+H</kbd> does the same for the window you are in.

<!-- only: gnome kde windows -->

In Preferences, the Debugger and the console log, <kbd>F1</kbd> does too.
(In the main window <kbd>F1</kbd> is the console's RESET.)

<!-- end -->

The same text is on GitHub, as
[data/help/help.md](https://github.com/FujiNetWIFI/fujinet-go-arcadia-desktop/blob/main/data/help/help.md).

<!-- only: macos -->

On a Mac, the Help menu's search field also finds Help topics.

<!-- end -->

<!-- context: main -->
# The window

The console's picture fills the window. The menus have:

- **Open Cartridge…**, **Eject Cartridge** and **Import Cartridge to SD…**:
  see [Cartridges](#cartridges).
- **Pause**, **Reset** and **Reset to CONFIG**.
- **Controllers** (<kbd>F9</kbd>) and **Debugger** (<kbd>F12</kbd>).
- **TV Aspect**, **Smooth Scaling** and **Fullscreen** (<kbd>F11</kbd>).
- **FujiNet Web UI** and **Console Log**.
- **Preferences** (<kbd>Ctrl+,</kbd>), **Help**, **About** and **Quit**
  (<kbd>Ctrl+Q</kbd>).

<!-- only: gnome -->

On GNOME, everything is in the main menu (the button at the right of the
header bar).

<!-- end -->

<!-- only: kde windows -->

The menu bar has Machine, View, FujiNet and Help. The Machine menu has the
cartridge items, Pause, Reset, Reset to CONFIG, Preferences and Quit (Exit on
Windows).

<!-- end -->

<!-- only: macos -->

On a Mac, Preferences (called Settings) and Quit are in the app menu, and
the cartridge items are in the Machine menu. Controllers and the Debugger are
also ⌘J and ⌘D.

<!-- end -->

The status shows the cartridge and FujiNet's state:

| Status | Means |
|---|---|
| connected | The cartridge is talking to FujiNet. |
| link down | FujiNet is off or not answering yet; CONFIG tries again. |
| loading 42% | FujiNet is sending a program to the cartridge. |
| game running; mailbox closed | A game that does not use FujiNet is running. |

Messages (a cartridge opened, a gamepad plugged in, FujiNet restarted) show
for a few seconds over the picture.

## Reset and power

- **Reset** presses the console's RESET button (<kbd>F1</kbd>). It holds the
  processor's reset line only, as the real button does, so a running FujiNet
  app starts again from the cartridge's current image.
- **Reset to CONFIG** (<kbd>Ctrl+R</kbd>, or <kbd>Escape</kbd>) is a power cycle:
  it ejects an opened cartridge and boots CONFIG, exactly as at power-on.
  The machine you chose in Preferences takes effect here.
- **Eject Cartridge** does the same; a FujiNet cartridge is always in the
  slot, and CONFIG is its own program.
- **Pause** stops the console and its sound; the Debugger's Stop is
  separate.

## The picture

The console draws 192 × 262 pixels. A television shows them 4:3; **TV
Aspect** switches between that and square pixels. The NTSC machine runs at
60 frames a second and the PAL clone at 50.

**Smooth Scaling** blends the pixels when the picture is stretched, and
leaves them sharp when it is off.

# Controllers and keys

The Arcadia 2001 has two hand controllers and a panel. Each controller has a
**12-key keypad** (1 to 9, CLEAR, 0 and ENTER), a **disc** that steers like
a joystick, and a **side fire button**. The panel has **START**, **OPTION**
and **SELECT**, and the console has a red **RESET**. The fire button shares
the keypad's `2` wire, as on the hardware. The **left controller is Player
1** and the **right controller is Player 2**.

Any number of keys can be held at once.

## Default keys

Keys go by where they are on the keyboard, whatever your layout.

| Does | Player 1 | Player 2 |
|---|---|---|
| Keypad 1 to 9 | <kbd>1</kbd> <kbd>2</kbd> <kbd>3</kbd> / <kbd>4</kbd> <kbd>5</kbd> <kbd>6</kbd> / <kbd>7</kbd> <kbd>8</kbd> <kbd>9</kbd> | <kbd>Q W E</kbd> / <kbd>A S D</kbd> / <kbd>Z X C</kbd> |
| Keypad CLEAR, 0, ENTER | <kbd>Backspace</kbd>, <kbd>0</kbd>, <kbd>Return</kbd> | <kbd>V</kbd>, <kbd>B</kbd>, <kbd>N</kbd> |
| Disc up, down, left, right | The arrow keys | <kbd>Keypad 8</kbd>, <kbd>Keypad 2</kbd>, <kbd>Keypad 4</kbd>, <kbd>Keypad 6</kbd> |
| Fire | <kbd>Right Ctrl</kbd> | <kbd>Keypad 0</kbd> |

| Console | Key |
|---|---|
| START | <kbd>F2</kbd> |
| OPTION | <kbd>F3</kbd> |
| SELECT | <kbd>F4</kbd> |
| RESET | <kbd>F1</kbd> |
| Reset to CONFIG | <kbd>Escape</kbd> |

A key does one thing: giving a key to a new job takes it from the old one.

The app keeps these keys for itself:

| Shortcut | Does |
|---|---|
| <kbd>Ctrl+O</kbd> | Open Cartridge |
| <kbd>Ctrl+R</kbd> | Reset to CONFIG |
| <kbd>Ctrl+,</kbd> | Preferences |
| <kbd>Ctrl+Q</kbd> | Quit |
| <kbd>Ctrl+Shift+H</kbd> | Help |
| <kbd>F9</kbd> | Controllers |
| <kbd>F11</kbd> | Fullscreen |
| <kbd>F12</kbd> | Debugger |

<!-- only: gnome kde windows -->

Keys with <kbd>Ctrl</kbd> never reach the console.

<!-- end -->

<!-- only: macos -->

Keys with ⌘ never reach the console.

<!-- end -->

<!-- only: linux -->

Key combinations with the Super (Windows) key are the desktop's.

<!-- end -->

## Gamepads

Gamepads are picked up as they are plugged in, and can come and go at any
time. The first is Player 1, the second Player 2; Preferences ▸ Input can
fix which is which, and a pad plugged back in gets its player back.

| Gamepad | Does |
|---|---|
| D-pad | The disc's four directions |
| Left stick | The disc, in proportion to how far it is pushed |
| South (A) | Fire |
| Start | START |
| West (X) | OPTION |
| Back | SELECT |

The disc is two potentiometers, so the left stick steers it smoothly. With
**Analog sticks steer too** off, the stick is an eight-way switch like the
D-pad. A gamepad with no standard layout (a plain USB adapter) uses its first
stick or hat and its buttons, which can be given jobs in the same way.

## Changing the controls

The controls are changed in the Controllers window (<kbd>F9</kbd>): click
**Map**, then click a control on the picture (40 in all: both discs'
four directions and fire, both keypads, START, OPTION, SELECT, RESET and the
app's own actions), then press the key or the gamepad button to give it.
The control's old key or button is taken from it. **Defaults** puts every
control back, and <kbd>Escape</kbd> leaves Map. The map is kept in the
settings, and only what differs from the defaults is written.

<!-- context: controllers -->
# Controllers window

The Controllers window (<kbd>F9</kbd>) draws the console as a player sees
it: two hand controllers with their 12-key keypads, the disc and the side
fire button, and the panel's START, OPTION and SELECT with the red RESET. It
floats on its own at a fixed size. The controls that are held light up in the
accent colour, whoever holds them: the keyboard, a gamepad or the mouse.

| With the mouse | Does |
|---|---|
| Click a key | Presses it |
| Drag the disc | Steers it; the pots follow the pointer, in proportion |
| Click the fire button | Presses it |
| Click a panel button | Presses it |

**Map** and **Defaults** change the controls (see
[Changing the controls](#changing-the-controls)).

Switching to another window lets go of everything the mouse holds. Keys
typed in the Controllers window go to the console.

# Machines

Preferences ▸ Machine picks the machine:

- **Emerson Arcadia 2001 (NTSC)**: the console as MAME has it. The 2650 runs
  at 0.895 MHz and draws 262 lines at 60 frames a second.
- **Arcadia 2001 clone (PAL)**: the Hanimex and Leisure-Vision class of
  clone, at 50 frames a second and 312 lines. It is **not verified against
  MAME**, which runs every clone at NTSC.

A new machine takes effect at the next power cycle (Reset to CONFIG). The
16-key Palladium Video-Computer-Game is not emulated, as MAME marks it not
working.

# No BIOS, no ROMs

The Arcadia 2001 has no BIOS, and its character set is part of the UVI
source, so there is nothing to import. CONFIG and every FujiNet app run as
they are, and no copyrighted ROM is part of the app.

# Cartridges

A cartridge is a file: a `.bin`, a `.rom` or an `.arc` of up to 8 KB, or a
`.zip` that holds one. The app works out what it is from its contents.

- **A FujiNet app** is an image that claims the mailbox. It runs in the
  FujiNet cartridge: it can talk to the network.
- **A game** is any other image. It gets the mapper a real cartridge would
  have (standard 4 KB or 8 KB, or Golf), and FujiNet is not reachable until
  the next power cycle.

## Open Cartridge

**Open Cartridge…** (<kbd>Ctrl+O</kbd>) powers the console off and on with
the cartridge in the slot. The cartridge is remembered and starts again
next time; **Eject Cartridge** or **Reset to CONFIG** forgets it. Dropping a
cartridge file on the window opens it. A file the console cannot take is
refused with the reason, and what was running goes on.

<!-- context: import-sd -->
## Import Cartridge to SD

**Import Cartridge to SD…** copies a cartridge into FujiNet's SD folder, so
CONFIG can list it and boot it through the cartridge (a zip of one is
unpacked to `<name>.bin`). The FujiNet cartridge refuses what it cannot
serve, and the message says why. Without the FujiNet runtime there is no SD
folder to copy to.

# FujiNet

The FujiNet is FujiNet's own firmware, running inside the app, and the
cartridge in the slot is its connection to the console. On the real console
the FujiNet cartridge is a board on the cartridge connector; here it is
emulated, and talks to FujiNet over a local network socket. FujiNet
listens; the cartridge dials in, and keeps trying again if the link drops.

The cartridge has an 8 KB program window, up to 70 KB of paged app image
(APPBANK), 1 KB of cart RAM, a 60 Hz tick and the mailbox at $2B00 to $2FFF,
with CONFIG baked in.

## CONFIG

CONFIG is FujiNet's program for choosing what to run:

- **Hosts** are where programs come from: the SD folder, and servers on the
  network.
- Pick a host, a program on it, and start it. FujiNet sends the image to the
  cartridge, which swaps it in and restarts the console on it. A FujiNet
  app can then ask FujiNet for more (pages, files, the network).

## The FujiNet menu

- **FujiNet Web UI** opens FujiNet's settings page in your browser, at
  `http://127.0.0.1:11523/`. Only this computer can reach it.
- **Console Log** shows what FujiNet prints. See
  [The console log](#the-console-log).

<!-- context: fujilog -->
## The console log

The FujiNet Console Log window shows FujiNet's own messages as they come,
following the end while you are at the bottom. It is also kept in
`fujinet-console.log` in the app's `fujinet` folder.

## Turning it off

Preferences ▸ FujiNet ▸ **Enable FujiNet** turns the FujiNet off. The
cartridge then reports *link down*, CONFIG runs without hosts, and nothing
is dialled. Games are not affected.

## An external FujiNet

Preferences ▸ FujiNet ▸ **Use external FujiNet** skips the built-in FujiNet
and dials another FujiNet instance at the address you give, for example
`192.168.1.20:11522`. Leave the field empty for `127.0.0.1:11522`. Set that instance's `[BOIP]` section to listen there
(`enabled=1`, `host`, `port`). The cartridge dials out, so the other FujiNet
can start before or after it. FujiNet Web UI and Console Log belong to the
built-in FujiNet, so they say the FujiNet is external; use the other
instance's own. The Debugger's FujiNet tab still shows the traffic.

## Behind the scenes

- The cartridge and FujiNet talk over TCP port 11522 on 127.0.0.1 (the
  BoIP port), and the web UI is on port 11523. Nothing from outside can
  reach either.
- The cartridge's mailbox service runs on its own thread, as it runs on the
  cartridge's second core, so a slow network transaction never stalls the
  picture.
- FujiNet's settings are `fnconfig.ini` in the app's `fujinet` folder. It is
  written once and then left for FujiNet and you.
- A game that does not claim the mailbox runs with FujiNet unreachable until
  the next power cycle.

# Preferences

Preferences (<kbd>Ctrl+,</kbd>) set up the console. The machine, audio,
gamepad and FujiNet settings apply with a power cycle: the app does it when
Preferences closes. The picture, the volume and the sticks apply at once.
The settings are one file, shared by the GNOME and KDE apps on the computer.

<!-- only: macos -->

On a Mac this window is called **Settings** (in the app menu), and its
pages are tabs.

<!-- end -->

<!-- context: prefs/Machine -->
## Machine settings

- **Machine**: which console (see [Machines](#machines)).

<!-- context: prefs/Display -->
## Display and audio settings

- **Aspect**: *Television (4:3)* or *Square pixels* (setting `aspect`).
- **Smooth scaling**: blend the pixels when the picture is stretched
  (setting `smooth`).
- **Enable audio**: open the system audio device.
- **Volume**: the output level, 0 to 100.

<!-- only: kde windows macos -->

On KDE, Windows and the Mac these are two pages, Display and Audio.

<!-- end -->

<!-- context: prefs/Input -->
## Input settings

- **Enable gamepads**: poll USB and Bluetooth gamepads.
- **Analog sticks steer too**: the left stick moves the disc in proportion;
  off, it acts as an eight-way switch like the D-pad (setting
  `analog_joystick`).
- **Gamepads**: each one plugged in, and which player it drives. A new
  gamepad drives Player 1, the next Player 2; choose *Player 1* or
  *Player 2* to fix it, or *Automatic*.

The keys and buttons are changed in the Controllers window (see
[Changing the controls](#changing-the-controls)).

<!-- context: prefs/FujiNet -->
## FujiNet settings

- **Enable FujiNet**: run the built-in FujiNet.
- **Use external FujiNet**: dial another instance instead.
- **Address**: its `host:port`, or just a port (see
  [An external FujiNet](#an-external-fujinet)).
- **Web UI**: the address of the built-in FujiNet's settings page.

## The settings file

The settings file is `settings.ini` (see [Files and folders](#files-and-folders)).
The keys the Preferences pages set:

| Key | Is |
|---|---|
| `machine` | 0 the Arcadia 2001 (NTSC), 1 the PAL clone |
| `analog_joystick` | 1 (the default) or 0 |
| `enable_fujinet` | 1 or 0 |
| `fujinet_external` | 1 or 0 |
| `fujinet_external_addr` | `host:port` |
| `enable_audio` | 1 or 0 |
| `enable_gamepad` | 1 or 0 |
| `volume` | 0 to 100 |
| `aspect` | 0 the television's 4:3, 1 square pixels |
| `smooth` | 1 blends the pixels when stretched, 0 keeps them sharp |
| `cart` | the opened cartridge, empty for CONFIG |

# Debugger

The Debugger (<kbd>F12</kbd>) stops the console and shows what it is doing.
It has eight tabs; closing it lets the console go on as it was.

| Button | Does |
|---|---|
| Stop or Run (<kbd>F5</kbd>) | Stops before the next instruction, or runs on |
| Step (<kbd>F7</kbd>) | One instruction |
| Step Over (<kbd>F8</kbd>) | Over a branch to subroutine, to the instruction after it |
| Step Out (<kbd>Shift+F8</kbd>) | On until the routine returns (RETC or RETE) |
| Scanline+1 | On to the next line of the picture |
| Frame+1 | On to the next frame |
| Symbols | Loads labels (see [Disassembly](#disassembly)) |
| Save | Saves the disassembly, the mailbox, the UVI, the font, the cartridge, the screen or the registers to a file |
| Help | Opens the topic of the tab that shows |

Run to an address with the Prompt's `runto`.

Beside them, the Debugger says why the console stopped: a step, a
breakpoint and its address, a data read or write.

<!-- context: debugger/prompt -->
## Prompt

The Prompt takes commands, with completion on <kbd>Tab</kbd> for commands,
registers and labels. Numbers are `$0400`, `0x400`, `400h`, `1024` or
`%1010`. `help` lists the commands.

| Command | Does |
|---|---|
| `step` | Runs `step n` instructions |
| `over`, `out` | Step over, step out |
| `frame`, `scanline` | Run `n` frames or lines |
| `run`, `stop` | Run on, or stop |
| `runto` | Runs to an address |
| `break` | An execute breakpoint, `break addr[-end] [cond]` |
| `bpr`, `bpw` | Data reads or writes of an address range |
| `delete`, `enable`, `disable` | A breakpoint by id |
| `clear`, `breaks` | Clear all, or list them |
| `print` | Evaluates an expression |
| `regs`, `set` | The registers; `set reg expr` |
| `mem` | Memory in a space: `mem [space] addr [len]` |
| `poke` | Writes bytes to a space |
| `uvi`, `objects`, `screen` | The 2637's registers, the four objects, the character screen |
| `io`, `keys` | The console's inputs; the keypads and discs |
| `cart`, `mailbox` | The cartridge, and FujiNet's mailbox page |
| `label`, `labels` | Name an address; load a symbol file |
| `disasm` | Disassembles `disasm [addr] [n]` |
| `trace` | The instruction trace: `trace [on\|off\|clear\|n]` |
| `save` | Saves `dis`, `mailbox`, `uvi`, `font`, `cart`, `screen` or `regs` to a file |

The spaces are `prog` (the CPU's view, with the UVI at $1800 to $1AFF),
`uvi` and `cart`. An expression has the registers (`r0` to `r3`, `r1'` to
`r3'` for the other set, `pc iar psu psl sp cc c com wc rs idc ovf f ii s`),
the byte at `[addr]`, `p[addr]`, `u[off]` and `c[off]`, labels, and `vpos`,
`hpos`, `frame` and `cycles`. In a breakpoint condition `value` and `addr` are
the access. The operators are C's.

<!-- context: debugger/cpu -->
## CPU and RAM

The registers: R0, R1 to R3 in both register banks, PSU and PSL decoded flag
by flag, the 15-bit IAR and the 8-deep return stack. Click a value to edit
it while stopped. The hex view reads the program space, the UVI and the
cartridge, and can edit them.

<!-- context: debugger/disassembly -->
## Disassembly

The 2650 disassembled at the PC, with the opcode bytes, labels and the
breakpoints in the margin. Double-click a line for an execute breakpoint.
Follow PC keeps the current line in view. **Symbols** loads the labels of a
listing, or a plain list of addresses and names.

<!-- context: debugger/video -->
## Video

The 2637 UVI draws a background and four objects over a character screen.
The Video tab shows:

- The rendered display: the live frame, re-rendered from the registers, or
  the frame in progress up to the beam, with the background and the four
  objects switchable and boxed.
- Every object with its position, size, glyph and colour, and a rendered
  bitmap.
- The 16 × 13 or 16 × 26 character screen with its colour bits, and the
  user-defined characters.
- Every register decoded and editable, and the collisions.
- The font and palette sheets.

<!-- context: debugger/io -->
## Sound and I/O

The UVI's sound as the CPU last wrote it, and the console's inputs: the
keypads, the discs and the panel, decoded as the CPU sees them.

<!-- context: debugger/breakpoints -->
## Breakpoints

The list of breakpoints with their ids, kinds, ranges, conditions and hit
counts. A breakpoint can be an execute, or a data read or write, each with
an optional condition. **Add**, **Remove**, and the check box to turn one
off.

<!-- context: debugger/cart -->
## Cart

The cartridge in the slot, and the FujiNet mailbox at $2B00 to $2FFF: the
registers, the live image and the one staged for the next swap, and the
events.

<!-- context: debugger/fujinet -->
## FujiNet packets

Everything between the cartridge and FujiNet as FujiBus packets, from the
moment the app starts: the direction, the device, the command and the
length. A repeat of the same exchange shows once with a count. **Clear**
empties the list.

# Files and folders

The app keeps its settings in one file, and everything else in its data
folder:

<!-- only: linux -->

On Linux, the settings are `~/.config/fujinet-go-arcadia/settings.ini` and
the data is in `~/.local/share/fujinet-go-arcadia` (or under
`$XDG_CONFIG_HOME` and `$XDG_DATA_HOME`). The GNOME and KDE apps share them.
As a Flatpak, they are under `~/.var/app/`, in the app's own folder.

<!-- end -->

<!-- only: windows -->

On Windows, the settings are `%APPDATA%\fujinet-go-arcadia\settings.ini` and
the data is in `%LOCALAPPDATA%\fujinet-go-arcadia`.

<!-- end -->

<!-- only: macos -->

On a Mac, the settings are `~/.config/fujinet-go-arcadia/settings.ini` and
the data is in `~/.local/share/fujinet-go-arcadia`.

<!-- end -->

In the data folder:

| Folder | Has |
|---|---|
| `carts` | cartridges dropped on the window |
| `fujinet` | `fnconfig.ini`, the SD folder (`SD`), and the console log |

# Troubleshooting

- **link down**: FujiNet is off, not started yet, or the external address
  is wrong. CONFIG tries again; check Preferences ▸ FujiNet.
- **The cartridge was refused**: the message says why: too big, empty, or
  not a cartridge image. A zip must hold exactly one image.
- **Import Cartridge to SD refused a game**: the FujiNet cartridge cannot
  serve it; open it with Open Cartridge instead.
- **Keys don't reach the console**: a dialog or another window is in front,
  or the key is one the app keeps (see
  [Controllers and keys](#controllers-and-keys)).
- **A gamepad does nothing**: it may not be on the player you expect (see
  [Gamepads](#gamepads)), or its buttons may be given other jobs.
- **No sound**: the audio device could not be opened, or Pause is on.
- **A port is taken**: another program is using 11522 or 11523. Close the
  other FujiNet, or point the app at it with **Use external FujiNet**.

# About and licences

FujiNet Go Arcadia 2001 is free software under the GNU General Public
License, version 3 or later. Copyright © 2026 Thomas Cherryhomes.

- **MAME** (the Emerson Arcadia 2001 driver, the Signetics 2650 processor and
  the 2637 UVI) is what the console is modelled on.
- **The FujiNet firmware** (GPL-3.0-or-later) runs inside the app, and the
  cartridge's own sources are the firmware's.
- No copyrighted ROM is part of the app. "Emerson" and "Arcadia 2001" are
  trademarks of their owners, used to name the machine emulated; this
  project is not affiliated with them.

[fujinet.online](https://fujinet.online/) ·
[Report an issue](https://github.com/FujiNetWIFI/fujinet-go-arcadia-desktop/issues)
