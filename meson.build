project('c64', 'c')
src = [
    'basic.c',
    'cpu.c',
    'cpu_instr.c',
    'cpu_port.c',
    'cia.c',
    'cia_timer.c',
    'cia1.c',
    'cia2.c',
    'keyboard.c',
    'mem.c',
    'pla.c',
    'sid.c',
    'vic.c',
    'vic_palette.c',

    'main.c',
    'commandline.c',
    'command.c',
    'trace.c',

    'sdl_c64.c',
    'snapshot.c',

    'ncurses_c64.c',
]

sdl_dep = dependency('sdl2')

executable('c64', src, dependencies: [sdl_dep], link_args: [
    '-lmenu', '-lncurses', '-lpng', '-lreadline'])

subdir('test')
