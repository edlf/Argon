// Argon: a FIR filter over a block of samples, the HRTF ears' inner loop (hrtf::EarFilter). Each tap is loaded once
// per pass over many outputs (16 with SSE, 32 with AVX2 and FMA when the CPU has them, picked once at start-up), so
// 512 taps cost about what the per-output dot product paid for 128.
#pragma once

namespace argon::fir
{

constexpr int kBlockFrames = 64;   // the most outputs one call makes

struct Block
{
    const float *taps = nullptr;
    int tapCount = 0;
    // the block's first sample: the tapCount - 1 samples before it are its history, and kBlockFrames samples from it
    // are readable (past `frames` they only feed outputs nobody reads)
    const float *input = nullptr;
    int frames = 0;   // 1..kBlockFrames
};

// output[f] = sum over k of taps[k] * input[f - k], for f < frames; output holds kBlockFrames floats
void convolve(const Block &block, float *output);
// the kernels convolve picks between (tests)
void convolveSse(const Block &block, float *output);
void convolveAvx2(const Block &block, float *output);   // only where cpuHasAvx2Fma()
bool cpuHasAvx2Fma();

}  // namespace argon::fir
