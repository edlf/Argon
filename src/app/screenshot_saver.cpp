#include "app/screenshot_saver.hpp"

#include <chrono>
#include <cstdio>
#include <format>
#include <fstream>
#include <system_error>

#include "files/png.hpp"

namespace
{
std::vector<uint8_t> rgbOf(const argon::gfx::Picture &picture)
{
    const size_t count = size_t(picture.width) * picture.height;
    const size_t red = picture.bgra ? 2 : 0;
    const size_t blue = picture.bgra ? 0 : 2;
    std::vector<uint8_t> rgb(count * 3);
    for (size_t pixel = 0; pixel < count; ++pixel)
    {
        const uint8_t *in = &picture.pixels[pixel * 4];
        rgb[pixel * 3 + 0] = in[red];
        rgb[pixel * 3 + 1] = in[1];
        rgb[pixel * 3 + 2] = in[blue];
    }
    return rgb;
}

void writeScreenshot(const argon::gfx::Picture &picture, const std::filesystem::path &file)
{
    std::error_code ignored;
    std::filesystem::create_directories(file.parent_path(), ignored);
    const std::vector<uint8_t> rgb = rgbOf(picture);
    const bool ok = argon::writePng(file.string(), picture.width, picture.height, rgb.data());
    std::printf("%s %s (%ux%u)\n", ok ? "saved" : "FAILED to save", file.string().c_str(), picture.width, picture.height);
}

bool isFinished(const std::future<void> &save)
{
    return save.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

// argon_<date>_<time>.png, numbered when two fall in the same second. The file is made (empty) at once, so a second
// F12 in the same second picks the next number while the first is still being written
std::filesystem::path reserveScreenshotFile(const std::filesystem::path &folder)
{
    const auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
    const std::chrono::zoned_time local{std::chrono::current_zone(), now};
    const std::string stem = std::format("argon_{:%Y-%m-%d_%H-%M-%S}", local);
    std::filesystem::path file = folder / (stem + ".png");
    for (int copy = 2; std::filesystem::exists(file); ++copy)
    {
        file = folder / std::format("{}_{}.png", stem, copy);
    }
    std::error_code ignored;
    std::filesystem::create_directories(folder, ignored);
    std::ofstream{file, std::ios::binary};
    return file;
}
} // namespace

ScreenshotSaver::~ScreenshotSaver()
{
    finish();
}

void ScreenshotSaver::save(argon::gfx::Picture picture, std::filesystem::path file)
{
    forgetFinishedSaves();
    running_.push_back(std::async(std::launch::async, writeScreenshot, std::move(picture), std::move(file)));
}

void ScreenshotSaver::saveFrame(argon::gfx::Presenter &presenter, const std::filesystem::path &file)
{
    std::optional<argon::gfx::Picture> picture = presenter.takePicture();
    if (!picture)
    {
        std::printf("FAILED to save %s (no picture kept)\n", file.string().c_str());
        return;
    }
    save(std::move(*picture), file);
}

void ScreenshotSaver::request()
{
    requested_ = true;
}

bool ScreenshotSaver::wantsFrame() const
{
    return requested_ && !kept_;
}

void ScreenshotSaver::frameKept(const std::filesystem::path &folder)
{
    requested_ = false;
    kept_ = reserveScreenshotFile(folder);
}

void ScreenshotSaver::saveOnceDrawn(argon::gfx::Presenter &presenter)
{
    if (kept_ && presenter.pictureReady())
    {
        saveFrame(presenter, *kept_);
        kept_.reset();
    }
}

void ScreenshotSaver::finish()
{
    for (std::future<void> &save : running_)
    {
        save.wait();
    }
    running_.clear();
}

void ScreenshotSaver::forgetFinishedSaves()
{
    std::erase_if(running_, isFinished);
}
