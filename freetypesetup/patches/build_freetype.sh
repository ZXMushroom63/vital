# NOTE FUTURE HUMANS:
# EM_CACHE MAY NEED TO BE MANUALLY SET

cd brotli
JMP=$(pwd)
BROTLIF=$(pwd)"/libbrotlidec.a"
#cd CMakeFiles/brotlidec.dir/c/dec
cd c/include/
BROTLIINC=$(pwd)
cd $JMP
cd ..
echo $BROTLIF
echo $BROTLIINC

mkdir -p freetype2/build
mkdir -p freetype2/cache
cd freetype2/cache
export EM_CACHE=$(pwd)
export EMCC_CACHE=$(pwd)
echo $EMCC_CACHE
EM_CACHE_T=$(pwd)
cd ../build || exit
export EM_CACHE=$EM_CACHE_T
export CPPFLAGS="-sSHARED_MEMORY=1 -Wl,--shared-memory -O3 -g0 -ftree-vectorize -fvisibility=hidden -DNDEBUG=1 --closure 1 -sEVAL_CTORS -fno-rtti -fno-exceptions -msimd128 -mavx2 --cache ./cache"
emcmake cmake \
    -D BROTLIDEC_LIBRARIES="$BROTLIF" \
    -D BROTLIDEC_INCLUDE_DIRS="$BROTLIINC" \
    -D FT_DISABLE_ZLIB=TRUE \
    -D FT_DISABLE_BZIP2=TRUE \
    -D FT_DISABLE_PNG=TRUE \
    -D FT_DISABLE_HARFBUZZ=TRUE \
    -D FT_REQUIRE_BROTLI=TRUE \
    -D CMAKE_C_FLAGS="$CPPFLAGS" \
    ..
export EM_CACHE=$EM_CACHE_T
emmake make #VERBOSE=1
export EM_CACHE=$EM_CACHE_T
#emmake make install
rm -f ../../../../standalone/builds/wasm_full/freetype/*.a
cp *.a ../../../../standalone/builds/wasm_full/freetype/