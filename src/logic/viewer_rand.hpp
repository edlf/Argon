// Argon: the viewer's own rand (the C standard's sample generator, seed 12345, 15 bits a call). The spray, the trails'
// jitter, the particles and the AI's route seed draw from one stream, so their order matters to what a race shows.
#pragma once
#include <cstdint>

namespace argon
{

class ViewerRand
{
  public:
    uint32_t next()
    {
        state_ = state_ * 1103515245u + 12345u;
        return (state_ >> 16) & 0x7fff;
    }

  private:
    uint32_t state_ = 12345;
};

}  // namespace argon
