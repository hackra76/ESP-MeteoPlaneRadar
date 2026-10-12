// =============================================================================
//  MeteoPlaneRadar - High-Performance 32-bit Packed SIMD Graphics Primitives
//  Optimized for ESP32-S3 (Xtensa LX7 32-bit registers & dual-core ALU)
//
//  Provides:
//    - blend565_packed64: 64-level RGB565 blend using 32-bit register packing
//      (processes Red and Blue channels simultaneously in a single 32-bit word).
//    - blendColor_fast: Drop-in replacement for float-based alpha blend.
//    - addGlow_fast: Native RGB565 additive bloom without 8-bit conversions.
//    - blendFrames565_SIMD: 32-bit dual-pixel cross-dissolve blending with fast
//      zero/equality skipping for radar animation frames.
// =============================================================================
#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>

/**
 * @brief Blends two RGB565 colors using 64-level alpha (6 bits).
 * In 16-bit RGB565 (R: bits 11..15, G: bits 5..10, B: bits 0..4):
 * Masking R and B as (c & 0xF81F) leaves 6 zero bits between Red and Blue.
 * Multiplying by weights in [0..64] produces at most 31 * 64 = 1984 (11 bits),
 * which NEVER bleeds from Blue (bits 0..10) into Red (bits 11..21).
 * Thus Red and Blue are blended SIMULTANEOUSLY in a single 32-bit multiply!
 */
static inline uint16_t blend565_packed64(uint16_t c1, uint16_t c2, uint32_t a64) {
  if (a64 == 0) return c1;
  if (a64 >= 64) return c2;
  const uint32_t inv_a = 64 - a64;
  const uint32_t rb1 = c1 & 0xF81F;
  const uint32_t rb2 = c2 & 0xF81F;
  const uint32_t g1  = c1 & 0x07E0;
  const uint32_t g2  = c2 & 0x07E0;
  const uint32_t rb = ((rb1 * inv_a + rb2 * a64) >> 6) & 0xF81F;
  const uint32_t g  = ((g1  * inv_a + g2  * a64) >> 6) & 0x07E0;
  return (uint16_t)(rb | g);
}

/**
 * @brief Fast float-alpha RGB565 blend wrapper.
 */
static inline uint16_t blendColor_fast(uint16_t c1, uint16_t c2, float a) {
  if (a <= 0.0f) return c1;
  if (a >= 1.0f) return c2;
  uint32_t a64 = (uint32_t)(a * 64.0f + 0.5f);
  return blend565_packed64(c1, c2, a64);
}

/**
 * @brief Native RGB565 additive bloom/glow without 8-bit unpacking roundtrips.
 */
static inline uint16_t addGlow_fast(uint16_t base, uint8_t gr, uint8_t gg, uint8_t gb, float intensity) {
  if (intensity <= 0.001f) return base;
  uint32_t ir = (uint32_t)(gr * intensity);
  uint32_t ig = (uint32_t)(gg * intensity);
  uint32_t ib = (uint32_t)(gb * intensity);

  uint32_t r = ((base >> 11) & 0x1F) + (ir >> 3);
  uint32_t g = ((base >> 5)  & 0x3F) + (ig >> 2);
  uint32_t b = (base         & 0x1F) + (ib >> 3);
  if (r > 31) r = 31;
  if (g > 63) g = 63;
  if (b > 31) b = 31;
  return (uint16_t)((r << 11) | (g << 5) | b);
}

/**
 * @brief Blends two RGB565 image buffers using packed 32-bit math.
 * Processes 2 pixels (32 bits) per iteration with zero/black fast-paths.
 */
static inline void blendFrames565_SIMD(const uint16_t* srcA, const uint16_t* srcB,
                                      uint16_t* dst, size_t count, uint32_t a64) {
  if (a64 == 0) {
    if (dst != srcA) memcpy(dst, srcA, count * sizeof(uint16_t));
    return;
  }
  if (a64 >= 64) {
    if (dst != srcB) memcpy(dst, srcB, count * sizeof(uint16_t));
    return;
  }
  const uint32_t inv_a = 64 - a64;
  size_t i = 0;

  // Align to 32-bit boundary if starting pointer is unaligned
  if (((uintptr_t)srcA & 2) && count > 0) {
    uint32_t c1 = srcA[0], c2 = srcB[0];
    if (c1 == c2) dst[0] = (uint16_t)c1;
    else if ((c1 | c2) == 0) dst[0] = 0;
    else {
      uint32_t rb = (((c1 & 0xF81F) * inv_a + (c2 & 0xF81F) * a64) >> 6) & 0xF81F;
      uint32_t g  = (((c1 & 0x07E0) * inv_a + (c2 & 0x07E0) * a64) >> 6) & 0x07E0;
      dst[0] = (uint16_t)(rb | g);
    }
    i = 1;
  }

  const uint32_t* pA32 = (const uint32_t*)(srcA + i);
  const uint32_t* pB32 = (const uint32_t*)(srcB + i);
  uint32_t* pDst32 = (uint32_t*)(dst + i);
  size_t pairs = (count - i) / 2;

  for (size_t p = 0; p < pairs; p++) {
    uint32_t wA = pA32[p];
    uint32_t wB = pB32[p];
    if ((wA | wB) == 0) {
      pDst32[p] = 0;
      continue;
    }
    if (wA == wB) {
      pDst32[p] = wA;
      continue;
    }

    // Pixel 0 (low 16 bits)
    uint32_t c1_0 = wA & 0xFFFF;
    uint32_t c2_0 = wB & 0xFFFF;
    uint32_t out0;
    if (c1_0 == c2_0) out0 = c1_0;
    else if ((c1_0 | c2_0) == 0) out0 = 0;
    else {
      uint32_t rb0 = (((c1_0 & 0xF81F) * inv_a + (c2_0 & 0xF81F) * a64) >> 6) & 0xF81F;
      uint32_t g0  = (((c1_0 & 0x07E0) * inv_a + (c2_0 & 0x07E0) * a64) >> 6) & 0x07E0;
      out0 = rb0 | g0;
    }

    // Pixel 1 (high 16 bits)
    uint32_t c1_1 = (wA >> 16) & 0xFFFF;
    uint32_t c2_1 = (wB >> 16) & 0xFFFF;
    uint32_t out1;
    if (c1_1 == c2_1) out1 = c1_1;
    else if ((c1_1 | c2_1) == 0) out1 = 0;
    else {
      uint32_t rb1 = (((c1_1 & 0xF81F) * inv_a + (c2_1 & 0xF81F) * a64) >> 6) & 0xF81F;
      uint32_t g1  = (((c1_1 & 0x07E0) * inv_a + (c2_1 & 0x07E0) * a64) >> 6) & 0x07E0;
      out1 = rb1 | g1;
    }

    pDst32[p] = out0 | (out1 << 16);
  }

  // Trailing odd pixel
  if ((count - i) & 1) {
    size_t lastIdx = count - 1;
    uint32_t c1 = srcA[lastIdx], c2 = srcB[lastIdx];
    if (c1 == c2) dst[lastIdx] = (uint16_t)c1;
    else if ((c1 | c2) == 0) dst[lastIdx] = 0;
    else {
      uint32_t rb = (((c1 & 0xF81F) * inv_a + (c2 & 0xF81F) * a64) >> 6) & 0xF81F;
      uint32_t g  = (((c1 & 0x07E0) * inv_a + (c2 & 0x07E0) * a64) >> 6) & 0x07E0;
      dst[lastIdx] = (uint16_t)(rb | g);
    }
  }
}
