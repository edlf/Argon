// Argon: the race start's countdown digits, data\hudmodels\countdown3 .. countdown0 (the 3, 2, 1 and GO models),
// read the first time a countdown starts and kept (App's countdown draws them: app_intro.cpp).
#pragma once
#include <string>

#include "gfx/gfx_types.hpp"
#include "render/gpu_model.hpp"

class ModelBuilder;

class CountdownDigits
{
  public:
    static constexpr int kDigits = 4;

    // once: the four models into the props' pool
    void load(ModelBuilder &models, const std::string &gameFolder, argon::gfx::SetPool pool);
    // the k'th model from 3 down to GO
    const Track &digit(int index) const
    {
        return digits_[index];
    }
    Track &digit(int index)
    {
        return digits_[index];
    }

  private:
    bool loaded_ = false;
    Track digits_[kDigits];
};
