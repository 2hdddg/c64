project('c64', 'c')
src = ['main.c', 'mem.c', 'cpu.c']
executable('c64', src)

# Test suites
shared_library('suite_cpu', ['test/suite_cpu.c', 'cpu.c'])
shared_library('suite_examples', [
    'test/suite_examples.c', 'cpu.c', 'mem.c'])

