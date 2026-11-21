// Argon: a FIR filter over a block of samples, see fir_block.hpp.
#include "audio/fir_block.hpp"

#include <immintrin.h>

#if defined(_MSC_VER)
#include <intrin.h>
#define ARGON_TARGET_AVX2_FMA
#else
#define ARGON_TARGET_AVX2_FMA __attribute__((target("avx2,fma")))
#endif

namespace argon::fir
{
namespace
{
using Kernel = void (*)(const Block &, float *);

Kernel chosenKernel()
{
    return cpuHasAvx2Fma() ? convolveAvx2 : convolveSse;
}

const Kernel kKernel = chosenKernel();
}  // namespace

#if defined(_MSC_VER)
// CPUID leaf 1: FMA, OSXSAVE and AVX; the OS saves the YMM registers (XCR0 bits 1, 2); leaf 7: AVX2
bool cpuHasAvx2Fma()
{
    constexpr int kFma = 1 << 12, kOsXsave = 1 << 27, kAvx = 1 << 28, kAvx2 = 1 << 5;
    constexpr unsigned long long kXmmYmmState = 6;
    int registers[4] = {};
    __cpuid(registers, 0);
    if (registers[0] < 7)
    {
        return false;
    }
    __cpuid(registers, 1);
    const int features = registers[2];
    if ((features & kFma) == 0 || (features & kOsXsave) == 0 || (features & kAvx) == 0 || (_xgetbv(0) & kXmmYmmState) != kXmmYmmState)
    {
        return false;
    }
    __cpuidex(registers, 7, 0);
    return (registers[1] & kAvx2) != 0;
}
#else
bool cpuHasAvx2Fma()
{
    __builtin_cpu_init();
    return __builtin_cpu_supports("avx2") && __builtin_cpu_supports("fma");
}
#endif

void convolve(const Block &block, float *output)
{
    kKernel(block, output);
}

// four registers of 4 outputs: per tap one broadcast and four loads for 16 products
void convolveSse(const Block &block, float *output)
{
    constexpr int kLanes = 4, kWidth = 4 * kLanes;
    for (int first = 0; first < block.frames; first += kWidth)
    {
        __m128 sum0 = _mm_setzero_ps(), sum1 = _mm_setzero_ps(), sum2 = _mm_setzero_ps(), sum3 = _mm_setzero_ps();
        const float *newest = block.input + first;
        for (int k = 0; k < block.tapCount; ++k)
        {
            const __m128 tap = _mm_set1_ps(block.taps[k]);
            const float *at = newest - k;
            sum0 = _mm_add_ps(sum0, _mm_mul_ps(tap, _mm_loadu_ps(at)));
            sum1 = _mm_add_ps(sum1, _mm_mul_ps(tap, _mm_loadu_ps(at + kLanes)));
            sum2 = _mm_add_ps(sum2, _mm_mul_ps(tap, _mm_loadu_ps(at + 2 * kLanes)));
            sum3 = _mm_add_ps(sum3, _mm_mul_ps(tap, _mm_loadu_ps(at + 3 * kLanes)));
        }
        _mm_storeu_ps(output + first, sum0);
        _mm_storeu_ps(output + first + kLanes, sum1);
        _mm_storeu_ps(output + first + 2 * kLanes, sum2);
        _mm_storeu_ps(output + first + 3 * kLanes, sum3);
    }
}

// four registers of 8 outputs: per tap one broadcast, four loads and four fused multiply-adds for 32 products
ARGON_TARGET_AVX2_FMA void convolveAvx2(const Block &block, float *output)
{
    constexpr int kLanes = 8, kWidth = 4 * kLanes;
    for (int first = 0; first < block.frames; first += kWidth)
    {
        __m256 sum0 = _mm256_setzero_ps(), sum1 = _mm256_setzero_ps(), sum2 = _mm256_setzero_ps(), sum3 = _mm256_setzero_ps();
        const float *newest = block.input + first;
        for (int k = 0; k < block.tapCount; ++k)
        {
            const __m256 tap = _mm256_broadcast_ss(block.taps + k);
            const float *at = newest - k;
            sum0 = _mm256_fmadd_ps(tap, _mm256_loadu_ps(at), sum0);
            sum1 = _mm256_fmadd_ps(tap, _mm256_loadu_ps(at + kLanes), sum1);
            sum2 = _mm256_fmadd_ps(tap, _mm256_loadu_ps(at + 2 * kLanes), sum2);
            sum3 = _mm256_fmadd_ps(tap, _mm256_loadu_ps(at + 3 * kLanes), sum3);
        }
        _mm256_storeu_ps(output + first, sum0);
        _mm256_storeu_ps(output + first + kLanes, sum1);
        _mm256_storeu_ps(output + first + 2 * kLanes, sum2);
        _mm256_storeu_ps(output + first + 3 * kLanes, sum3);
    }
    _mm256_zeroupper();
}

}  // namespace argon::fir
