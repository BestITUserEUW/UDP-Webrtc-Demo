#pragma once

#include <cstddef>
#include <vector>
#include <functional>

#include <opencv2/core/mat.hpp>

#include <oryx/expected.hpp>

#include "image.hpp"
#include "ffmpeg_helpers.hpp"

namespace oryx {

class FFMpegEncoder {
public:
    enum class ErrorKind { kOk, kSendFailed, kEncodeFailed, kParseFailed, kNoFrame, kUnknown };

    struct Config {
        int frame_width;
        int frame_height;
        int frame_rate;
        int bitrate;
    };

    FFMpegEncoder() = default;
    ~FFMpegEncoder();

    auto Init(Config config) -> void_expected<Error>;
    void DeInit();

    auto Encode(const Image& decoded, ByteVector& encoded) -> ErrorKind;

private:
    AvCodecContextPtr codec_ctx_{};
    SwsContextPtr sws_ctx_{};
    AvFramePtr frame_yuv_{};
    AvPacketPtr encoded_pkt_{};
};

}  // namespace oryx