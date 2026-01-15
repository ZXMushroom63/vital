#ifndef UNSHADED_FILTER_RESPONSES_H
#define UNSHADED_FILTER_RESPONSES_H

#include <vector>
#include <cmath>
#include <complex>
#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>
#include <GLES3/gl3.h>

namespace ShaderConstants {
    constexpr float kPi = 3.14159265359f;
    constexpr float kMinMidiNote = 8.0f;
    constexpr float kMaxMidiNote = 137.0f;
    constexpr float kMidi0Frequency = 8.1757989156f;
    
    constexpr float kMinDb = -30.0f;
    constexpr float kMaxDb = 20.0f;

    constexpr float kEqMinDb = -1.0f;
    constexpr float kEqMaxDb = 1.0f;
    constexpr float kEqMaxMidiNote = 136.0f;
}

struct FilterUniforms {
    float midi_cutoff = 64.0f;
    float resonance = 0.0f;
    float drive = 1.0f;
    float mix = 1.0f;
    float db24 = 0.0f;
    float stage0 = 0.0f;
    float stage1 = 0.0f;
    float stage2 = 0.0f;
    float stage3 = 0.0f;
    float stage4 = 0.0f;
};

struct FormantUniforms {
    std::array<float, 4> formant_cutoff;
    std::array<float, 4> formant_resonance;
    std::array<float, 4> low;
    std::array<float, 4> band;
    std::array<float, 4> high;
    float mix = 1.0f;
};

struct EqUniforms {
    std::array<float, 3> midi_cutoff;
    std::array<float, 3> resonance;
    std::array<float, 3> low_amount;
    std::array<float, 3> band_amount;
    std::array<float, 3> high_amount;
};

class UnshadedFilterResponses {
private:
    using Complex = std::complex<float>;

    static Complex onePoleResponse(float cutoff);
    static Complex onePoleInvertResponse(float cutoff);
    static float magnitudeToDb(float magnitude);
    static float getCutoffRatio(float x, float midi_cutoff, float maxMidi = ShaderConstants::kMaxMidiNote);
    static float getYForResponse(Complex response);
    static float getYForEqResponse(Complex response);
    static float step(float edge, float x);

public:
    static float getX(int i, int size = 512);
    static void fillAnalog(float* output, int size, const FilterUniforms& u);
    static void fillComb(float* output, int size, const FilterUniforms& u);
    static void fillPositiveFlange(float* output, int size, const FilterUniforms& u);
    static void fillNegativeFlange(float* output, int size, const FilterUniforms& u);
    static void fillDigital(float* output, int size, const FilterUniforms& u);
    static void fillDiode(float* output, int size, const FilterUniforms& u);
    static void fillDirty(float* output, int size, const FilterUniforms& u);
    static void fillFormant(float* output, int size, const FormantUniforms& u);
    static void fillLadder(float* output, int size, const FilterUniforms& u);
    static void fillPhaser(float* output, int size, const FilterUniforms& u);
    static void fillEq(float* output, int size, const EqUniforms& u);
};

#endif