#include "button_service.hpp"
#include "led_rpc_service.hpp"

#include "cogitor_iot/cogitor_iot.hpp"
#include "embed/embed.hpp"
#include "embed_core/metrics_service.hpp"
#include "embed_core/mqtt_service.hpp"
#include "embed_core/nvs_store.hpp"
#include "embed_core/wifi_service.hpp"
#include "embed_extra/led_strip_service.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include <optional>

namespace {

constexpr char TAG[] = "Main";

constexpr auto kTopicStyle =
#ifdef CONFIG_EMBED_COGITOR_IOT_TOPIC_SHORT
    cogitor::iot::TopicStyle::Short;
#else
    cogitor::iot::TopicStyle::Full;
#endif

constexpr bool kUseTls =
#ifdef CONFIG_EMBED_COGITOR_IOT_USE_TLS
    true;
#else
    false;
#endif

} // namespace

extern "C" void app_main()
{
    if (embed::NvsStore::initFlash() != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed");
        return;
    }
    embed::EventLoop::instance().init();

    // The first boot copies the menuconfig values into NVS; later boots read
    // them from there, so a firmware built without the token keeps working.
    // MqttService keeps a reference: the credentials must outlive app_main.
    static std::optional<cogitor::iot::CogitorCredentials> creds =
        cogitor::iot::loadOrSeedCredentials(
            CONFIG_EMBED_COGITOR_IOT_PRODUCT_ID,
            CONFIG_EMBED_COGITOR_IOT_DEVICE_ID,
            CONFIG_EMBED_COGITOR_IOT_HOST,
            CONFIG_EMBED_COGITOR_IOT_ACCESS_TOKEN,
            kTopicStyle, kUseTls,
            static_cast<uint16_t>(CONFIG_EMBED_COGITOR_IOT_PORT));
    if (!creds || !creds->isValid()) {
        ESP_LOGE(TAG, "Set host, product, device and token in menuconfig → Embed Framework — Cogitor IoT");
        return;
    }

    // Services start in this order; each finds the ones it needs in start().
    auto& registry = embed::ServiceRegistry::instance();
    registry.createService<embed::WifiService>();
    registry.createService<embed::MetricsService>();             // heap, uptime, Wi-Fi RSSI every 10 s
    registry.createService<embed::MqttService>(*creds);          // connection and reconnects
    registry.createService<cogitor::iot::IotService>(*creds);    // platform topics, RPC dispatch
    registry.createService<cogitor::iot::MetricsTelemetryBridge>(); // metrics → telemetry
    registry.createService<cogitor::iot::DeviceInfo>();          // reported attributes on connect
    registry.createService<embed::LedStripService>();
    registry.createService<LedRpcService>();
    registry.createService<ButtonService>(CONFIG_EXAMPLE_BUTTON_GPIO);
    registry.startAll();

    ESP_LOGI(TAG, "%s → %s", creds->clientId(), creds->brokerUri());

    while (true) {
        vTaskDelay(portMAX_DELAY);
    }
}
