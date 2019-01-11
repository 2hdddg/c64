project('c64', 'c')
src = [
    'emulation/cpu.c',
    'emulation/cpu_instr.c',
    'emulation/cpu_port.c',
    'emulation/cia.c',
    'emulation/cia_timer.c',
    'emulation/cia1.c',
    'emulation/cia2.c',
    'emulation/keyboard.c',
    'emulation/mem.c',
    'emulation/pla.c',
    'emulation/sid.c',
    'emulation/vic.c',
    'emulation/vic_palette.c',

    'basic.c',

    'main.c',
    'commandline.c',
    'command.c',
    'trace.c',

    'sdl_c64.c',
    'snapshot.c',

    'ncurses_c64.c',
]
inc = include_directories('emulation')

sdl_dep = dependency('sdl2')

executable('c64', src, dependencies: [sdl_dep], link_args: [
    '-lmenu', '-lncurses', '-lpng', '-lreadline'],
    include_directories: inc)

#subdir('test')
