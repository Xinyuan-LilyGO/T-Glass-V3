#include "AstraCameraWebServer.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <Arduino.h>
#include <esp_camera.h>
#include <esp_heap_caps.h>
#include <esp_http_server.h>

#include "../AstraCameraPipeline.h"
#include "camera_index.h"

namespace {

constexpr char kStreamBoundary[] = "123456789000000000000987654321";
constexpr char kJpegContentType[] = "image/jpeg";
constexpr std::size_t kJpegBufferSize = 96U * 1024U;

struct Query {
    char variable[32] = {};
    char value[16] = {};
};

bool readQuery(httpd_req_t *request, Query &query) {
    const size_t length = httpd_req_get_url_query_len(request);
    if (length == 0 || length >= 128) {
        return false;
    }

    char buffer[128] = {};
    if (httpd_req_get_url_query_str(request, buffer, sizeof(buffer)) != ESP_OK ||
        httpd_query_key_value(buffer,
                              "var",
                              query.variable,
                              sizeof(query.variable)) != ESP_OK ||
        httpd_query_key_value(buffer,
                              "val",
                              query.value,
                              sizeof(query.value)) != ESP_OK) {
        return false;
    }
    return true;
}

std::uint8_t *allocateJpegBuffer() {
    auto *buffer = static_cast<std::uint8_t *>(
        heap_caps_malloc(kJpegBufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (buffer == nullptr) {
        buffer = static_cast<std::uint8_t *>(std::malloc(kJpegBufferSize));
    }
    return buffer;
}

void releaseJpegBuffer(std::uint8_t *buffer) {
    if (buffer != nullptr) {
        heap_caps_free(buffer);
    }
}

AstraCameraWebServer *serverFromRequest(httpd_req_t *request) {
    return static_cast<AstraCameraWebServer *>(request->user_ctx);
}

esp_err_t sendCameraError(httpd_req_t *request) {
    return httpd_resp_send_err(request,
                               HTTPD_500_INTERNAL_SERVER_ERROR,
                               "camera frame unavailable");
}

esp_err_t indexHandler(httpd_req_t *request) {
    httpd_resp_set_type(request, "text/html");
    httpd_resp_set_hdr(request, "Content-Encoding", "gzip");
    httpd_resp_set_hdr(request, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(request,
                           reinterpret_cast<const char *>(index_ov2640_html_gz),
                           index_ov2640_html_gz_len);
}

esp_err_t captureHandler(httpd_req_t *request) {
    auto *server = serverFromRequest(request);
    if (server == nullptr) {
        return sendCameraError(request);
    }

    auto *jpeg = allocateJpegBuffer();
    astra_camera::FrameInfo info;
    const bool copied = jpeg != nullptr &&
                        server->cameraPipeline().copyJpegFrame(
                            jpeg, kJpegBufferSize, info, pdMS_TO_TICKS(200));
    if (!copied) {
        releaseJpegBuffer(jpeg);
        return sendCameraError(request);
    }

    httpd_resp_set_type(request, kJpegContentType);
    httpd_resp_set_hdr(request, "Content-Disposition", "inline; filename=capture.jpg");
    httpd_resp_set_hdr(request, "Access-Control-Allow-Origin", "*");
    char dimensions[32] = {};
    std::snprintf(dimensions,
                  sizeof(dimensions),
                  "%u x %u",
                  static_cast<unsigned>(info.width),
                  static_cast<unsigned>(info.height));
    httpd_resp_set_hdr(request, "X-Frame-Size", dimensions);
    const esp_err_t result = httpd_resp_send(
        request, reinterpret_cast<const char *>(jpeg), info.length);
    releaseJpegBuffer(jpeg);
    return result;
}

esp_err_t statusHandler(httpd_req_t *request) {
    auto *server = serverFromRequest(request);
    const sensor_t *sensor = esp_camera_sensor_get();
    const int sensorId = sensor == nullptr ? 0 : sensor->id.PID;
    const bool cameraReady = server != nullptr && server->cameraPipeline().available();

    char status[256] = {};
    std::snprintf(status,
                  sizeof(status),
                  "{\"camera\":%s,\"format\":\"JPEG\",\"width\":320,"
                  "\"height\":240,\"sensor\":%d,\"server\":%s}",
                  cameraReady ? "true" : "false",
                  sensorId,
                  server != nullptr && server->isRunning() ? "true" : "false");
    httpd_resp_set_type(request, "application/json");
    httpd_resp_set_hdr(request, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(request, status, HTTPD_RESP_USE_STRLEN);
}

esp_err_t controlHandler(httpd_req_t *request) {
    Query query;
    sensor_t *sensor = esp_camera_sensor_get();
    if (sensor == nullptr || !readQuery(request, query)) {
        return httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "invalid control");
    }

    const int value = std::atoi(query.value);
    int result = -1;
    if (std::strcmp(query.variable, "quality") == 0) {
        result = sensor->set_quality(sensor, value);
    } else if (std::strcmp(query.variable, "brightness") == 0) {
        result = sensor->set_brightness(sensor, value);
    } else if (std::strcmp(query.variable, "contrast") == 0) {
        result = sensor->set_contrast(sensor, value);
    } else if (std::strcmp(query.variable, "saturation") == 0) {
        result = sensor->set_saturation(sensor, value);
    } else if (std::strcmp(query.variable, "gainceiling") == 0) {
        result = sensor->set_gainceiling(sensor, static_cast<gainceiling_t>(value));
    } else if (std::strcmp(query.variable, "colorbar") == 0) {
        result = sensor->set_colorbar(sensor, value);
    } else if (std::strcmp(query.variable, "awb") == 0) {
        result = sensor->set_whitebal(sensor, value);
    } else if (std::strcmp(query.variable, "agc") == 0) {
        result = sensor->set_gain_ctrl(sensor, value);
    } else if (std::strcmp(query.variable, "aec") == 0) {
        result = sensor->set_exposure_ctrl(sensor, value);
    } else if (std::strcmp(query.variable, "hmirror") == 0) {
        result = sensor->set_hmirror(sensor, value);
    } else if (std::strcmp(query.variable, "vflip") == 0) {
        result = sensor->set_vflip(sensor, value);
    } else if (std::strcmp(query.variable, "special_effect") == 0) {
        result = sensor->set_special_effect(sensor, value);
    } else if (std::strcmp(query.variable, "wb_mode") == 0) {
        result = sensor->set_wb_mode(sensor, value);
    }

    if (result != 0) {
        return httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "unsupported control");
    }
    httpd_resp_set_type(request, "text/plain");
    return httpd_resp_send(request, "OK", HTTPD_RESP_USE_STRLEN);
}

esp_err_t streamHandler(httpd_req_t *request) {
    auto *server = serverFromRequest(request);
    if (server == nullptr) {
        return ESP_FAIL;
    }

    char contentType[96] = {};
    std::snprintf(contentType,
                  sizeof(contentType),
                  "multipart/x-mixed-replace;boundary=%s",
                  kStreamBoundary);
    httpd_resp_set_type(request, contentType);
    httpd_resp_set_hdr(request, "Access-Control-Allow-Origin", "*");

    auto *jpeg = allocateJpegBuffer();
    if (jpeg == nullptr) {
        return sendCameraError(request);
    }

    char header[128] = {};
    while (true) {
        astra_camera::FrameInfo info;
        if (!server->cameraPipeline().copyJpegFrame(
                jpeg, kJpegBufferSize, info, pdMS_TO_TICKS(200))) {
            break;
        }

        const int headerLength = std::snprintf(
            header,
            sizeof(header),
            "\r\n--%s\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
            kStreamBoundary,
            static_cast<unsigned>(info.length));
        if (headerLength <= 0 ||
            httpd_resp_send_chunk(request, header, headerLength) != ESP_OK ||
            httpd_resp_send_chunk(request,
                                  reinterpret_cast<const char *>(jpeg),
                                  info.length) != ESP_OK) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(30));
    }

    httpd_resp_send_chunk(request, nullptr, 0);
    releaseJpegBuffer(jpeg);
    return ESP_OK;
}

void registerUri(httpd_handle_t server,
                 const char *path,
                 httpd_method_t method,
                 esp_err_t (*handler)(httpd_req_t *),
                 AstraCameraWebServer *context) {
    httpd_uri_t uri = {};
    uri.uri = path;
    uri.method = method;
    uri.handler = handler;
    uri.user_ctx = context;
    httpd_register_uri_handler(server, &uri);
}

} // namespace

bool astraCameraWebServerStart(AstraCameraWebServer &server) {
    if (server.isRunning()) {
        return true;
    }
    if (!server.cameraPipeline().available()) {
        return false;
    }

    httpd_config_t cameraConfig = HTTPD_DEFAULT_CONFIG();
    cameraConfig.server_port = 80;
    cameraConfig.ctrl_port = 32768;
    cameraConfig.max_uri_handlers = 8;
    cameraConfig.lru_purge_enable = true;
    cameraConfig.stack_size = 8192;
    if (httpd_start(&server.cameraHttpd_, &cameraConfig) != ESP_OK) {
        server.cameraHttpd_ = nullptr;
        return false;
    }

    registerUri(server.cameraHttpd_, "/", HTTP_GET, indexHandler, &server);
    registerUri(server.cameraHttpd_, "/capture", HTTP_GET, captureHandler, &server);
    registerUri(server.cameraHttpd_, "/status", HTTP_GET, statusHandler, &server);
    registerUri(server.cameraHttpd_, "/control", HTTP_GET, controlHandler, &server);

    httpd_config_t streamConfig = HTTPD_DEFAULT_CONFIG();
    streamConfig.server_port = 81;
    streamConfig.ctrl_port = 32769;
    streamConfig.max_uri_handlers = 2;
    streamConfig.stack_size = 8192;
    if (httpd_start(&server.streamHttpd_, &streamConfig) != ESP_OK) {
        httpd_stop(server.cameraHttpd_);
        server.cameraHttpd_ = nullptr;
        server.streamHttpd_ = nullptr;
        return false;
    }
    registerUri(server.streamHttpd_, "/stream", HTTP_GET, streamHandler, &server);
    return true;
}

void astraCameraWebServerStop(AstraCameraWebServer &server) {
    if (server.streamHttpd_ != nullptr) {
        httpd_stop(server.streamHttpd_);
        server.streamHttpd_ = nullptr;
    }
    if (server.cameraHttpd_ != nullptr) {
        httpd_stop(server.cameraHttpd_);
        server.cameraHttpd_ = nullptr;
    }
}
