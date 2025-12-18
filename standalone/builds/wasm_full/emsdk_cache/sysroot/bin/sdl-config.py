#!/nix/store/jd20rkmqmkfkcvk2wl2lmzz7acq4svlr-python3-3.12.12/bin/python3

import sys

print('emscripten sdl-config called with', ' '.join(sys.argv), file=sys.stderr)

args = sys.argv[1:]

if args[0] == '--cflags':
  print('')
elif '--version' in args:
  print('1.3.0')

