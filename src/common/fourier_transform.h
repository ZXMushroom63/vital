/* Copyright 2013-2019 Matt Tytel
 *
 * vital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vital.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "JuceHeader.h"
#include <kissfft.hh>
#include <pffft.h>

#ifndef USE_PFFFT
  #define USE_PFFFT 1
#endif

#ifndef USE_KISSFFT
  #define USE_KISSFFT 0
#endif

namespace vital {

  #if USE_PFFFT

  class FourierTransform {
    public:
      FourierTransform(int bits) : size_(1 << bits) {
        setup_ = pffft_new_setup(size_, PFFFT_REAL);
        // PFFFT requires 16/32-byte alignment.
        // We allocate internal buffers to ensure alignment regardless of the input pointer.
        work_ = (float*)pffft_aligned_malloc(size_ * sizeof(float));
        buffer_ = (float*)pffft_aligned_malloc(size_ * sizeof(float));
      }

      ~FourierTransform() {
        pffft_destroy_setup(setup_);
        pffft_aligned_free(work_);
        pffft_aligned_free(buffer_);
      }

      void transformRealForward(float* data) {
        // 1. Copy unaligned Vital data to aligned PFFFT buffer
        memcpy(buffer_, data, size_ * sizeof(float));

        // 2. Perform Transform
        // Output format is ordered: [DC, Nyquist, Re1, Im1, Re2, Im2...]
        pffft_transform_ordered(setup_, buffer_, buffer_, work_, PFFFT_FORWARD);

        // 3. Copy back to Vital data
        memcpy(data, buffer_, size_ * sizeof(float));

        // 4. Adjust Layout for Vital
        // Vital expects: [DC, 0, Re1, Im1... ReN/2-1, ImN/2-1, Nyquist, 0]
        float nyquist = data[1];
        data[1] = 0.0f;
        data[size_] = nyquist;
        data[size_ + 1] = 0.0f;
      }

      void transformRealInverse(float* data) {
        // 1. Prepare Data for PFFFT
        // Restore Nyquist from index size_ to index 1
        memcpy(buffer_, data, size_ * sizeof(float));
        buffer_[1] = data[size_];

        // 2. Perform Transform
        pffft_transform_ordered(setup_, buffer_, buffer_, work_, PFFFT_BACKWARD);

        // 3. Apply Scaling (1/N) and Copy Back
        // PFFFT inverse transform is unscaled (sum(X * exp)), so we scale by 1/N
        float scale = 1.0f / size_;
        for (int i = 0; i < size_; ++i) {
          data[i] = buffer_[i] * scale;
        }

        // 4. Zero out the rest of the buffer (Vital expects clean buffer beyond real data)
        memset(data + size_, 0, size_ * sizeof(float));
      }

    private:
      int size_;
      PFFFT_Setup* setup_;
      float* work_;
      float* buffer_;

      JUCE_LEAK_DETECTOR(FourierTransform)
  };

  #elif USE_KISSFFT

  class FourierTransform {
    public:
      FourierTransform(size_t bits) : bits_(bits), size_(1 << bits), forward_(size_, false), inverse_(size_, true) {
        buffer_ = std::make_unique<std::complex<float>[]>(size_);
      }

      ~FourierTransform() { }

      void transformRealForward(float* data) {
        for (int i = size_ - 1; i >= 0; --i) {
          data[2 * i] = data[i];
          data[2 * i + 1] = 0.0f;
        }

        forward_.transform((std::complex<float>*)data, buffer_.get());

        int num_floats = size_ * 2;
        memcpy(data, buffer_.get(), num_floats * sizeof(float));
        data[size_] = data[1];
        data[size_ + 1] = 0.0f;
        data[1] = 0.0f;
      }

      void transformRealInverse(float* data) {
        data[0] *= 0.5f;
        data[1] = data[size_];
        inverse_.transform((std::complex<float>*)data, buffer_.get());
        
        float multiplier = 2.0f / size_;
        for (int i = 0; i < size_; ++i)
          data[i] = buffer_[i].real() * multiplier;

        memset(data + size_, 0, size_ * sizeof(float));
      }

    private:
      size_t bits_;
      size_t size_;
      std::unique_ptr<std::complex<float>[]> buffer_;
      kissfft<float> forward_;
      kissfft<float> inverse_;

      JUCE_LEAK_DETECTOR(FourierTransform)
  };

  #elif INTEL_IPP

  #include "ipps.h"

  class FourierTransform {
    public:
      FourierTransform(int bits) : size_(1 << bits) {
        int spec_size = 0;
        int spec_buffer_size = 0;
        int buffer_size = 0;
        ippsFFTGetSize_R_32f(bits, IPP_FFT_DIV_INV_BY_N, ippAlgHintNone, &spec_size, &spec_buffer_size, &buffer_size);

        spec_ = std::make_unique<Ipp8u[]>(spec_size);
        spec_buffer_ = std::make_unique<Ipp8u[]>(spec_buffer_size);
        buffer_ = std::make_unique<Ipp8u[]>(buffer_size);

        ippsFFTInit_R_32f(&ipp_specs_, bits, IPP_FFT_DIV_INV_BY_N, ippAlgHintNone, spec_.get(), spec_buffer_.get());
      }

      void transformRealForward(float* data) {
        data[size_] = 0.0f;
        ippsFFTFwd_RToPerm_32f_I((Ipp32f*)data, ipp_specs_, buffer_.get());
        data[size_] = data[1];
        data[size_ + 1] = 0.0f;
        data[1] = 0.0f;
      }

      void transformRealInverse(float* data) {
        data[1] = data[size_];
        ippsFFTInv_PermToR_32f_I((Ipp32f*)data, ipp_specs_, buffer_.get());
        memset(data + size_, 0, size_ * sizeof(float));
      }

    private:
      int size_;
      IppsFFTSpec_R_32f *ipp_specs_;
      std::unique_ptr<Ipp8u[]> spec_;
      std::unique_ptr<Ipp8u[]> spec_buffer_;
      std::unique_ptr<Ipp8u[]> buffer_;

      JUCE_LEAK_DETECTOR(FourierTransform)
  };

  #elif 0

  class FourierTransform {
    public:
      FourierTransform(int bits) : fft_(bits) { }

      void transformRealForward(float* data) { fft_.performRealOnlyForwardTransform(data, true); }
      void transformRealInverse(float* data) { fft_.performRealOnlyInverseTransform(data); }

    private:
      dsp::FFT fft_;

      JUCE_LEAK_DETECTOR(FourierTransform)
  };

  #elif __APPLE__
  #define VIMAGE_H
  #include <Accelerate/Accelerate.h>

  class FourierTransform {
    public:
      FourierTransform(vDSP_Length bits) : setup_(vDSP_create_fftsetup(bits, 2)), bits_(bits), size_(1 << bits) { }
      ~FourierTransform() {
        vDSP_destroy_fftsetup(setup_);
      }

      void transformRealForward(float* data) {
        static const float kMult = 0.5f;
        data[size_] = 0.0f;
        DSPSplitComplex split = { data, data + 1 };
        vDSP_fft_zrip(setup_, &split, 2, bits_, kFFTDirection_Forward);
        vDSP_vsmul(data, 1, &kMult, data, 1, size_);

        data[size_] = data[1];
        data[size_ + 1] = 0.0f;
        data[1] = 0.0f;
      }

      void transformRealInverse(float* data) {
        float multiplier = 1.0f / size_;
        DSPSplitComplex split = { data, data + 1 };
        data[1] = data[size_];

        vDSP_fft_zrip(setup_, &split, 2, bits_, kFFTDirection_Inverse);
        vDSP_vsmul(data, 1, &multiplier, data, 1, size_ * 2);
        memset(data + size_, 0, size_ * sizeof(float));
      }

    private:
      FFTSetup setup_;
      vDSP_Length bits_;
      vDSP_Length size_;

      JUCE_LEAK_DETECTOR(FourierTransform)
  };

  #else

  // Default Fallback (re-using KissFFT logic if no other flag is matched)
  class FourierTransform {
    public:
      FourierTransform(size_t bits) : bits_(bits), size_(1 << bits), forward_(size_, false), inverse_(size_, true) {
        buffer_ = std::make_unique<std::complex<float>[]>(size_);
      }

      ~FourierTransform() { }

      void transformRealForward(float* data) {
        for (int i = size_ - 1; i >= 0; --i) {
          data[2 * i] = data[i];
          data[2 * i + 1] = 0.0f;
        }

        forward_.transform((std::complex<float>*)data, buffer_.get());

        int num_floats = size_ * 2;
        memcpy(data, buffer_.get(), num_floats * sizeof(float));
        data[size_] = data[1];
        data[size_ + 1] = 0.0f;
        data[1] = 0.0f;
      }

      void transformRealInverse(float* data) {
        data[0] *= 0.5f;
        data[1] = data[size_];
        inverse_.transform((std::complex<float>*)data, buffer_.get());
        int num_floats = size_ * 2;

        float multiplier = 2.0f / size_;
        for (int i = 0; i < size_; ++i)
          data[i] = buffer_[i].real() * multiplier;

        memset(data + size_, 0, size_ * sizeof(float));
      }

    private:
      size_t bits_;
      size_t size_;
      std::unique_ptr<std::complex<float>[]> buffer_;
      kissfft<float> forward_;
      kissfft<float> inverse_;

      JUCE_LEAK_DETECTOR(FourierTransform)
  };

  #endif

  template <size_t bits>
  class FFT {
    public:
      static FourierTransform* transform() {
        static FFT<bits> instance;
        return &instance.fourier_transform_;
      }

    private:
      FFT() : fourier_transform_(bits) { }

      FourierTransform fourier_transform_;
  };

} // namespace vital