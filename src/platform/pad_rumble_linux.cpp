#include "platform/pad_rumble.hpp"

#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

namespace
{

using Clock = std::chrono::steady_clock;
const auto kRescanInterval = std::chrono::seconds(2);   // looking for a pad opens every input device

bool supportsRumble(int fd)
{
    const size_t bitsPerWord = 8 * sizeof(unsigned long);
    unsigned long features[FF_MAX / (8 * sizeof(unsigned long)) + 1]{};
    if (ioctl(fd, EVIOCGBIT(EV_FF, sizeof features), features) < 0)
    {
        return false;
    }
    return (features[FF_RUMBLE / bitsPerWord] >> (FF_RUMBLE % bitsPerWord)) & 1;
}

std::vector<std::string> eventDevices()
{
    std::vector<std::string> paths;
    std::error_code ec;
    for (const auto &entry : std::filesystem::directory_iterator("/dev/input", ec))
    {
        if (entry.path().filename().string().rfind("event", 0) == 0)
        {
            paths.push_back(entry.path().string());
        }
    }
    std::sort(paths.begin(), paths.end());
    return paths;
}

// the n-th input device with rumble, open for writing its effects; -1 when there is none (or no permission)
int openRumblePad(int index)
{
    int seen = 0;
    for (const std::string &path : eventDevices())
    {
        const int fd = open(path.c_str(), O_RDWR | O_CLOEXEC);
        if (fd < 0)
        {
            continue;
        }
        if (supportsRumble(fd) && seen++ == index)
        {
            return fd;
        }
        close(fd);
    }
    return -1;
}

}  // namespace

struct PadRumble::Device
{
    int ordinal = 0;   // which of the rumble pads
    int fd = -1;
    short effectId = -1;   // the uploaded rumble effect; the kernel removes it when the device is closed
    uint16_t sent[2]{};
    Clock::time_point lastScan{};

    ~Device()
    {
        release();
    }

    void release()
    {
        if (fd >= 0)
        {
            close(fd);
        }
        fd = -1;
        effectId = -1;
        sent[0] = sent[1] = 0;
    }

    bool connect()
    {
        if (fd >= 0)
        {
            return true;
        }
        const Clock::time_point now = Clock::now();
        if (now - lastScan < kRescanInterval)
        {
            return false;
        }
        lastScan = now;
        fd = openRumblePad(ordinal);
        return fd >= 0;
    }

    // replay length 0: plays until changed or the device is closed
    bool upload(uint16_t lowFrequency, uint16_t highFrequency)
    {
        ff_effect effect{};
        effect.type = FF_RUMBLE;
        effect.id = effectId;
        effect.u.rumble.strong_magnitude = lowFrequency;
        effect.u.rumble.weak_magnitude = highFrequency;
        if (ioctl(fd, EVIOCSFF, &effect) < 0)
        {
            return false;
        }
        if (effectId >= 0)
        {
            return true;
        }
        effectId = effect.id;
        input_event play{};
        play.type = EV_FF;
        play.code = uint16_t(effectId);
        play.value = 1;
        return write(fd, &play, sizeof play) == ssize_t(sizeof play);
    }
};

PadRumble::PadRumble()
{
    for (int pad = 0; pad < kPads; ++pad)
    {
        devices_[pad] = std::make_unique<Device>();
        devices_[pad]->ordinal = pad;
    }
}

PadRumble::~PadRumble() = default;

void PadRumble::set(int pad, uint16_t lowFrequency, uint16_t highFrequency)
{
    if (pad < 0 || pad >= kPads)
    {
        return;
    }
    Device &device = *devices_[pad];
    if (lowFrequency == device.sent[0] && highFrequency == device.sent[1])
    {
        return;
    }
    if (!device.connect())
    {
        return;
    }
    if (!device.upload(lowFrequency, highFrequency))
    {   // unplugged: look again later
        device.release();
        return;
    }
    device.sent[0] = lowFrequency;
    device.sent[1] = highFrequency;
}
