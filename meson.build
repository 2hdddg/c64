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
    'emulation/basic.c',
    'emulation/c64.c',

    'infrastructure/commandline.c',
    'infrastructure/command.c',
    'infrastructure/trace.c',

    'ui/sdl_c64.c',
    'ui/ncurses_c64.c',
    'ui/snapshot.c',

    'main.c',
]
inc = include_directories('emulation', 'infrastructure', 'ui')

sdl_dep = dependency('sdl2')

executable('c64', src, dependencies: [sdl_dep], link_args: [
    '-lmenu', '-lncurses', '-lpng', '-lreadline'],
    include_directories: inc)

subdir('test')
