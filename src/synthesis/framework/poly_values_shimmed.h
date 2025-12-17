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

#include <cstdint>
#include <climits>
#include <cstdlib>

#if !defined(force_inline)
#if defined(__EMSCRIPTEN__)
#define force_inline inline __attribute__((always_inline))
#else
#define force_inline inline
#endif
#endif

#if VITAL_AVX2
  #define VITAL_AVX2 0
  static_assert(false, "AVX2 is not supported yet.");
#elif __SSE2__
  #define VITAL_SSE2 0
#elif defined(__ARM_NEON__) || defined(__ARM_NEON)
  #define VITAL_NEON 0
#else
  //static_assert(false, "No SIMD Intrinsics found which are necessary for compilation");
#endif



namespace vital {
struct poly_int {
static constexpr size_t kSize = 4;
union storage_union {
  int32_t scalar[kSize];
  struct {
    int32_t a, b, c, d;
  } named;
};
storage_union value;

static constexpr uint32_t kFullMask = (unsigned int)-1;
static constexpr uint32_t kSignMask = 0x80000000;
static constexpr uint32_t kNotSignMask = kFullMask ^ kSignMask;

force_inline poly_int() noexcept {
  value.scalar[0] = 0;
  value.scalar[1] = 0;
  value.scalar[2] = 0;
  value.scalar[3] = 0;
}


force_inline poly_int(uint32_t initial_value) noexcept {
  value.scalar[0] = (int32_t)initial_value;
  value.scalar[1] = (int32_t)initial_value;
  value.scalar[2] = (int32_t)initial_value;
  value.scalar[3] = (int32_t)initial_value;
}

force_inline poly_int(uint32_t first, uint32_t second, uint32_t third, uint32_t fourth) noexcept {
  value.scalar[0] = (int32_t)first;
  value.scalar[1] = (int32_t)second;
  value.scalar[2] = (int32_t)third;
  value.scalar[3] = (int32_t)fourth;
}

force_inline uint32_t access(size_t index) const noexcept {
  return (uint32_t)value.scalar[index];
}

force_inline void set(size_t index, uint32_t new_value) noexcept {
  value.scalar[index] = (int32_t)new_value;
}

force_inline uint32_t operator[](size_t index) const noexcept {
  return access(index);
}

static force_inline poly_int add(poly_int one, poly_int two) noexcept {
  poly_int r;
  r.value.scalar[0] = one.value.scalar[0] + two.value.scalar[0];
  r.value.scalar[1] = one.value.scalar[1] + two.value.scalar[1];
  r.value.scalar[2] = one.value.scalar[2] + two.value.scalar[2];
  r.value.scalar[3] = one.value.scalar[3] + two.value.scalar[3];
  return r;
}


static force_inline poly_int neg(poly_int value) noexcept {
  poly_int r;
  r.value.scalar[0] = -value.value.scalar[0];
  r.value.scalar[1] = -value.value.scalar[1];
  r.value.scalar[2] = -value.value.scalar[2];
  r.value.scalar[3] = -value.value.scalar[3];
  return r;
}



static force_inline poly_int mul(poly_int one, poly_int two) noexcept {
  poly_int r;
  r.value.scalar[0] = one.value.scalar[0] * two.value.scalar[0];
  r.value.scalar[1] = one.value.scalar[1] * two.value.scalar[1];
  r.value.scalar[2] = one.value.scalar[2] * two.value.scalar[2];
  r.value.scalar[3] = one.value.scalar[3] * two.value.scalar[3];
  return r;
}

static force_inline poly_int bitAnd(poly_int value, poly_int mask) noexcept {
  poly_int r;
  r.value.scalar[0] = value.value.scalar[0] & mask.value.scalar[0];
  r.value.scalar[1] = value.value.scalar[1] & mask.value.scalar[1];
  r.value.scalar[2] = value.value.scalar[2] & mask.value.scalar[2];
  r.value.scalar[3] = value.value.scalar[3] & mask.value.scalar[3];
  return r;
}


static force_inline poly_int bitOr(poly_int value, poly_int mask) noexcept {
  poly_int r;
  r.value.scalar[0] = value.value.scalar[0] | mask.value.scalar[0];
  r.value.scalar[1] = value.value.scalar[1] | mask.value.scalar[1];
  r.value.scalar[2] = value.value.scalar[2] | mask.value.scalar[2];
  r.value.scalar[3] = value.value.scalar[3] | mask.value.scalar[3];
  return r;
}


static force_inline poly_int bitXor(poly_int value, poly_int mask) noexcept {
  poly_int r;
  r.value.scalar[0] = value.value.scalar[0] ^ mask.value.scalar[0];
  r.value.scalar[1] = value.value.scalar[1] ^ mask.value.scalar[1];
  r.value.scalar[2] = value.value.scalar[2] ^ mask.value.scalar[2];
  r.value.scalar[3] = value.value.scalar[3] ^ mask.value.scalar[3];
  return r;
}

static force_inline poly_int bitNot(poly_int value) noexcept {
  poly_int r;
  r.value.scalar[0] = ~value.value.scalar[0];
  r.value.scalar[1] = ~value.value.scalar[1];
  r.value.scalar[2] = ~value.value.scalar[2];
  r.value.scalar[3] = ~value.value.scalar[3];
  return r;
}


static force_inline poly_int max(poly_int one, poly_int two) noexcept {
  poly_int r;
  r.value.scalar[0] = (one.value.scalar[0] > two.value.scalar[0]) ? one.value.scalar[0] : two.value.scalar[0];
  r.value.scalar[1] = (one.value.scalar[1] > two.value.scalar[1]) ? one.value.scalar[1] : two.value.scalar[1];
  r.value.scalar[2] = (one.value.scalar[2] > two.value.scalar[2]) ? one.value.scalar[2] : two.value.scalar[2];
  r.value.scalar[3] = (one.value.scalar[3] > two.value.scalar[3]) ? one.value.scalar[3] : two.value.scalar[3];
  return r;
}

static force_inline poly_int min(poly_int one, poly_int two) noexcept {
  poly_int r;
  r.value.scalar[0] = (one.value.scalar[0] < two.value.scalar[0]) ? one.value.scalar[0] : two.value.scalar[0];
  r.value.scalar[1] = (one.value.scalar[1] < two.value.scalar[1]) ? one.value.scalar[1] : two.value.scalar[1];
  r.value.scalar[2] = (one.value.scalar[2] < two.value.scalar[2]) ? one.value.scalar[2] : two.value.scalar[2];
  r.value.scalar[3] = (one.value.scalar[3] < two.value.scalar[3]) ? one.value.scalar[3] : two.value.scalar[3];
  return r;
}
static force_inline poly_int equal(poly_int one, poly_int two) noexcept {
  poly_int r;
  r.value.scalar[0] = (one.value.scalar[0] == two.value.scalar[0]);
  r.value.scalar[1] = (one.value.scalar[1] == two.value.scalar[1]);
  r.value.scalar[2] = (one.value.scalar[2] == two.value.scalar[2]);
  r.value.scalar[3] = (one.value.scalar[3] == two.value.scalar[3]);
  return r;
}


static force_inline poly_int greaterThan(poly_int one, poly_int two) noexcept {
  poly_int r;
  r.value.scalar[0] = (one.value.scalar[0] > two.value.scalar[0]);
  r.value.scalar[1] = (one.value.scalar[1] > two.value.scalar[1]);
  r.value.scalar[2] = (one.value.scalar[2] > two.value.scalar[2]);
  r.value.scalar[3] = (one.value.scalar[3] > two.value.scalar[3]);
  return r;
}


static force_inline poly_int lessThan(poly_int one, poly_int two) noexcept {
  poly_int r;
  r.value.scalar[0] = (one.value.scalar[0] < two.value.scalar[0]);
  r.value.scalar[1] = (one.value.scalar[1] < two.value.scalar[1]);
  r.value.scalar[2] = (one.value.scalar[2] < two.value.scalar[2]);
  r.value.scalar[3] = (one.value.scalar[3] < two.value.scalar[3]);
  return r;
}


static force_inline uint32_t sum(poly_int value) noexcept {
  return (uint32_t)(value.value.scalar[0] + value.value.scalar[1] +
                    value.value.scalar[2] + value.value.scalar[3]);
}


static force_inline uint32_t anyMask(poly_int value) noexcept {
  return (value.value.scalar[0] | value.value.scalar[1] | 
          value.value.scalar[2] | value.value.scalar[3]) != 0;
}
};


struct poly_float {
static constexpr size_t kSize = 4;

union storage_union {
float scalar[kSize];
struct {
float a, b, c, d;
} named;
};

storage_union value;
//floats
force_inline poly_float() noexcept {
value.scalar[0] = 0.0f;
value.scalar[1] = 0.0f;
value.scalar[2] = 0.0f;
value.scalar[3] = 0.0f;
}

force_inline poly_float(float initial_value) noexcept {
value.scalar[0] = initial_value;
value.scalar[1] = initial_value;
value.scalar[2] = initial_value;
value.scalar[3] = initial_value;
}

force_inline poly_float(float first, float second, float third, float fourth) noexcept {
value.scalar[0] = first;
value.scalar[1] = second;
value.scalar[2] = third;
value.scalar[3] = fourth;
}

force_inline float access(size_t index) const noexcept {
return value.scalar[index];
}

force_inline void set(size_t index, float new_value) noexcept {
value.scalar[index] = new_value;
}

force_inline float operator[](size_t index) const noexcept {
return access(index);
}

static force_inline poly_float add(poly_float one, poly_float two) noexcept {
poly_float r;
r.value.scalar[0] = one.value.scalar[0] + two.value.scalar[0];
r.value.scalar[1] = one.value.scalar[1] + two.value.scalar[1];
r.value.scalar[2] = one.value.scalar[2] + two.value.scalar[2];
r.value.scalar[3] = one.value.scalar[3] + two.value.scalar[3];
return r;
}

static force_inline poly_float neg(poly_float value) noexcept {
poly_float r;
r.value.scalar[0] = -value.value.scalar[0];
r.value.scalar[1] = -value.value.scalar[1];
r.value.scalar[2] = -value.value.scalar[2];
r.value.scalar[3] = -value.value.scalar[3];
return r;
}

static force_inline poly_float mul(poly_float one, poly_float two) noexcept {
poly_float r;
r.value.scalar[0] = one.value.scalar[0] * two.value.scalar[0];
r.value.scalar[1] = one.value.scalar[1] * two.value.scalar[1];
r.value.scalar[2] = one.value.scalar[2] * two.value.scalar[2];
r.value.scalar[3] = one.value.scalar[3] * two.value.scalar[3];
return r;
}

static force_inline poly_float bitAnd(poly_float value, poly_float mask) noexcept {
poly_float r;
r.value.scalar[0] = static_cast(static_cast<int32_t>(value.value.scalar[0]) & static_cast<int32_t>(mask.value.scalar[0]));
r.value.scalar[1] = static_cast(static_cast<int32_t>(value.value.scalar[1]) & static_cast<int32_t>(mask.value.scalar[1]));
r.value.scalar[2] = static_cast(static_cast<int32_t>(value.value.scalar[2]) & static_cast<int32_t>(mask.value.scalar[2]));
r.value.scalar[3] = static_cast(static_cast<int32_t>(value.value.scalar[3]) & static_cast<int32_t>(mask.value.scalar[3]));
return r;
}

static force_inline poly_float bitOr(poly_float value, poly_float mask) noexcept {
poly_float r;
r.value.scalar[0] = static_cast(static_cast<int32_t>(value.value.scalar[0]) | static_cast<int32_t>(mask.value.scalar[0]));
r.value.scalar[1] = static_cast(static_cast<int32_t>(value.value.scalar[1]) | static_cast<int32_t>(mask.value.scalar[1]));
r.value.scalar[2] = static_cast(static_cast<int32_t>(value.value.scalar[2]) | static_cast<int32_t>(mask.value.scalar[2]));
r.value.scalar[3] = static_cast(static_cast<int32_t>(value.value.scalar[3]) | static_cast<int32_t>(mask.value.scalar[3]));
return r;
}
static force_inline poly_float bitXor(poly_float value, poly_float mask) noexcept {
poly_float r;
r.value.scalar[0] = static_cast(static_cast<int32_t>(value.value.scalar[0]) ^ static_cast<int32_t>(mask.value.scalar[0]));
r.value.scalar[1] = static_cast(static_cast<int32_t>(value.value.scalar[1]) ^ static_cast<int32_t>(mask.value.scalar[1]));
r.value.scalar[2] = static_cast(static_cast<int32_t>(value.value.scalar[2]) ^ static_cast<int32_t>(mask.value.scalar[2]));
r.value.scalar[3] = static_cast(static_cast<int32_t>(value.value.scalar[3]) ^ static_cast<int32_t>(mask.value.scalar[3]));
return r;
}

static force_inline poly_float bitNot(poly_float value) noexcept {
poly_float r;
r.value.scalar[0] = static_cast(~static_cast<int32_t>(value.value.scalar[0]));
r.value.scalar[1] = static_cast(~static_cast<int32_t>(value.value.scalar[1]));
r.value.scalar[2] = static_cast(~static_cast<int32_t>(value.value.scalar[2]));
r.value.scalar[3] = static_cast(~static_cast<int32_t>(value.value.scalar[3]));
return r;
}

static force_inline poly_float max(poly_float one, poly_float two) noexcept {
poly_float r;
r.value.scalar[0] = (one.value.scalar[0] > two.value.scalar[0]) ? one.value.scalar[0] : two.value.scalar[0];
r.value.scalar[1] = (one.value.scalar[1] > two.value.scalar[1]) ? one.value.scalar[1] : two.value.scalar[1];
r.value.scalar[2] = (one.value.scalar[2] > two.value.scalar[2]) ? one.value.scalar[2] : two.value.scalar[2];
r.value.scalar[3] = (one.value.scalar[3] > two.value.scalar[3]) ? one.value.scalar[3] : two.value.scalar[3];
return r;
}

static force_inline poly_float min(poly_float one, poly_float two) noexcept {
poly_float r;
r.value.scalar[0] = (one.value.scalar[0] < two.value.scalar[0]) ? one.value.scalar[0] : two.value.scalar[0];
r.value.scalar[1] = (one.value.scalar[1] < two.value.scalar[1]) ? one.value.scalar[1] : two.value.scalar[1];
r.value.scalar[2] = (one.value.scalar[2] < two.value.scalar[2]) ? one.value.scalar[2] : two.value.scalar[2];
r.value.scalar[3] = (one.value.scalar[3] < two.value.scalar[3]) ? one.value.scalar[3] : two.value.scalar[3];
return r;
}

static force_inline poly_float equal(poly_float one, poly_float two) noexcept {
poly_float r;
r.value.scalar[0] = (one.value.scalar[0] == two.value.scalar[0]);
r.value.scalar[1] = (one.value.scalar[1] == two.value.scalar[1]);
r.value.scalar[2] = (one.value.scalar[2] == two.value.scalar[2]);
r.value.scalar[3] = (one.value.scalar[3] == two.value.scalar[3]);
return r;
}

static force_inline poly_float greaterThan(poly_float one, poly_float two) noexcept {
poly_float r;
r.value.scalar[0] = (one.value.scalar[0] > two.value.scalar[0]);
r.value.scalar[1] = (one.value.scalar[1] > two.value.scalar[1]);
r.value.scalar[2] = (one.value.scalar[2] > two.value.scalar[2]);
r.value.scalar[3] = (one.value.scalar[3] > two.value.scalar[3]);
return r;
}

static force_inline poly_float lessThan(poly_float one, poly_float two) noexcept {
poly_float r;
r.value.scalar[0] = (one.value.scalar[0] < two.value.scalar[0]);
r.value.scalar[1] = (one.value.scalar[1] < two.value.scalar[1]);
r.value.scalar[2] = (one.value.scalar[2] < two.value.scalar[2]);
r.value.scalar[3] = (one.value.scalar[3] < two.value.scalar[3]);
return r;
}

  static force_inline float sum(poly_float value) noexcept {
    return value.value.scalar[0] + value.value.scalar[1] +
           value.value.scalar[2] + value.value.scalar[3];
  }

  static force_inline uint32_t anyMask(poly_float value) noexcept {
    return (static_cast<int32_t>(value.value.scalar[0]) != 0 ||
            static_cast<int32_t>(value.value.scalar[1]) != 0 ||
            static_cast<int32_t>(value.value.scalar[2]) != 0 ||
            static_cast<int32_t>(value.value.scalar[3]) != 0);
  }


//static force_inline poly_int max(poly_int one, poly_int two) { return max(one, two); }
//static force_inline poly_int min(poly_int one, poly_int two) { return min(one, two); }
//static force_inline poly_int equal(poly_int one, poly_int two) { return equal(one, two); }
//static force_inline poly_int greaterThan(poly_int one, poly_int two) { return greaterThan(one, two); }
//static force_inline poly_int lessThan(poly_int one, poly_int two) { return lessThan(one, two); }
//force_inline uint32_t sum() const noexcept { return sum(value); }
//force_inline uint32_t anyMask() const noexcept { return anyMask(value); }
};
};