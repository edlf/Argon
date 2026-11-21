// Argon: the mesh draws' records (mesh_draw_records.hpp).
#include "render/mesh_draw_records.hpp"

#include <stdexcept>
#include <string>

void MeshDrawRecords::create(argon::gfx::Device &device, int frames)
{
    for (int frame = 0; frame < frames; ++frame)
    {
        buffers_.push_back(device.createStreamBuffer(argon::gfx::BufferUse::Storage, size_t(kCapacity) * sizeof(MeshDraw)));
        mapped_.push_back(static_cast<MeshDraw *>(device.mapped(buffers_.back())));
    }
}

void MeshDrawRecords::destroy(argon::gfx::Device &device)
{
    for (argon::gfx::BufferRef &buffer : buffers_)
    {
        device.destroyBuffer(buffer);
    }
    buffers_.clear();
    mapped_.clear();
    current_ = nullptr;
}

void MeshDrawRecords::begin(int frame)
{
    current_ = mapped_.at(size_t(frame));
    count_ = 0;
}

uint32_t MeshDrawRecords::add(const MeshDraw &draw)
{
    if (count_ >= kCapacity)
    {
        throw std::runtime_error("a frame records more than " + std::to_string(kCapacity) + " mesh draws");
    }
    current_[count_] = draw;
    return count_++;
}

std::span<const argon::gfx::BufferRef> MeshDrawRecords::buffers() const
{
    return buffers_;
}
