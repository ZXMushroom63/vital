# WebVial
<img src=webvial.webp width=400>

WebVial is a WIP web port of the Vital source release.\
Try it here: [https://zxmushroom63.github.io/vital](https://zxmushroom63.github.io/vital)

### Features
- WebMIDI support
- It is a versatile synth
- Moderately performant. (still working on that)

### Compiling & Testing
This guide is for NixOS, it makes dependency management very easy. See [shell.nix](shell.nix) for a list of required packages. Also ensure you have a mainstream web browser available. (chromium or firefox is fine)
1. `git clone --depth=1 https://github.com/ZXMushroom63/vital.git`
    - Clone the repository locally
2. `cd vital`
    - Enter the repository directory
3. `nix-shell`
    - Setup the environment. This installs required packages and adds Emscripten to your $PATH.
4. `./update_pffft.sh`
    - Download and compile pffft (main fourier transform library, SIMD accelerated)
5. `./update_kissfft.sh`
    - Download and compile kissfft
6. `cd freetypesetup`
    - Enter the `freetypesetup` directory, where we will cross compile freetype2 to WASM
7. `./init.sh`
    - Clone the freetype-wasm repo and download dependencies
8. `./build.sh`
    - Build brotli, then freetype, and move their binaries to the correct locations
9. `cd ..`
    - Go back to the main directory, `vital`
10. `make wasm_full`
    - Build the .wasm and .js files, and combine with templates. This will output to `docs/` directory (naming scheme for gh pages)
11. `cd docs`
    - Enter the `docs` directory
12. `(&>/dev/null python -m http.server 3000 &)`
    - Start a HTTP server in the background
13. `firefox --new-window "http://localhost:3000"`
    - This will open the WebVial launcher in the browser
    - For chromium: `chromium --new-window "http://localhost:3000"`
    - For chrome: `chrome --new-window "http://localhost:3000"`
14. Leave the default settings, and click the start button
    - <img src=startbtn.png height=24 alt="screenshot of start button">
15. After a bit of lag (up to 30s), the synth should start! Click around a bit i guess
16. Optional: Extra optimisations! Run one of the following in the root directory for increased performance at the cost of filesize.
    - `make wasm_hyprpass_lite` (recommended, pretty much as fast as the others but takes somewhat less time)
    - `make wasm_hyprpass` (very slow)
    - `make wasm_hyprpass_converge` (ETA to pass completion: 1242 years)
    - This should significantly improve startup speeds.