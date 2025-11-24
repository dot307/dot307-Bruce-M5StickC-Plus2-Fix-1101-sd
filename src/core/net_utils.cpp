#include "net_utils.h"
#include <ArduinoJson.h>
#include <ESPping.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <sstream>


bool internetConnection() {
    if (Ping.ping("api.mxin.moe")) { return true; }
    return Ping.ping(IPAddress(114, 114, 114, 114));
}
const String PROXY_HOST = "http://18.218.8.189:5000";
String getManufacturer(const String &mac) {
    // 0. 基础检查：没连网就别费劲了
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[MAC] No WiFi connection");
        return "NO_WIFI";
    }

    // 1. 拼接 URL：直接找你的 AWS 代理
    // 格式: http://18.218.8.189:5000/query?mac=AA:BB:CC:DD:EE:FF
    String url = PROXY_HOST + "/query?mac=" + mac;

    Serial.printf("[MAC] Proxy Request: %s\n", url.c_str());

    // 2. 创建最普通的 HTTP 客户端 (非加密，极速，不占 RAM)
    WiFiClient client;
    HTTPClient http;

    // 设置超时 (3秒足够了，EC2 响应很快)
    http.setTimeout(3000);

    // 3. 发起连接
    // 注意：这里用的是 http.begin(client, url)
    if (!http.begin(client, url)) {
        Serial.println("[MAC] Connect Proxy Failed");
        return "PROXY_ERR";
    }

    // 4. 发送 GET 请求
    int httpCode = http.GET();
    Serial.printf("[MAC] Code: %d\n", httpCode);

    String result = "UNKNOWN";

    if (httpCode == HTTP_CODE_OK) { // Code 200
        // 成功！EC2 已经把 "Huizhou Gaoshengda..." 这种纯文本发回来了
        result = http.getString();
        result.trim(); // 去掉可能存在的换行符
        Serial.printf("[MAC] Vendor: %s\n", result.c_str());

        // 防错截断：防止返回太长撑爆屏幕
        if (result.length() > 32) {
            result = result.substring(0, 32);
        }
    } else {
        // 打印错误原因
        Serial.printf("[MAC] HTTP Err: %s\n", http.errorToString(httpCode).c_str());

        if (httpCode == 500) result = "SERVER_ERR"; // Python 脚本报错
        if (httpCode == 404) result = "NOT_FOUND";  // 数据库里没这个 MAC
    }

    http.end();
    return result;
}

String MAC(uint8_t *data) {
    char macStr[18];
    snprintf(
        macStr,
        sizeof(macStr),
        "%02x:%02x:%02x:%02x:%02x:%02x",
        data[0],
        data[1],
        data[2],
        data[3],
        data[4],
        data[5]
    );

    return macStr;
}

void stringToMAC(const std::string &macStr, uint8_t MAC[6]) {
    std::stringstream ss(macStr);
    unsigned int temp;
    for (int i = 0; i < 6; ++i) {
        char delimiter;
        ss >> std::hex >> temp;
        MAC[i] = static_cast<uint8_t>(temp);
        ss >> delimiter;
    }
}

// Função para converter IP para string
String ipToString(const uint8_t *ip) {
    return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
}

// Função para converter MAC para string
String macToString(const uint8_t *mac) {
    char buf[18];
    sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}
