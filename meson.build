project('c64', 'c')
src = [
    'main.c',
    'mem.c',
    'cpu.c',
    'cpu_instr.c',
    'cpu_port.c',
    'pla.c']
executable('c64', src)

# Test suites
shared_library('suite_cpu', [
    'test/suite_cpu.c', 'cpu.c', 'cpu_instr.c'])
shared_library('suite_cpu_examples', [
    'test/suite_cpu_examples.c', 'cpu.c', 'cpu_instr.c'])
shared_library('suite_mem', [
    'test/suite_mem.c', 'mem.c'])
shared_library('cpu_port', [
    'test/suite_cpu_port.c', 'cpu_port.c', 'mem.c'])

