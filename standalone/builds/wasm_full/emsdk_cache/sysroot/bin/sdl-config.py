#!/nix/store/cdaifv92znxy5ai4sawricjl0p5b9sgf-python3-3.13.11/bin/python3

import sys

print('emscripten sdl-config called with', ' '.join(sys.argv), file=sys.stderr)

args = sys.argv[1:]

if args[0] == '--cflags':
  print('')
elif '--version' in args:
  print('1.3.0')

