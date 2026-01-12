# NOTE FUTURE HUMANS:
# EM_CACHE MAY NEED TO BE MANUALLY SET

mkdir -p brotli/cache

cd brotli/cache
EM_CACHE_T=$(pwd)
export EM_CACHE=$(pwd)
cd ..
export EM_CACHE=$EM_CACHE_T
emcmake cmake -DBROTLI_EMSCRIPTEN=1 -DBUILD_SHARED_LIBS=OFF
export EM_CACHE=$EM_CACHE_T
emmake make
export EM_CACHE=$EM_CACHE_T

rm -f ../../../standalone/builds/wasm_full/brotli/*.a
cp *.a ../../../standalone/builds/wasm_full/brotli/