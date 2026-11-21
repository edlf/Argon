// Argon: the window's size, Argon Options' Resolution (settings.json "resolution": "1600x900"). The sizes offered are
// the monitor's video modes that fit on its desktop (window_mode.hpp monitorResolutions); none stored, or one the
// monitor does not offer, becomes the largest it offers at start-up (validResolution).
#pragma once
#include <algorithm>
#include <charconv>
#include <optional>
#include <string>
#include <vector>

namespace argon
{

struct Resolution
{
    int width = 1600, height = 900;

    bool operator==(const Resolution &other) const = default;
    bool fitsInside(const Resolution &other) const
    {
        return width <= other.width && height <= other.height;
    }
    long long pixels() const
    {
        return static_cast<long long>(width) * height;
    }
    std::string text() const   // "1600x900"
    {
        return std::to_string(width) + "x" + std::to_string(height);
    }
};

// "<width>x<height>", both positive; none for anything else
inline std::optional<Resolution> parseResolution(const std::string &text)
{
    Resolution parsed;
    const char *end = text.data() + text.size();
    const std::from_chars_result width = std::from_chars(text.data(), end, parsed.width);
    if (width.ec != std::errc() || width.ptr == end || *width.ptr != 'x')
    {
        return std::nullopt;
    }
    const std::from_chars_result height = std::from_chars(width.ptr + 1, end, parsed.height);
    if (height.ec != std::errc() || height.ptr != end || parsed.width <= 0 || parsed.height <= 0)
    {
        return std::nullopt;
    }
    return parsed;
}

// the most pixels; none when nothing is offered
inline std::optional<Resolution> largestResolution(const std::vector<Resolution> &offered)
{
    std::optional<Resolution> largest;
    for (const Resolution &size : offered)
    {
        if (!largest || size.pixels() > largest->pixels())
        {
            largest = size;
        }
    }
    return largest;
}

// wanted when it is offered; else (none stored, or one the monitor does not offer) the largest offered. Nothing
// offered (no monitor to ask): wanted, else 1600x900
inline Resolution validResolution(const std::optional<Resolution> &wanted, const std::vector<Resolution> &offered)
{
    if (wanted && std::find(offered.begin(), offered.end(), *wanted) != offered.end())
    {
        return *wanted;
    }
    return largestResolution(offered).value_or(wanted.value_or(Resolution()));
}

}  // namespace argon
