#include "platform/pad_rumble.hpp"

#include <windows.h>
#include <Xinput.h>

namespace
{

// the XInput user index of the n-th connected pad; -1 when fewer are connected
int connectedPad(int index)
{
    int seen = 0;
    for (DWORD user = 0; user < XUSER_MAX_COUNT; ++user)
    {
        XINPUT_STATE state;
        if (XInputGetState(user, &state) == ERROR_SUCCESS && seen++ == index)
        {
            return int(user);
        }
    }
    return -1;
}

void vibrate(int pad, uint16_t lowFrequency, uint16_t highFrequency)
{
    XINPUT_VIBRATION vibration{lowFrequency, highFrequency};
    XInputSetState(DWORD(pad), &vibration);
}

}  // namespace

struct PadRumble::Device
{
    int pad = -1;   // the XInput user index the speeds went to
    uint16_t sent[2]{};

    ~Device()
    {
        if (pad >= 0)
        {
            vibrate(pad, 0, 0);
        }
    }

    void follow(int connected)
    {
        if (connected != pad && pad >= 0)
        {
            vibrate(pad, 0, 0);
            sent[0] = sent[1] = 0;
        }
        pad = connected;
    }

    void send(uint16_t lowFrequency, uint16_t highFrequency)
    {
        if (pad < 0 || (lowFrequency == sent[0] && highFrequency == sent[1]))
        {
            return;
        }
        vibrate(pad, lowFrequency, highFrequency);
        sent[0] = lowFrequency;
        sent[1] = highFrequency;
    }
};

PadRumble::PadRumble()
{
    for (auto &device : devices_)
    {
        device = std::make_unique<Device>();
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
    if (lowFrequency == 0 && highFrequency == 0 && device.sent[0] == 0 && device.sent[1] == 0)
    {
        return;   // already still: no need to look for the pad
    }
    device.follow(connectedPad(pad));
    device.send(lowFrequency, highFrequency);
}
