#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/int8.hpp>
#include <alsa/asoundlib.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include "ei_whistle_cpp/ring_buffer.hpp"

// Edge Impulse SDK (from ei-sdk/)
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "model-parameters/model_metadata.h"

static const int SAMPLE_RATE = 16000;
static const char* ALSA_DEVICE = "plughw:1,0"; // adjust via parameter if needed
static const float THRESH = 0.80f;             // default threshold
static const char* TARGET_LABEL = "whistle";   // default label

class WhistleNode : public rclcpp::Node {
public:
    WhistleNode() : Node("ei_whistle_cpp") {
        pub_ = create_publisher<std_msgs::msg::Int8>("/whistle_detected", 10);

        // parameters
        this->declare_parameter<std::string>("alsa_device", ALSA_DEVICE);
        this->declare_parameter<double>("threshold", THRESH);
        this->declare_parameter<std::string>("label", TARGET_LABEL);

        alsa_device_ = this->get_parameter("alsa_device").as_string();
        threshold_ = static_cast<float>(this->get_parameter("threshold").as_double());
        target_label_ = this->get_parameter("label").as_string();

        // ALSA init
        if (snd_pcm_open(&pcm_, alsa_device_.c_str(), SND_PCM_STREAM_CAPTURE, 0) < 0) {
            RCLCPP_FATAL(get_logger(), "Cannot open ALSA device %s", alsa_device_.c_str());
            throw std::runtime_error("ALSA open failed");
        }
        snd_pcm_set_params(pcm_, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
                           1, SAMPLE_RATE, 1, 20000);

        // ring buffer for EI window
        ring_ = std::make_unique<RingBuffer<int16_t>>(EI_CLASSIFIER_RAW_SAMPLE_COUNT);

        // setup signal callback
        signal_.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
        signal_.get_data = [](size_t offset, size_t length, float *out) -> int {
            // static storage to access ring_
            extern RingBuffer<int16_t>* g_ring;
            auto &buf = g_ring->data();
            size_t head = g_ring->head();
            for (size_t i = 0; i < length; i++) {
                size_t idx = (head + offset + i) % buf.size();
                out[i] = buf[idx] / 32768.0f;
            }
            return 0;
        };
        g_ring = ring_.get();

        timer_ = create_wall_timer(std::chrono::milliseconds(10),
                 std::bind(&WhistleNode::tick, this));
        RCLCPP_INFO(get_logger(), "Whistle detector started (label=%s, thresh=%.2f, device=%s)",
                    target_label_.c_str(), threshold_, alsa_device_.c_str());
    }

    ~WhistleNode() override {
        if (pcm_) snd_pcm_close(pcm_);
    }

private:
    void tick() {
        const size_t CHUNK = 512;
        std::vector<int16_t> cap(CHUNK);

        // read audio
        auto got = snd_pcm_readi(pcm_, cap.data(), CHUNK);
        if (got <= 0) { snd_pcm_prepare(pcm_); return; }
        ring_->push(cap.data(), static_cast<size_t>(got));

        // run EI classifier
        ei_impulse_result_t result;
        EI_IMPULSE_ERROR err = run_classifier(&signal_, &result, false);
        if (err != EI_IMPULSE_OK) return;

        // find target score
        float s = 0.f;
        for (size_t i = 0; i < result.classification_count; i++) {
            if (result.classification[i].label &&
                target_label_ == result.classification[i].label) {
                s = result.classification[i].value;
                break;
            }
        }

        std_msgs::msg::Int8 msg;
        msg.data = (s >= threshold_) ? 1 : 0;
        pub_->publish(msg);
    }

    rclcpp::Publisher<std_msgs::msg::Int8>::SharedPtr pub_;
    rclcpp::TimerBase::SharedPtr timer_;
    snd_pcm_t* pcm_ = nullptr;
    std::unique_ptr<RingBuffer<int16_t>> ring_;
    signal_t signal_;
    std::string alsa_device_;
    float threshold_;
    std::string target_label_;
};

// static ring pointer used in signal callback
RingBuffer<int16_t>* g_ring = nullptr;

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<WhistleNode>());
    rclcpp::shutdown();
    return 0;
}
