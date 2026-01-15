#include <vector>
#include <cmath>
#include <complex>
#include <algorithm>
#include <array>
#include <GLES3/gl3.h>

using Complex = std::complex<float>;

Complex UnshadedFilterResponses::onePoleResponse(float cutoff) {
    return 1.0f / Complex(1.0f, cutoff);
}

Complex UnshadedFilterResponses::onePoleInvertResponse(float cutoff) {
    return Complex(1.0f, cutoff);
}

float UnshadedFilterResponses::magnitudeToDb(float magnitude) {
    return 8.685889638065037f * std::log(magnitude + 1e-9f);
}

float UnshadedFilterResponses::getCutoffRatio(float x, float midi_cutoff, float maxMidi) {
    float percent = 0.5f * (x + 1.0f);
    float midi_note = ShaderConstants::kMinMidiNote + percent * (maxMidi - ShaderConstants::kMinMidiNote);
    float exponent = std::min((midi_note - midi_cutoff) / 12.0f, 8.0f);
    return std::pow(2.0f, exponent);
}

float UnshadedFilterResponses::getYForResponse(Complex response) {
    float magnitude_response = std::abs(response);
    float db = magnitudeToDb(magnitude_response);
    return 2.0f * (db - ShaderConstants::kMinDb) / (ShaderConstants::kMaxDb - ShaderConstants::kMinDb) - 1.0f;
}

float UnshadedFilterResponses::getYForEqResponse(Complex response) {
    float magnitude_response = std::abs(response);
    float db = magnitudeToDb(magnitude_response);
    return 2.0f * (db - ShaderConstants::kEqMinDb) / (ShaderConstants::kEqMaxDb - ShaderConstants::kEqMinDb) - 1.0f;
}

float UnshadedFilterResponses::step(float edge, float x) {
    return (x < edge) ? 0.0f : 1.0f;
}
float UnshadedFilterResponses::getX(int i, int size) {
    return ((float)i / (float)(size - 1)) * 2.0f - 1.0f;
}

void UnshadedFilterResponses::fillAnalog(float* output, int size, const FilterUniforms& u) {
    for (int i = 0; i < size; ++i) {
        float posX = getX(i, size);
        
        Complex one_pole = onePoleResponse(getCutoffRatio(posX, u.midi_cutoff));
        Complex low = one_pole * one_pole;
        Complex band = one_pole - low;
        Complex high = Complex(1.0f, 0.0f) - one_pole - band;

        Complex two_stage_pre = u.stage3 * low + u.stage1 * band + u.stage4 * high;
        Complex two_stage = u.stage0 * low + u.stage1 * band + u.stage2 * high;

        Complex feedback = one_pole * (Complex(1.0f, 0.0f) - one_pole);
        Complex denominator_pre = Complex(1.0f, 0.0f) - feedback;
        Complex denominator = Complex(1.0f, 0.0f) - u.resonance * feedback;

        Complex response_pre = two_stage_pre / denominator_pre;
        Complex response_res = two_stage / denominator;

        Complex response = u.drive * response_res;
        response = response + u.db24 * ((response_pre * response) - response);
        response = u.mix * response + Complex(1.0f - u.mix, 0.0f);

        output[i] = getYForResponse(response);
    }
}

void UnshadedFilterResponses::fillComb(float* output, int size, const FilterUniforms& u) {
    float kMaxCycles = 6.0f;
    float posY = 0.0f; 

    for (int i = 0; i < size; ++i) {
        float posX = getX(i, size);

        float ratio = getCutoffRatio(posX, u.midi_cutoff);
        float ratio_diff = getCutoffRatio(posX + 0.02f, u.midi_cutoff) - ratio;
        float max_step = step(kMaxCycles, ratio);

        float angle = 2.0f * ShaderConstants::kPi * ratio;
        Complex tick = u.resonance * Complex(std::cos(angle), -std::sin(angle));

        Complex low_pass = onePoleResponse(getCutoffRatio(posX, u.stage2));
        Complex high_pass = Complex(1.0f, 0.0f) - low_pass;
        Complex one_pole = u.stage0 * low_pass + u.stage1 * high_pass;
        
        Complex high_pass2 = Complex(1.0f, 0.0f) - onePoleResponse(getCutoffRatio(posX, u.stage3));
        
        Complex filter_input = Complex(1.0f - 0.5f * std::abs(u.resonance), 0.0f);
        filter_input = (filter_input * one_pole) * high_pass2;

        Complex denominator = Complex(1.0f, 0.0f) - ((tick * one_pole) * high_pass2);

        float round_val = (one_pole * high_pass2).real() * std::abs(u.resonance);
        Complex denominator_round = Complex(1.0f - round_val, 0.0f);
        Complex denominator_round_down = Complex(1.0f + round_val, 0.0f);

        float max_step_mult = 1.0f - posY;
        Complex max_step_denominator = max_step_mult * denominator_round + (1.0f - max_step_mult) * denominator_round_down;

        denominator = max_step * max_step_denominator + (1.0f - max_step) * denominator;

        float denominator_round_step = 1.0f - std::max(max_step, step(ratio_diff, std::abs(denominator)));
        denominator = denominator_round_step * denominator_round + (1.0f - denominator_round_step) * denominator;

        Complex response = (u.drive * filter_input) / denominator;
        response = u.mix * response + Complex(1.0f - u.mix, 0.0f);

        output[i] = getYForResponse(response);
    }
}

void UnshadedFilterResponses::fillPositiveFlange(float* output, int size, const FilterUniforms& u) {
    float kMaxCycles = 8.0f;
    float posY = 0.0f; 

    for (int i = 0; i < size; ++i) {
        float posX = getX(i, size);
        
        float ratio = getCutoffRatio(posX, u.midi_cutoff);
        float ratio_diff = getCutoffRatio(posX + 0.02f, u.midi_cutoff) - ratio;
        float max_step = step(kMaxCycles, ratio);

        float angle = 2.0f * ShaderConstants::kPi * ratio;
        Complex delay = Complex(std::cos(angle), -std::sin(angle));
        Complex tick = u.resonance * delay;

        Complex low_pass = onePoleResponse(getCutoffRatio(posX, u.stage2));
        Complex high_pass = Complex(1.0f, 0.0f) - low_pass;
        Complex one_pole = u.stage0 * low_pass + u.stage1 * high_pass;
        
        Complex high_pass2 = Complex(1.0f, 0.0f) - onePoleResponse(getCutoffRatio(posX, u.stage3));
        
        Complex filter_input = Complex(0.70710678119f, 0.0f);
        Complex delay_input = (filter_input * one_pole) * high_pass2;

        Complex denominator = Complex(1.0f, 0.0f) - tick;
        
        Complex round_value = (one_pole * high_pass2) * u.resonance;
        Complex denominator_round = delay * (Complex(1.0f, 0.0f) - round_value);
        Complex denominator_round_down = -delay * (Complex(1.0f, 0.0f) + round_value);

        float max_step_mult = 1.0f - posY;
        Complex max_step_denominator = max_step_mult * denominator_round + (1.0f - max_step_mult) * denominator_round_down;

        denominator = max_step * max_step_denominator + (1.0f - max_step) * denominator;
        float denominator_round_step = 1.0f - std::max(max_step, step(ratio_diff, std::abs(denominator)));
        denominator = denominator_round_step * denominator_round + (1.0f - denominator_round_step) * denominator;

        Complex response = filter_input * u.drive + (delay_input / denominator) * delay;
        response = u.mix * response + Complex(1.0f - u.mix, 0.0f);

        output[i] = getYForResponse(response);
    }
}

void UnshadedFilterResponses::fillNegativeFlange(float* output, int size, const FilterUniforms& u) {
    float kMaxCycles = 8.0f;
    float posY = 0.0f; 

    for (int i = 0; i < size; ++i) {
        float posX = getX(i, size);

        float ratio = getCutoffRatio(posX, u.midi_cutoff + 12.0f);
        float max_step = step(kMaxCycles, ratio);

        float angle = 2.0f * ShaderConstants::kPi * ratio;
        Complex delay = Complex(std::cos(angle), -std::sin(angle));
        Complex tick = -u.resonance * delay;

        Complex low_pass = onePoleResponse(getCutoffRatio(posX, u.stage2));
        Complex high_pass = Complex(1.0f, 0.0f) - low_pass;
        Complex one_pole = u.stage0 * low_pass + u.stage1 * high_pass;
        Complex high_pass2 = Complex(1.0f, 0.0f) - onePoleResponse(getCutoffRatio(posX, u.stage3));

        Complex filter_input = Complex(0.70710678119f, 0.0f);
        Complex delay_input = (filter_input * one_pole) * high_pass2;

        Complex denominator = Complex(1.0f, 0.0f) - (tick * (one_pole * high_pass2));
        Complex round_value = -(one_pole * high_pass2) * u.resonance;
        
        Complex denominator_round = delay * (Complex(1.0f, 0.0f) - round_value);
        Complex denominator_round_down = -delay * (Complex(1.0f, 0.0f) + round_value);

        float max_step_mult = 1.0f - posY;
        Complex max_step_denominator = max_step_mult * denominator_round + (1.0f - max_step_mult) * denominator_round_down;

        denominator = max_step * max_step_denominator + (1.0f - max_step) * denominator;

        Complex response = filter_input * u.drive - (delay_input / denominator) * delay;
        response = u.mix * response + Complex(1.0f - u.mix, 0.0f);

        output[i] = getYForResponse(response);
    }
}

void UnshadedFilterResponses::fillDigital(float* output, int size, const FilterUniforms& u) {
    for (int i = 0; i < size; ++i) {
        float posX = getX(i, size);
        
        float g = getCutoffRatio(posX, u.midi_cutoff);
        Complex g2 = Complex(g * g, 0.0f);

        Complex denominator = g2 + Complex(0.0f, g * u.resonance) + Complex(-1.0f, 0.0f);
        Complex numerator = -u.stage0 * Complex(1.0f, 0.0f) + u.stage1 * Complex(0.0f, g) + u.stage2 * g2;
        Complex numerator_pre = -u.stage3 * Complex(1.0f, 0.0f) + u.stage1 * Complex(0.0f, g) + u.stage4 * g2;

        Complex response = numerator / denominator;
        Complex pre_denominator = g2 + Complex(0.0f, g) + Complex(-1.0f, 0.0f);
        Complex pre_response = numerator_pre / pre_denominator;

        response = response + u.db24 * ((response * pre_response) - response);
        response *= u.drive;
        response = u.mix * response + Complex(1.0f - u.mix, 0.0f);

        output[i] = getYForResponse(response);
    }
}

void UnshadedFilterResponses::fillDiode(float* output, int size, const FilterUniforms& u) {
    for (int i = 0; i < size; ++i) {
        float posX = getX(i, size);
        
        float ratio = getCutoffRatio(posX, u.midi_cutoff);
        Complex one_pole = onePoleResponse(ratio);
        
        float high_pass_ratio = (u.stage0 != 0.0f) ? (ratio / u.stage0) : 1e9f;
        Complex high_pass_one_pole = onePoleResponse(high_pass_ratio);

        Complex high = Complex(1.0f, 0.0f) - high_pass_one_pole * 2.0f + (high_pass_one_pole * high_pass_one_pole);
        Complex high_feedback = high_pass_one_pole * (Complex(1.0f, 0.0f) - high_pass_one_pole);
        Complex high_denominator = Complex(1.0f, 0.0f) - high_feedback;

        Complex high_pass_response = high / high_denominator;
        high_pass_response = Complex(1.0f, 0.0f) + u.db24 * (high_pass_response + Complex(-1.0f, 0.0f));

        Complex loop = one_pole * one_pole;
        Complex series = 0.125f * (loop * loop);
        Complex chain = series / (Complex(1.0f, 0.0f) + series - loop);

        Complex numerator = u.drive * chain;
        Complex denominator = Complex(1.0f, 0.0f) + u.resonance * chain;
        Complex response = numerator / denominator;

        response = u.mix * response + Complex(1.0f - u.mix, 0.0f);
        response = response * high_pass_response;

        output[i] = getYForResponse(response);
    }
}

void UnshadedFilterResponses::fillDirty(float* output, int size, const FilterUniforms& u) {
    for (int i = 0; i < size; ++i) {
        float posX = getX(i, size);
        
        Complex one_pole = onePoleResponse(getCutoffRatio(posX, u.midi_cutoff));
        Complex low = one_pole * one_pole;
        Complex band = one_pole - low;
        Complex high = Complex(1.0f, 0.0f) - one_pole - band;

        Complex two_stage_pre = u.stage3 * low + u.stage1 * band + u.stage4 * high;
        Complex two_stage = u.stage0 * low + u.stage1 * band + u.stage2 * high;

        Complex feedback = one_pole * (Complex(1.0f, 0.0f) - one_pole);
        Complex denominator_pre = Complex(1.0f, 0.0f) - feedback;
        
        float res = (std::abs(u.resonance) < 1e-9f) ? 1e-9f : u.resonance;
        Complex denominator = Complex(1.0f / res, 0.0f) - feedback;

        Complex resonance_loop = band / denominator;
        Complex response_pre = two_stage_pre / denominator_pre;
        Complex response_res = two_stage * (Complex(1.0f, 0.0f) + resonance_loop);
        
        Complex response = u.drive * response_res;
        response = response + u.db24 * ((response_pre * response) - response);
        response = u.mix * response + Complex(1.0f - u.mix, 0.0f);

        output[i] = getYForResponse(response);
    }
}

void UnshadedFilterResponses::fillFormant(float* output, int size, const FormantUniforms& u) {
    for (int i = 0; i < size; ++i) {
        float posX = getX(i, size);
        Complex total_response(0.0f, 0.0f);

        for (int k = 0; k < 4; ++k) {
            float g = getCutoffRatio(posX, u.formant_cutoff[k]);
            Complex g_sqr = Complex(g * g, 0.0f);
            Complex denominator = g_sqr + Complex(0.0f, g * u.formant_resonance[k]) + Complex(-1.0f, 0.0f);
            Complex numerator = -u.low[k] * Complex(1.0f, 0.0f) + u.band[k] * Complex(0.0f, g) + u.high[k] * g_sqr;
            total_response += numerator / denominator;
        }

        Complex response = u.mix * total_response + Complex(1.0f - u.mix, 0.0f);
        output[i] = getYForResponse(response);
    }
}

void UnshadedFilterResponses::fillLadder(float* output, int size, const FilterUniforms& u) {
    for (int i = 0; i < size; ++i) {
        float posX = getX(i, size);
        
        Complex one_pole_invert = onePoleInvertResponse(getCutoffRatio(posX, u.midi_cutoff));
        Complex two_pole_invert = one_pole_invert * one_pole_invert;
        Complex three_pole_invert = one_pole_invert * two_pole_invert;
        Complex four_pole_invert = one_pole_invert * three_pole_invert;

        Complex numerator = u.drive * (
            u.stage0 * four_pole_invert + 
            u.stage1 * three_pole_invert + 
            u.stage2 * two_pole_invert + 
            u.stage3 * one_pole_invert + 
            Complex(u.stage4, 0.0f)
        );
        Complex denominator = four_pole_invert + Complex(u.resonance, 0.0f);
        
        Complex response = numerator / denominator;
        response = u.mix * response + Complex(1.0f - u.mix, 0.0f);

        output[i] = getYForResponse(response);
    }
}

void UnshadedFilterResponses::fillPhaser(float* output, int size, const FilterUniforms& u) {
    for (int i = 0; i < size; ++i) {
        float posX = getX(i, size);

        float g = getCutoffRatio(posX, u.midi_cutoff);
        Complex one_pole = onePoleResponse(g);
        Complex all_pass = Complex(1.0f, 0.0f) - 2.0f * one_pole;
        
        Complex half_peak = all_pass * all_pass;
        Complex peak1 = half_peak * half_peak;
        Complex peak3 = peak1 * peak1;
        Complex peak5 = peak3 * peak1;

        Complex chain = u.stage0 * peak1 + u.stage1 * peak3 + u.stage2 * peak5;
        float invert_mult = 1.0f - 2.0f * u.db24;

        Complex feedback_chain = chain * onePoleResponse(0.05f * g);
        feedback_chain = feedback_chain * (Complex(1.0f, 0.0f) - onePoleResponse(20.0f * g));

        Complex denominator = Complex(1.0f, 0.0f) - invert_mult * u.resonance * feedback_chain;
        Complex phase_response = chain / denominator;

        Complex response = Complex(0.5f, 0.0f) + 0.5f * invert_mult * phase_response;
        response = u.mix * response + Complex(1.0f - u.mix, 0.0f);

        output[i] = getYForResponse(response);
    }
}

void UnshadedFilterResponses::fillEq(float* output, int size, const EqUniforms& u) {
    for (int i = 0; i < size; ++i) {
        float posX = getX(i, size);
        
        Complex numerator(1.0f, 0.0f);
        Complex denominator(1.0f, 0.0f);

        for (int k = 0; k < 3; ++k) {
            float g = getCutoffRatio(posX, u.midi_cutoff[k], ShaderConstants::kEqMaxMidiNote);
            Complex g_sqr = Complex(g * g, 0.0f);
            Complex den = g_sqr + Complex(0.0f, g * u.resonance[k]) + Complex(-1.0f, 0.0f);
            Complex num = -u.low_amount[k] * Complex(1.0f, 0.0f) + u.band_amount[k] * Complex(0.0f, g) + u.high_amount[k] * g_sqr;
            
            numerator *= num;
            denominator *= den;
        }

        Complex response = numerator / denominator;
        output[i] = getYForEqResponse(response);
    }
}