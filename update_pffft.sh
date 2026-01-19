cd third_party
rm -rf pffft.wasm
git clone --depth=1 https://github.com/JorenSix/pffft.wasm.git
cd pffft.wasm
mkdir -p bin/pffft/
mkdir -p emcache
cd emcache
export EM_CACHE=$(pwd)
cd ../src
AGGRESSIVE_MATH="-ffast-math -fno-signed-zeros -fno-trapping-math -funsafe-math-optimizations -fassociative-math -freciprocal-math -ffinite-math-only -fno-strict-aliasing"
emcc -c pffft.c \
        -O3 \
        -msimd128 -msse4.2 \
        -Wall \
        -W \
        -fPIC \
        -o ../bin/pffft/pffft.simd.o \
        -ftree-vectorize -fvectorize -ffast-math -funroll-loops -finline-functions \
        -fno-exceptions $AGGRESSIVE_MATH
ar rcs ../bin/pffft/libpffft.a ../bin/pffft/pffft.simd.o
cd ..
mkdir -p ../../standalone/builds/wasm_full/pffft
mkdir -p ../../standalone/builds/wasm_full/pffft/include/
mkdir -p ../../standalone/builds/wasm_full/pffft/lib/
cp bin/pffft/libpffft.a ../../standalone/builds/wasm_full/pffft/lib/
cp src/*.h ../../standalone/builds/wasm_full/pffft/include/