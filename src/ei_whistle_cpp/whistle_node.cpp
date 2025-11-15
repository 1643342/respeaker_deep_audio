#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/int8.hpp>
#include <alsa/asoundlib.h>

#include <vector>
#include <string>
#include <memory>
#include <chrono>

// Edge Impulse SDK
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "model-parameters/model_metadata.h"

// Global pointer used by the EI signal callback
static std::vector<int16_t>* g_audio_buffer = nullptr;

static const int   SAMPLE_RATE          = 48000;
static const char* DEFAULT_ALSA_DEVICE  = "plughw:1,0";
static const float DEFAULT_THRESHOLD    = 0.8f;
static const char* DEFAULT_LABEL        = "whistle";

class WhistleNode : public rclcpp::Node {
public:
    WhistleNode() : Node("ei_whistle_cpp"), pcm_(nullptr) {
        pub_ = this->create_publisher<std_msgs::msg::Int8>("/whistle_detected", 10);

        // Parameters
        this->declare_parameter<std::string>("alsa_device", DEFAULT_ALSA_DEVICE);
        this->declare_parameter<double>("threshold", DEFAULT_THRESHOLD);
        this->declare_parameter<std::string>("label", DEFAULT_LABEL);

        alsa_device_  = this->get_parameter("alsa_device").as_string();
        threshold_    = static_cast<float>(this->get_parameter("threshold").as_double());
        target_label_ = this->get_parameter("label").as_string();

        // Init ALSA
        int err = snd_pcm_open(&pcm_, alsa_device_.c_str(), SND_PCM_STREAM_CAPTURE, 0);
        if (err < 0) {
            RCLCPP_FATAL(this->get_logger(),
                "Cannot open ALSA device '%s': %s",
                alsa_device_.c_str(), snd_strerror(err));
            throw std::runtime_error("ALSA open failed");
        }

        err = snd_pcm_set_params(
            pcm_,
            SND_PCM_FORMAT_S16_LE,
            SND_PCM_ACCESS_RW_INTERLEAVED,
            1,               // channels
            SAMPLE_RATE,
            1,               // soft resample
            48000);          // latency us
        if (err < 0) {
            RCLCPP_FATAL(this->get_logger(),
                "Cannot set ALSA params: %s", snd_strerror(err));
            throw std::runtime_error("ALSA params failed");
        }

        // Our sliding window buffer for EI input
        audio_buffer_.reserve(EI_CLASSIFIER_RAW_SAMPLE_COUNT);
        g_audio_buffer = &audio_buffer_;

        // Set up Edge Impulse signal struct
        signal_.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;

        
        signal_.get_data = [](size_t offset, size_t length, float *out) -> int {
            const auto &buf = *g_audio_buffer;

            if (buf.size() < EI_CLASSIFIER_RAW_SAMPLE_COUNT)
                return -1;

            size_t max_len = EI_CLASSIFIER_RAW_SAMPLE_COUNT;

            if (offset + length > max_len)
                length = max_len - offset;   // clamp instead of failing

            for (size_t i = 0; i < length; i++) {
                out[i] = buf[offset + i] / 32768.0f;
            }
            return 0;
        };


        // Timer to read audio + run classifier
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(10),
            std::bind(&WhistleNode::tick, this));

        RCLCPP_INFO(this->get_logger(),
            "WhistleNode started (device=%s, label=%s, threshold=%.2f, window=%d samples)",
            alsa_device_.c_str(), target_label_.c_str(), threshold_,
            EI_CLASSIFIER_RAW_SAMPLE_COUNT);
    }

    ~WhistleNode() override {
        if (pcm_) {
            snd_pcm_close(pcm_);
            pcm_ = nullptr;
        }
    }

private:
    void tick() {
        const size_t CHUNK = 512;
        std::vector<int16_t> buffer(CHUNK);

        // Capture audio
        snd_pcm_sframes_t frames = snd_pcm_readi(pcm_, buffer.data(), CHUNK);
        if (frames < 0) {
            snd_pcm_prepare(pcm_);  // recover from overrun/underrun
            return;
        }

        // Append new frames to our sliding window
        size_t valid_frames = static_cast<size_t>(frames);
        if (valid_frames > buffer.size()) {
            valid_frames = buffer.size();
        }

        audio_buffer_.insert(audio_buffer_.end(),
                             buffer.begin(),
                             buffer.begin() + valid_frames);

        // Keep only the last EI_CLASSIFIER_RAW_SAMPLE_COUNT samples
        if (audio_buffer_.size() > EI_CLASSIFIER_RAW_SAMPLE_COUNT) {
            size_t extra = audio_buffer_.size() - EI_CLASSIFIER_RAW_SAMPLE_COUNT;
            audio_buffer_.erase(audio_buffer_.begin(),
                                audio_buffer_.begin() + extra);
        }

        // Don't run classifier until we have a full window
        if (audio_buffer_.size() < EI_CLASSIFIER_RAW_SAMPLE_COUNT) {
            return;
        }

        // Run Edge Impulse classifier
        ei_impulse_result_t result;
        EI_IMPULSE_ERROR ei_err = run_classifier(&signal_, &result, false);
        if (ei_err != EI_IMPULSE_OK) {
            // RCLCPP_WARN(this->get_logger(), "run_classifier failed (%d)", ei_err);
            return;
        }

        float score = 0.0f;
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            const auto &c = result.classification[ix];
            if (!c.label) continue;
            std::string label(c.label);
            if (label == target_label_) {
                score = c.value;
                break;
            }
        }

        std_msgs::msg::Int8 msg;
        msg.data = (score >= threshold_) ? 1 : 0;
        pub_->publish(msg);
    }

    rclcpp::Publisher<std_msgs::msg::Int8>::SharedPtr pub_;
    rclcpp::TimerBase::SharedPtr timer_;
    snd_pcm_t* pcm_;
    signal_t signal_;   // from Edge Impulse headers

    // NEW: holds latest raw audio samples
    std::vector<int16_t> audio_buffer_;

    std::string alsa_device_;
    std::string target_label_;
    float threshold_;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<WhistleNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
