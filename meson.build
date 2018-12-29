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
    'trace.c',

    'sdl_c64.c',

    'ncurses_c64.c',
]

sdl_dep = dependency('sdl2')

executable('c64', src, dependencies: [sdl_dep], link_args: [
    '-lmenu', '-lncurses'])

# Test suites
shared_library('suite_cpu', [
    'test/suite_cpu.c', 'cpu.c', 'cpu_instr.c', 'trace.c'])
shared_library('suite_cpu_examples', [
    'test/suite_cpu_examples.c', 'cpu.c', 'cpu_instr.c', 'trace.c'])
shared_library('suite_mem', [
    'test/suite_mem.c', 'mem.c'])
shared_library('suite_cpu_port', [
    'test/suite_cpu_port.c', 'cpu_port.c', 'mem.c'])
shared_library('suite_cia1', [
    'test/suite_cia1.c', 'cia1.c', 'cia.c', 'keyboard.c',
    'cia_timer.c', 'trace.c'])
shared_library('suite_cia_timer', [
    'test/suite_cia_timer.c', 'cia_timer.c'])
shared_library('suite_keyboard', [
    'test/suite_keyboard.c', 'keyboard.c', 'trace.c'])
shared_library('suite_vic', [
    'test/suite_vic.c', 'vic.c', 'trace.c'])

