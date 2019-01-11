inc = include_directories('..')

shared_library('suite_cpu', [
    'suite_cpu.c',
    '../cpu.c', '../cpu_instr.c', '../trace.c'],
    include_directories: inc)
shared_library('suite_cpu_examples', [
    'suite_cpu_examples.c',
    '../cpu.c', '../cpu_instr.c', '../trace.c'],
    include_directories: inc)
shared_library('suite_mem', [
    'suite_mem.c',
    '../mem.c'],
    include_directories: inc)
shared_library('suite_cpu_port', [
    'suite_cpu_port.c',
    '../cpu_port.c', '../mem.c'],
    include_directories: inc)
shared_library('suite_pla', [
    'suite_pla.c',
    '../pla.c', '../mem.c', '../trace.c' ],
    include_directories: inc)
shared_library('suite_cia1', [
    'suite_cia1.c',
    '../cia1.c', '../cia.c', '../keyboard.c', '../cia_timer.c',
    '../trace.c'],
    include_directories: inc)
shared_library('suite_cia_timer', [
    'suite_cia_timer.c',
    '../cia_timer.c'],
    include_directories: inc)
shared_library('suite_keyboard', [
    'suite_keyboard.c',
    '../keyboard.c', '../trace.c'],
    include_directories: inc)
shared_library('suite_vic', [
    'suite_vic.c',
    '../vic.c', '../vic_palette.c', '../trace.c', '../snapshot.c'],
    link_args: ['-lpng'],
    include_directories: inc)

