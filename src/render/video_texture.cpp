#include "render/video_texture.hpp"

#include <cstring>

namespace
{

// one BGRA level, opaque black
argon::TextureData blackPicture(const VideoSize &size)
{
    argon::TextureData picture;
    picture.width = size.width;
    picture.height = size.height;
    picture.format = argon::gfx::TextureFormat::Bgra8;
    const size_t bytes = size_t(size.width) * size.height * 4;
    picture.bytes.assign(bytes, 0);
    for (size_t alpha = 3; alpha < bytes; alpha += 4)
    {
        picture.bytes[alpha] = 255;
    }
    picture.levels.push_back({size.width, size.height, 0, bytes});
    return picture;
}

}  // namespace

void VideoTexture::create(argon::gfx::Device &device, argon::gfx::UiRenderer &ui, const VideoSize &size)
{
    destroy();
    device_ = &device;
    ui_ = &ui;
    size_ = size;
    texture_ = device.createTexture(blackPicture(size));
    image_ = ui.addImage(texture_, argon::gfx::SamplerMode::Clamp);
    const size_t bytes = size_t(device.uploadRowPitch(size.width)) * size.height;
    for (argon::gfx::BufferRef &staging : staging_)
    {
        staging = device.createStreamBuffer(argon::gfx::BufferUse::Upload, bytes);
    }
}

void VideoTexture::destroy()
{
    if (!device_)
    {
        return;
    }
    ui_->removeImage(image_);
    image_ = {};
    device_->destroyTexture(texture_);
    for (argon::gfx::BufferRef &staging : staging_)
    {
        device_->destroyBuffer(staging);
    }
    size_ = {};
}

void VideoTexture::upload(argon::gfx::CommandList &commands, int frameSlot, const uint8_t *bgrx)
{
    if (!ready())
    {
        return;
    }
    const uint32_t pitch = device_->uploadRowPitch(size_.width), rowBytes = size_.width * 4;
    auto *rows = static_cast<uint8_t *>(device_->mapped(staging_[frameSlot]));
    for (uint32_t y = 0; y < size_.height; ++y)
    {
        std::memcpy(rows + size_t(y) * pitch, bgrx + size_t(y) * rowBytes, rowBytes);
    }
    commands.copyToTexture({staging_[frameSlot], texture_.handle, size_.width, size_.height, pitch});
}
