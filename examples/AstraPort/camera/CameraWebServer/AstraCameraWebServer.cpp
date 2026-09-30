#include "AstraCameraWebServer.h"

AstraCameraWebServer::AstraCameraWebServer(AstraCameraPipeline &camera)
    : camera_(camera) {
}

AstraCameraWebServer::~AstraCameraWebServer() {
    stop();
}

bool AstraCameraWebServer::start() {
    return astraCameraWebServerStart(*this);
}

void AstraCameraWebServer::stop() {
    astraCameraWebServerStop(*this);
}

bool AstraCameraWebServer::isRunning() const {
    return cameraHttpd_ != nullptr && streamHttpd_ != nullptr;
}

