#pragma once

#include "embed/embed.hpp"

#include "esp_timer.h"

#include <cstdint>

/// Polls a button (active low) and sends a "button" event to the platform on each press.
class ButtonService : public embed::Service {
public:
    explicit ButtonService(int gpio) : gpio_(gpio) {}

    const char* serviceName() const override { return "Button"; }

    void start() override;
    void stop() override;

private:
    static void onTick(void* arg);
    void publishPress();

    int gpio_;
    esp_timer_handle_t timer_ = nullptr;
    uint8_t stableTicks_ = 0;
    bool pressed_ = false;
    uint32_t presses_ = 0;
};
