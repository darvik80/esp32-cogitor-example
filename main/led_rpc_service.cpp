#include "led_rpc_service.hpp"

#include "cogitor_iot/cogitor_iot_service.hpp"
#include "embed_extra/led_strip_service.hpp"

#include "esp_log.h"

#include <cstdio>

using cogitor::iot::IotService;
using cogitor::iot::RpcParamDef;
using cogitor::iot::RpcParams;
using cogitor::iot::rpcBool;
using cogitor::iot::rpcInt;

namespace {

constexpr char TAG[] = "LedRpc";

embed::LedStripService* strip()
{
    return embed::ServiceRegistry::instance().getService<embed::LedStripService>();
}

int clampByte(int v)
{
    return v < 0 ? 0 : (v > 255 ? 255 : v);
}

void onLedAttach(IotService& iot, std::string_view id, const RpcParams& p, void*)
{
    int gpio = 0;
    int count = 0;
    if (!p.get("gpio", gpio) || !p.get("count", count) || gpio < 0 || count < 1 || count > 256) {
        iot.respondRpc(id, 400, "gpio and count (1..256) required");
        return;
    }
    // Attaching the same strip again just replaces it, so callers may repeat it freely.
    const auto brightness = static_cast<uint8_t>(clampByte(p.getInt("brightness", 0)));
    if (!strip()->attach(gpio, static_cast<uint16_t>(count), brightness)) {
        iot.respondRpc(id, 500, "attach failed: gpio busy or no free RMT channel");
        return;
    }
    char data[64];
    std::snprintf(data, sizeof(data), R"({"gpio":%d,"count":%d})", gpio, count);
    iot.respondRpc(id, 0, "ok", data);
}

void onLedDetach(IotService& iot, std::string_view id, const RpcParams& p, void*)
{
    int gpio = 0;
    if (!p.get("gpio", gpio)) {
        iot.respondRpc(id, 400, "gpio required");
        return;
    }
    if (!strip()->detach(gpio)) {
        iot.respondRpc(id, 404, "no strip on this gpio");
        return;
    }
    iot.respondRpc(id, 0, "ok");
}

void onSetLed(IotService& iot, std::string_view id, const RpcParams& p, void*)
{
    int gpio = 0;
    int offset = 0;
    int length = 0;
    if (!p.get("gpio", gpio) || !p.get("offset", offset) || !p.get("length", length)) {
        iot.respondRpc(id, 400, "gpio, offset and length required");
        return;
    }
    auto* leds = strip();
    if (!leds->attached(gpio)) {
        iot.respondRpc(id, 404, "no strip on this gpio, call led_attach first");
        return;
    }
    if (offset < 0 || length < 1 || offset + length > leds->ledCount(gpio)) {
        iot.respondRpc(id, 400, "led range out of bounds");
        return;
    }
    const bool on = p.getBool("on", true);
    const int r = on ? clampByte(p.getInt("r", 255)) : 0;
    const int g = on ? clampByte(p.getInt("g", 255)) : 0;
    const int b = on ? clampByte(p.getInt("b", 255)) : 0;
    if (!leds->setRange(gpio, static_cast<uint16_t>(offset), static_cast<uint16_t>(length),
                        static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b))) {
        iot.respondRpc(id, 500, "led update failed");
        return;
    }
    iot.respondRpc(id, 0, "ok");
}

} // namespace

void LedRpcService::start()
{
    auto* iot = embed::ServiceRegistry::instance().getService<IotService>();
    if (!iot || !strip()) {
        ESP_LOGE(TAG, "IotService and LedStripService must be created first");
        return;
    }

    // Parameter lists also feed the built-in rpc-list, which the dashboard
    // RPC console uses to build its forms.
    static constexpr RpcParamDef kAttach[] = {
        rpcInt("gpio"), rpcInt("count"), rpcInt("brightness", false, 0)};
    static constexpr RpcParamDef kDetach[] = {rpcInt("gpio")};
    static constexpr RpcParamDef kSetLed[] = {
        rpcInt("gpio"), rpcInt("offset"), rpcInt("length"),
        rpcInt("r", false, 255), rpcInt("g", false, 255), rpcInt("b", false, 255),
        rpcBool("on", false, true)};

    auto& rpc = iot->rpc();
    rpc.add("led_attach", kAttach, onLedAttach, nullptr, "Attach a WS2812 strip to a GPIO");
    rpc.add("led_detach", kDetach, onLedDetach, nullptr, "Detach the strip from a GPIO");
    rpc.add("set_led", kSetLed, onSetLed, nullptr, "Paint or turn off a range of LEDs");
    ESP_LOGI(TAG, "%u RPC methods registered", rpc.count());
}
