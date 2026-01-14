# WebVial
<img src=webvial.webp width=400>

WebVial is a WIP web port of the Vital source release.\
Try it here: [https://zxmushroom63.github.io/vital](https://zxmushroom63.github.io/vital)



### Compiling & Testing
This guide is for NixOS, it makes dependency management very easy. See [shell.nix](shell.nix) for a list of required packages. Also ensure you have a mainstream web browser available. (chromium or firefox is fine)
1. `git clone --depth=1 https://github.com/ZXMushroom63/vital.git`
    - Clone the repository locally
2. `cd vital`
    - Enter the repository directory
3. `nix-shell`
    - Setup the environment. This installs required packages and adds Emscripten to your $PATH.
4. `cd freetypesetup`
    - Enter the `freetypesetup` directory, where we will cross compile freetype2 to WASM
5. `./init.sh`
    - Clone the freetype-wasm repo and download dependencies
6. `./build.sh`
    - Build brotli, then freetype, and move their binaries to the correct locations
7. `cd ..`
    - Go back to the main directory, `vital`
8. `make wasm_full`
    - Build the .wasm and .js files, and combine with templates. This will output to `docs/` directory (naming scheme for gh pages)
9. `cd docs`
    - Enter the `docs` directory
10. `(&>/dev/null python -m http.server 3000 &)`
    - Start a HTTP server in the background
11. `firefox --new-window "http://localhost:3000"`
    - This will open the WebVial launcher in the browser
    - For chromium: `chromium --new-window "http://localhost:3000"`
    - For chrome: `chrome --new-window "http://localhost:3000"`
12. Leave the default settings, and click the start button
    - <img src=startbtn.png height=24 alt="screenshot of start button">
13. After a bit of lag (up to 30s), the synth should start! Click around a bit i guess