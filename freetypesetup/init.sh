#!/bin/sh
cd ..
rm -rf third_party/freetype-wasm
cd third_party
git clone https://github.com/Ciantic/freetype-wasm.git --depth=1
cp ../freetypesetup/patches/*.sh ./freetype-wasm/
cd freetype-wasm
./deps.sh
