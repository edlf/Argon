#pragma once
// Screenshots written to PNG off the main thread, so taking one does not hold the frame. The saves still running
// are waited for by finish() (and the destructor). F12's: the next shown frame kept, then saved once the GPU has
// drawn it, so the main loop never waits for it.
#include <filesystem>
#include <future>
#include <optional>
#include <vector>

#include "gfx/presenter.hpp"

class ScreenshotSaver
{
public:
    ScreenshotSaver() = default;
    ScreenshotSaver(const ScreenshotSaver &) = delete;
    ScreenshotSaver &operator=(const ScreenshotSaver &) = delete;
    ~ScreenshotSaver();

    void save(argon::gfx::Picture picture, std::filesystem::path file);
    // the presenter's kept picture (read back here, written on another thread)
    void saveFrame(argon::gfx::Presenter &presenter, const std::filesystem::path &file);
    void finish();

    void request();
    bool wantsFrame() const;   // requested, and no kept frame waiting
    // the frame just shown was kept for it: its file reserved in folder now
    void frameKept(const std::filesystem::path &folder);
    void saveOnceDrawn(argon::gfx::Presenter &presenter);

private:
    void forgetFinishedSaves();

    std::vector<std::future<void>> running_;
    bool requested_ = false;
    std::optional<std::filesystem::path> kept_;   // the frame kept for F12, until the GPU has drawn it
};
