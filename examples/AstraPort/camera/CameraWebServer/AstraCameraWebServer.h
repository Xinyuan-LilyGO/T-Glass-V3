#pragma once

#include <esp_http_server.h>

#include "../AstraCameraPipeline.h"

class AstraCameraWebServer {
public:
    explicit AstraCameraWebServer(AstraCameraPipeline &camera);
    ~AstraCameraWebServer();

    AstraCameraWebServer(const AstraCameraWebServer &) = delete;
    AstraCameraWebServer &operator=(const AstraCameraWebServer &) = delete;

    bool start();
    void stop();
    bool isRunning() const;

    AstraCameraPipeline &cameraPipeline() {
        return camera_;
    }

private:
    friend bool astraCameraWebServerStart(AstraCameraWebServer &server);
    friend void astraCameraWebServerStop(AstraCameraWebServer &server);

    AstraCameraPipeline &camera_;
    httpd_handle_t cameraHttpd_ = nullptr;
    httpd_handle_t streamHttpd_ = nullptr;
};

bool astraCameraWebServerStart(AstraCameraWebServer &server);
void astraCameraWebServerStop(AstraCameraWebServer &server);
