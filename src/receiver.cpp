#include <print>
#include <chrono>
#include <memory>
#include <thread>
#include <mutex>
#include <string>

#include <rtc/rtc.hpp>
#include <opencv2/core/utils/logger.hpp>
#include <opencv2/opencv.hpp>
#include <oryx/enchantum.hpp>
#include <oryx/httplib.hpp>
#include <oryx/spsc_queue.hpp>
#include <oryx/function_tracer.hpp>
#include <oryx/argparse.hpp>
#include <oryx/chrono/stopwatch.hpp>

#include "ffmpeg_decoder.hpp"
#include "signal_handler.hpp"

using namespace oryx;
using std::println;

httplib::Server server;
std::shared_ptr<rtc::PeerConnection> connection;
std::shared_ptr<rtc::Track> track;
std::jthread http_worker;
std::jthread decode_worker;
folly::ProducerConsumerQueue<ByteVector> queue{32};
std::condition_variable has_data_cv;
bool is_display_enabled = false;
std::string window_name{"Receiver"};

void HttpWorker(std::stop_token) {
    ORYX_TRACE_FUNCTION();
    server.listen("0.0.0.0", 8080);
}

void DecodeWorker(std::stop_token stoken) {
    ORYX_TRACE_FUNCTION();
    std::stop_callback scb(stoken, [&cv = has_data_cv]() { cv.notify_all(); });
    FFMpegDecoder decoder{};
    Image image{};
    std::mutex mtx{};

    const auto result = decoder.Init();
    if (!result) {
        println("Init decoder failed with error: {}", result.error().what());
        return;
    }

    while (!stoken.stop_requested()) {
        auto encoded = queue.frontPtr();
        if (!encoded) {
            std::unique_lock lck(mtx);
            has_data_cv.wait(lck);
            continue;
        }

        auto rc = decoder.Decode(*encoded, image);
        queue.popFront();
        if (rc != FFMpegDecoder::ErrorKind::kOk) {
            if (rc != FFMpegDecoder::ErrorKind::kNoFrame)
                println("Failed to decode image: {}", enchantum::to_string(rc));
            continue;
        }

        if (is_display_enabled) {
            cv::imshow(window_name, image);
            cv::waitKey(1);
        }
    }
}

void OnFrame(rtc::binary binary, rtc::FrameInfo info) {
    static chrono::Stopwatch sw;

    if (binary.empty()) {
        println("Empty frame received");
        return;
    }

    auto data = reinterpret_cast<uint8_t *>(binary.data());
    if (queue.write(ByteVector(data, data + binary.size()))) {
        has_data_cv.notify_all();
    }

    println("Frame Latency: {} Size: {} Timestamp: {}", sw.ElapsedMs(), binary.size(), info.timestamp);
    sw.Reset();
}

void StartHttpServer() {
    println("Setting up http server");
    server.new_task_queue = [] { return new httplib::ThreadPool(2); };
    server.Options("/offer", [](const httplib::Request &req, httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        res.status = 204;  // No Content
    });

    server.Post("/offer", [pc = connection](const httplib::Request &req, httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        rtc::Description offer(req.body, rtc::Description::Type::Offer);

        println("Incoming request from {}:{} with offer {}", req.remote_addr, req.remote_port, req.body);
        pc->setRemoteDescription(offer);
        auto description = pc->localDescription();
        if (!description) {
            println("No local description available. Discarding");
            return;
        }

        auto answer = std::string(description.value());

        res.set_content(answer, "application/sdp");
        println("Responding offer with description {}", answer);
    });

    http_worker = std::jthread(HttpWorker);
}

void StartPeerConnection() {
    println("Setting up peer connection");
    rtc::Configuration config{};
    connection = std::make_shared<rtc::PeerConnection>(config);
    connection->onStateChange([](auto state) { println("Connection State: {}", enchantum::to_string(state)); });
    connection->onGatheringStateChange([](auto state) { println("Gathering State: {}", enchantum::to_string(state)); });
    connection->onTrack([](std::shared_ptr<rtc::Track> track_offer) {
        auto desc = track_offer->description();
        println("Track direction: {} type: {}", enchantum::to_string(desc.direction()), desc.type().c_str());

        track_offer->setMediaHandler(std::make_shared<rtc::H264RtpDepacketizer>());
        track_offer->chainMediaHandler(std::make_shared<rtc::RtcpReceivingSession>());
        track_offer->onFrame(OnFrame);
        track = track_offer;
    });
}

int main(int argc, char *argv[]) {
    auto sig_handler = SignalHandler::Instance();
    auto cli = argparse::CLI(argc, argv);

    if (cli.Contains("--display")) {
        println("Display feature has been requested");
        is_display_enabled = true;
    }

    rtc::InitLogger(rtc::LogLevel::Info);
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_INFO);

    if (is_display_enabled) {
        try {
            cv::namedWindow(window_name, cv::WINDOW_AUTOSIZE);
        } catch (cv::Exception &exc) {
            println("Display is not supported. Disabling feature");
            is_display_enabled = false;
        }
    }

    StartPeerConnection();
    StartHttpServer();
    decode_worker = std::jthread(&DecodeWorker);

    sig_handler->Wait();
    println("Gracefully shutting down");

    server.stop();
    http_worker.request_stop();
    decode_worker.request_stop();
    http_worker.join();
    decode_worker.join();
    return 0;
}