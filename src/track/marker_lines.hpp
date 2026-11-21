// Argon: the editor's markings as line segments (a viewer extra, drawn by render\editor_markings.cpp): the start grid,
// the AI route, the pickups, the sound emitters with their MINDIST / MAXDIST rings, the route check's problems.
#pragma once
#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "audio/audio.hpp"
#include "craft/drive.hpp"
#include "track/trackdata.hpp"

struct MarkerVertex
{
    glm::vec3 pos, colour;
};

class MarkerLines
{
  public:
    void addStarts(const argon::TrackData &track);
    void addAiRoute(const argon::TrackData &track);
    void addPickups(const argon::TrackData &track);
    void addEmitters(const std::vector<argon::SoundEmitter> &emitters);
    void addIssues(const std::vector<argon::RouteIssue> &issues);

    uint32_t count() const
    {
        return uint32_t(vertices_.size());
    }
    const std::vector<MarkerVertex> &vertices() const
    {
        return vertices_;
    }

  private:
    void line(const glm::vec3 &from, const glm::vec3 &to, const glm::vec3 &colour);
    void cross(const glm::vec3 &at, float size, const glm::vec3 &colour);   // x and z arms, and up twice as long
    void ring(const glm::vec3 &centre, float radius, const glm::vec3 &colour);

    std::vector<MarkerVertex> vertices_;
};
