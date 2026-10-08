#include "button_service.hpp"

#include "cogitor_iot/cogitor_iot_service.hpp"

#include "driver/gpio.h"
#include "esp_log.h"

#include <cstdio>

namespace {

constexpr char TAG[] = "Button";
constexpr uint64_t kTickUs = 10'000;
constexpr uint8_t kDebounceTicks = 3; // 30 ms of a steady level

} // namespace

void ButtonService::start()
{
    const gpio_config_t io = {
        .pin_bit_mask = 1ULL << gpio_,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);

    const esp_timer_create_args_t args = {
        .callback = onTick,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "button",
        .skip_unhandled_events = true,
    };
    esp_timer_create(&args, &timer_);
    esp_timer_start_periodic(timer_, kTickUs);
    ESP_LOGI(TAG, "watching GPIO %d", gpio_);
}

void ButtonService::stop()
{
    if (timer_) {
        esp_timer_stop(timer_);
        esp_timer_delete(timer_);
        timer_ = nullptr;
    }
}

void ButtonService::onTick(void* arg)
{
    auto* self = static_cast<ButtonService*>(arg);
    const bool down = gpio_get_level(static_cast<gpio_num_t>(self->gpio_)) == 0;
    if (down == self->pressed_) {
        self->stableTicks_ = 0;
        return;
    }
    if (++self->stableTicks_ < kDebounceTicks) {
        return;
    }
    self->stableTicks_ = 0;
    self->pressed_ = down;
    if (down) {
        self->publishPress();
    }
}

void ButtonService::publishPress()
{
    auto* iot = embed::ServiceRegistry::instance().getService<cogitor::iot::IotService>();
    if (!iot) {
        return;
    }
    ++presses_;
    // The platform stamps the event with its own time when "ts" is missing.
    // The counter keeps two quick presses from being dropped as duplicates.
    char json[160];
    std::snprintf(json, sizeof(json),
                  R"({"type":"button","code":"PRESS","severity":"info",)"
                  R"("message":"Button pressed","data":{"gpio":%d,"presses":%lu}})",
                  gpio_, static_cast<unsigned long>(presses_));
    if (iot->publishEvents(json) < 0) {
        ESP_LOGW(TAG, "event not sent: MQTT is offline");
    }
}
