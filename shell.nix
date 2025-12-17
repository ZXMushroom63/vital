{ pkgs ? import <nixpkgs> {} }:
  pkgs.mkShell {
    # nativeBuildInputs is usually what you want -- tools you need to run
    nativeBuildInputs = with pkgs.buildPackages; [
        gnumake
        libglvnd
        alsa-lib
        alsa-oss
        portaudio
        portmidi
        jack2
        xorg.libX11
        xorg.libXinerama
        xorg.libXext
        xorg.libXcursor
        pkg-config
        glib
        libsecret
        meson
        ninja
        fftw
        fftwFloat
        cmake
        ftgl
        freetype
        emscripten
    ];
}