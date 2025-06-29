#include <print>
#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/logger.hpp>
#include <rtc/rtc.hpp>
#include <oryx/argparse.hpp>
#include <oryx/enchantum.hpp>
#include <oryx/httplib.hpp>
#include <oryx/function_tracer.hpp>
#include <oryx/chrono/frame_rate_controller.hpp>

#include "ffmpeg_encoder.hpp"
#include "signal_handler.hpp"

using namespace oryx;
using std::println;

std::shared_ptr<rtc::PeerConnection> connection;
std::shared_ptr<rtc::Track> track;
std::shared_ptr<rtc::RtpPacketizationConfig> packet_config;
std::jthread worker;
bool is_display_enabled = false;
std::string window_name{"Sender"};

auto GenerateRGBFlowEffect(int frame_index, int width, int height) {
    cv::Mat yuv(height + height / 2, width, CV_8UC1);

    // Fill Y plane
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            yuv.at<uchar>(y, x) = x + y + frame_index * 3;
        }
    }

    // Fill U (Cb) plane
    for (int y = 0; y < height / 2; y++) {
        for (int x = 0; x < width / 2; x++) {
            yuv.at<uchar>(height + y, x * 2) = 128 + y + frame_index * 2;
        }
    }

    // Fill V (Cr) plane
    for (int y = 0; y < height / 2; y++) {
        for (int x = 0; x < width / 2; x++) {
            yuv.at<uchar>(height + y, x * 2 + 1) = 64 + x + frame_index * 5;
        }
    }

    // Convert to BGR for visualization
    cv::Mat bgr;
    cv::cvtColor(yuv, bgr, cv::COLOR_YUV420p2BGR);
    return bgr;
}

void FrameSender(std::stop_token stoken) {
    using namespace std::chrono;

    ORYX_TRACE_FUNCTION();
    FFMpegEncoder encoder;
    FFMpegEncoder::Config config;
    config.frame_width = 960;
    config.frame_height = 540;
    config.bitrate = 4000000;
    config.frame_rate = 30;

    auto result = encoder.Init(config);
    if (!result) {
        println("Encoder init failed with error: {}", result.error().what());
        return;
    }

    ByteVector encoded;
    u64 counter{};
    chrono::FrameRateController fr_controller{config.frame_rate};

    const cv::Scalar text_color(0, 0, 255);
    const cv::Point point(20, 20);
    while (!stoken.stop_requested()) {
        auto image = GenerateRGBFlowEffect(counter, config.frame_width, config.frame_height);
        cv::putText(image, std::format("{}", counter), point, cv::FONT_HERSHEY_SIMPLEX, 0.5, text_color, 2);

        if (is_display_enabled) {
            cv::imshow(window_name, image);
            cv::waitKey(1);
        }

        auto rc = encoder.Encode(image, encoded);
        if (rc == FFMpegEncoder::ErrorKind::kOk) {
            if (track->isClosed()) {
                println("Track is currently closed");
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }
            println("Sending frame: {}", counter);
            track->send(reinterpret_cast<const std::byte*>(encoded.data()), encoded.size());
            counter++;
        } else {
            println("Encoding failed with rc: {}", enchantum::to_string(rc));
        }
        fr_controller.Sleep();
    }
}

int main(int argc, char* argv[]) {
    auto sig_handler = SignalHandler::Instance();
    argparse::CLI cli(argc, argv);
    std::string endpoint{"localhost"};
    int port{8080};

    cli.VisitIfContains<std::string>("--endpoint", [&endpoint](std::string value) {
        println("Using endpoint provided by cli: {}", value);
        endpoint = value;
    });

    cli.VisitIfContains<int>("--port", [&port](int value) {
        println("Using port provided by cli: {}", port);
        port = value;
    });

    if (cli.Contains("--display")) {
        println("Display feature has been requested");
        is_display_enabled = true;
    }

    rtc::InitLogger(rtc::LogLevel::Info);
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_INFO);

    if (is_display_enabled) {
        try {
            cv::namedWindow(window_name, cv::WINDOW_AUTOSIZE);
        } catch (cv::Exception& exc) {
            println("Display is not supported. Disabling feature");
            is_display_enabled = false;
        }
    }

    httplib::Client client(endpoint, port);
    connection = std::make_shared<rtc::PeerConnection>();

    const rtc::SSRC ssrc = 42;
    const uint8_t payload_type = 100;

    rtc::Description::Video media("video", rtc::Description::Direction::SendOnly);
    media.addH264Codec(payload_type);
    media.addSSRC(ssrc, "video-send");

    packet_config = std::make_shared<rtc::RtpPacketizationConfig>(ssrc, "video", payload_type, 90000);
    track = connection->addTrack(media);
    track->onOpen([]() {
        println("Track opened starting frame sender");
        worker = std::jthread(FrameSender);
    });
    track->onClosed([]() {
        println("Track closed. Raising termination");
        SignalHandler::Instance()->RaiseTermination();
    });
    track->setMediaHandler(
        std::make_shared<rtc::H264RtpPacketizer>(rtc::NalUnit::Separator::LongStartSequence, packet_config));

    connection->setLocalDescription();

    std::string data = connection->localDescription().value();
    auto result = client.Post("/offer", data.data(), data.size(), "text/plain");
    if (!result) {
        println("Sending offer failed with error: {}", enchantum::to_string(result.error()));
        return 1;
    }

    auto response = result.value();
    println("Got answer setting remote description: {}", response.body);
    connection->setRemoteDescription(response.body);
    client.stop();

    sig_handler->Wait();
    println("Gracefully shutting down");

    worker.request_stop();
    worker.join();
    return 0;
}