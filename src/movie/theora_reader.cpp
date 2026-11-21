#include "movie/theora_reader.hpp"

#include <cstdio>
#include <stdexcept>

#include <ogg/ogg.h>
#include <theora/theoradec.h>

#include "movie/ycbcr.hpp"
#include "platform/platform.hpp"

namespace
{

const long kReadChunkBytes = 64 * 1024;

// th_pixel_fmt: 4:2:0 = 0, 4:2:2 = 2, 4:4:4 = 3; bit 0 clear: half-width chroma, bit 1 clear: half-height chroma
YCbCrImage imageOf(const th_ycbcr_buffer planes, th_pixel_fmt format)
{
    YCbCrImage image;
    image.y = {planes[0].data, planes[0].stride};
    image.cb = {planes[1].data, planes[1].stride};
    image.cr = {planes[2].data, planes[2].stride};
    image.chromaShiftX = (format & 1) ? 0 : 1;
    image.chromaShiftY = (format & 2) ? 0 : 1;
    return image;
}

PictureRect pictureOf(const th_info &info)
{
    return {int(info.pic_x), int(info.pic_y), int(info.pic_width), int(info.pic_height)};
}

}  // namespace

struct TheoraVideoReader::Decoder
{
    std::FILE *file;
    ogg_sync_state sync{};
    ogg_stream_state stream{};
    bool streamFound = false;
    th_info info{};
    th_comment comment{};
    th_setup_info *setup = nullptr;
    th_dec_ctx *context = nullptr;
    ogg_packet firstFramePacket{};   // read while looking for the end of the headers
    bool firstFramePending = false;
    long frameIndex = 0;

    explicit Decoder(std::FILE *opened) : file(opened)
    {
        ogg_sync_init(&sync);
        th_info_init(&info);
        th_comment_init(&comment);
    }
    ~Decoder()
    {
        if (context)
        {
            th_decode_free(context);
        }
        th_setup_free(setup);
        th_comment_clear(&comment);
        th_info_clear(&info);
        if (streamFound)
        {
            ogg_stream_clear(&stream);
        }
        ogg_sync_clear(&sync);
        std::fclose(file);
    }
    Decoder(const Decoder &) = delete;
    Decoder &operator=(const Decoder &) = delete;

    bool readPage(ogg_page &page)
    {
        while (ogg_sync_pageout(&sync, &page) != 1)
        {
            char *buffer = ogg_sync_buffer(&sync, kReadChunkBytes);
            const size_t got = std::fread(buffer, 1, size_t(kReadChunkBytes), file);
            if (got == 0)
            {
                return false;
            }
            ogg_sync_wrote(&sync, long(got));
        }
        return true;
    }

    // the next packet of the Theora stream (ogg_stream_pagein refuses the pages of other streams)
    bool readPacket(ogg_packet &packet)
    {
        while (ogg_stream_packetout(&stream, &packet) != 1)
        {
            ogg_page page;
            if (!readPage(page))
            {
                return false;
            }
            ogg_stream_pagein(&stream, &page);
        }
        return true;
    }

    // the beginning-of-stream pages come first; the one whose first packet is a Theora header is the picture
    void findTheoraStream()
    {
        ogg_page page;
        while (readPage(page))
        {
            if (!ogg_page_bos(&page))
            {
                if (streamFound)
                {
                    ogg_stream_pagein(&stream, &page);
                }
                return;
            }
            tryTheoraStream(page);
        }
    }

    void tryTheoraStream(ogg_page &page)
    {
        ogg_stream_state candidate;
        ogg_stream_init(&candidate, ogg_page_serialno(&page));
        ogg_stream_pagein(&candidate, &page);
        ogg_packet packet;
        if (!streamFound && ogg_stream_packetout(&candidate, &packet) == 1 && th_decode_headerin(&info, &comment, &setup, &packet) > 0)
        {
            stream = candidate;
            streamFound = true;
            return;
        }
        ogg_stream_clear(&candidate);
    }

    // the rest of the three headers; the packet after them is the first frame's
    void readHeaders()
    {
        ogg_packet packet;
        while (readPacket(packet))
        {
            const int result = th_decode_headerin(&info, &comment, &setup, &packet);
            if (result > 0)
            {
                continue;
            }
            if (result < 0)
            {
                throw std::runtime_error("bad Theora header");
            }
            firstFramePacket = packet;
            firstFramePending = true;
            context = th_decode_alloc(&info, setup);
            if (!context)
            {
                throw std::runtime_error("unsupported Theora stream");
            }
            return;
        }
        throw std::runtime_error("the Theora headers end early");
    }

    // a damaged packet is skipped; an empty one repeats the last frame (TH_DUPFRAME)
    bool decodeNextFrame()
    {
        ogg_packet packet;
        for (;;)
        {
            if (firstFramePending)
            {
                packet = firstFramePacket;
                firstFramePending = false;
            }
            else if (!readPacket(packet))
            {
                return false;
            }
            const int result = th_decode_packetin(context, &packet, nullptr);
            if (result == 0 || result == TH_DUPFRAME)
            {
                return true;
            }
        }
    }
};

TheoraVideoReader::TheoraVideoReader(const std::filesystem::path &file)
{
    std::FILE *handle = platform::openForReading(file);
    if (!handle)
    {
        throw std::runtime_error("cannot open the file");
    }
    decoder_ = std::make_unique<Decoder>(handle);
    decoder_->findTheoraStream();
    if (!decoder_->streamFound)
    {
        throw std::runtime_error("no Theora stream");
    }
    decoder_->readHeaders();
}

TheoraVideoReader::~TheoraVideoReader() = default;

int TheoraVideoReader::width() const
{
    return int(decoder_->info.pic_width);
}

int TheoraVideoReader::height() const
{
    return int(decoder_->info.pic_height);
}

bool TheoraVideoReader::readFrame(VideoFrame &out)
{
    if (!decoder_->decodeNextFrame())
    {
        return false;
    }
    th_ycbcr_buffer planes;
    th_decode_ycbcr_out(decoder_->context, planes);
    const th_info &info = decoder_->info;
    out.seconds = double(decoder_->frameIndex++) * double(info.fps_denominator) / double(info.fps_numerator);
    out.width = width();
    out.height = height();
    out.bgrx.resize(size_t(out.width) * size_t(out.height) * 4);
    convertToBgrx(imageOf(planes, info.pixel_fmt), pictureOf(info), out.bgrx.data());
    return true;
}
