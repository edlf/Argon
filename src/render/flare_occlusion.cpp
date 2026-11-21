// Argon: the flares' occlusion test points (flare_occlusion.hpp).
#include "render/flare_occlusion.hpp"

namespace argon::flare
{

namespace
{

// the game's rand() for the occlusion points (0xa63b0): its next 12 bits as 0..1
class GameRand
{
  public:
    float next()
    {
        seed_ = seed_ * 1103515245u + 12345u;
        return float((seed_ >> 16) & 0xfff) / 4095.f;
    }
    bool lastHadBit16() const
    {
        return (seed_ & 0x10000) != 0;
    }

  private:
    uint32_t seed_ = 0x2545f491u;
};

}  // namespace

std::vector<glm::vec3> testPoints()
{
    std::vector<glm::vec3> points;
    GameRand rand;
    while (points.size() < size_t(kTestPoints))
    {
        glm::vec3 p(rand.next(), rand.next(), rand.next());
        if (rand.lastHadBit16())
        {
            p.x = -p.x;
        }
        p.y = rand.next() < 0.5f ? -p.y : p.y;
        p.z = rand.next() < 0.5f ? -p.z : p.z;
        if (glm::dot(p, p) > 1.f)
        {
            continue;
        }
        points.push_back(p);
    }
    return points;
}

}  // namespace argon::flare
