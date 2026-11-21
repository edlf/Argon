// Argon: the race start's countdown digits (render\countdown_digits.hpp).
#include "render/countdown_digits.hpp"

#include "render/model_builder.hpp"

namespace
{
const char *const kDigitModels[CountdownDigits::kDigits] = {"countdown3", "countdown2", "countdown1", "countdown0"};
}  // namespace

void CountdownDigits::load(ModelBuilder &models, const std::string &gameFolder, argon::gfx::SetPool pool)
{
    if (loaded_)
    {
        return;
    }
    loaded_ = true;
    for (int k = 0; k < kDigits; ++k)
    {
        models.load(digits_[k], gameFolder + "/data/hudmodels/" + kDigitModels[k], ModelUse{.pool = pool, .prop = true});
    }
}
