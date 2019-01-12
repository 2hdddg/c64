inc = include_directories('..', '../emulation',
                          '../infrastructure', '../ui')

shared_library('suite_cpu', [
    'suite_cpu.c',
    '../emulation/cpu.c', '../emulation/cpu_instr.c',
    '../infrastructure/trace.c'],
    include_directories: inc)
shared_library('suite_cpu_examples', [
    'suite_cpu_examples.c',
    '../emulation/cpu.c', '../emulation/cpu_instr.c',
    '../infrastructure/trace.c'],
    include_directories: inc)
shared_library('suite_mem', [
    'suite_mem.c',
    '../emulation/mem.c'],
    include_directories: inc)
shared_library('suite_cpu_port', [
    'suite_cpu_port.c',
    '../emulation/cpu_port.c', '../emulation/mem.c'],
    include_directories: inc)
shared_library('suite_pla', [
    'suite_pla.c',
    '../emulation/pla.c', '../emulation/mem.c',
    '../infrastructure/trace.c' ],
    include_directories: inc)
shared_library('suite_cia1', [
    'suite_cia1.c',
    '../emulation/cia1.c', '../emulation/cia.c',
    '../emulation/keyboard.c', '../emulation/cia_timer.c',
    '../infrastructure/trace.c'],
    include_directories: inc)
shared_library('suite_cia_timer', [
    'suite_cia_timer.c',
    '../emulation/cia_timer.c'],
    include_directories: inc)
shared_library('suite_keyboard', [
    'suite_keyboard.c',
    '../emulation/keyboard.c',
    '../infrastructure/trace.c'],
    include_directories: inc)
shared_library('suite_vic', [
    'suite_vic.c',
    '../emulation/vic.c', '../emulation/vic_palette.c', 
    '../infrastructure/trace.c', '../ui/snapshot.c'],
    link_args: ['-lpng'],
    include_directories: inc)

