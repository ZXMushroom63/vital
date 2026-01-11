cd third_party
rm -rf kissfft
git clone --depth=1 https://github.com/mborgerding/kissfft.git
cd kissfft
mkdir -p bin
mkdir -p emcache
mkdir -p install
mkdir -p include
cd emcache
export EM_CACHE=$(pwd)
cd ..
cd install
export CMAKE_INSTALL_PREFIX=$(pwd)
export ABS_LIBDIR=$(pwd)
cd ..
cd include
export CMAKE_INSTALL_PREFIX=$(pwd)
export ABS_PKGINCLUDEDIR=$(pwd)
cd ..
echo $(pwd)
export KISSFFT_OPENMP=0
export KISSFFT_STATIC=1
export KISSFFT_TOOLS=0
export KISSFFT_USE_ALLOCA=0
export KISSFFT_DATATYPE="float"
emmake cmake -B bin . -DKISSFFT_TEST=OFF -DKISSFFT_OPENMP=OFF -DKISSFFT_STATIC=ON -DKISSFFT_DATATYPE=float -DKISSFFT_TOOLS=OFF
emmake make
emmake make install ABS_LIBDIR=$ABS_LIBDIR ABS_PKGINCLUDEDIR=$ABS_PKGINCLUDEDIR
cd ../../
rm -rf src/common/kissfft/*
mkdir -p src/common/kissfft/
cp third_party/kissfft/include/* src/common/kissfft/
echo "emcache" >> third_party/kissfft/.gitignore
echo ".o" >> third_party/kissfft/.gitignore
echo "test" >> third_party/kissfft/.gitignore
echo "bin" >> third_party/kissfft/.gitignore
echo "cmake" >> third_party/kissfft/.gitignore
echo "tools" >> third_party/kissfft/.gitignore
