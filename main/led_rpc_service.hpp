#pragma once

#include "embed/embed.hpp"

/// RPC methods for a WS2812 strip: led_attach, led_detach, set_led.
/// The strip is bound to a GPIO at runtime, so the same firmware fits any wiring.
class LedRpcService : public embed::Service {
public:
    const char* serviceName() const override { return "LedRpc"; }

    void start() override;
    void stop() override {}
};
