#!/nix/store/cdaifv92znxy5ai4sawricjl0p5b9sgf-python3-3.13.11/bin/python3

import sys

print('emscripten sdl2-config called with', ' '.join(sys.argv), file=sys.stderr)

args = sys.argv[1:]

if '--cflags' in args or '--libs' in args:
  print('-sUSE_SDL=2')
elif '--version' in args:
  print('2.0.10')

