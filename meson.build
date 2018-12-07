project('c64', 'c')
src = [
    'main.c',
    'mem.c',
    'cpu.c',
    'cpu_instr.c',
    'cpu_port.c',
    'cia_timer.c',
    'cia1.c',
    'cia2.c',
    'vic.c',
    'sid.c',
    'pla.c']
thread_dep = dependency('threads')
executable('c64', src, dependencies: [thread_dep])

# Test suites
shared_library('suite_cpu', [
    'test/suite_cpu.c', 'cpu.c', 'cpu_instr.c'])
shared_library('suite_cpu_examples', [
    'test/suite_cpu_examples.c', 'cpu.c', 'cpu_instr.c'])
shared_library('suite_mem', [
    'test/suite_mem.c', 'mem.c'])
shared_library('suite_cpu_port', [
    'test/suite_cpu_port.c', 'cpu_port.c', 'mem.c'])
shared_library('suite_cia_timer', [
    'test/suite_cia_timer.c', 'cia_timer.c'])

