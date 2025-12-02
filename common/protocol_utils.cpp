#include <netinet/in.h>

#include "protocol.h"

void Protocol::insertUint16(std::uint16_t value) {
    uint16_t _value = htons(value);
    appendValue(_value);
}

void Protocol::insertUint32(std::uint32_t value) {
    uint32_t _value = htonl(value);
    appendValue(_value);
}

void Protocol::insertFloat(float value) {
    value *= 100;
    uint32_t int_value = static_cast<uint32_t>(value);
    insertUint32(int_value);
}

void Protocol::insertInt(int value) {
    uint32_t int_value = static_cast<uint32_t>(value);
    insertUint32(int_value);
}

uint16_t Protocol::exportUint16(const std::vector<uint8_t>& buffer, size_t& idx) {
    uint16_t net_value = readValue<uint16_t>(buffer, idx);
    return ntohs(net_value);
}

uint32_t Protocol::exportUint32(const std::vector<uint8_t>& buffer, size_t& idx) {
    uint32_t net_value = readValue<uint32_t>(buffer, idx);
    return ntohl(net_value);
}

float Protocol::exportFloat(const std::vector<uint8_t>& buffer, size_t& idx) {
    uint32_t int_value = exportUint32(buffer, idx);
    return static_cast<float>(int_value) / 100.0f;
}

int Protocol::exportInt(const std::vector<uint8_t>& buffer, size_t& idx) {
    uint32_t int_value = exportUint32(buffer, idx);
    return static_cast<int>(int_value);
}

bool Protocol::exportBoolFromNitroStatus(const std::vector<uint8_t>& buffer, size_t& idx) {
    uint8_t value = readValue<uint8_t>(buffer, idx);
    if (value == 0x07)
        return true;  // nitro activado
    else
        return false;  // nitro expirado
}

void Protocol::insertString(const std::string& str) {
    uint16_t len = static_cast<uint16_t>(str.size());
    insertUint16(len);
    for (char c : str) {
        buffer.push_back(static_cast<uint8_t>(c));
    }
}

void Protocol::insertPosition(const Position& pos) {
    buffer.push_back(pos.on_bridge ? 1 : 0);
    buffer.push_back(static_cast<int8_t>(pos.direction_x));
    buffer.push_back(static_cast<int8_t>(pos.direction_y));
    insertFloat(pos.new_X);
    insertFloat(pos.new_Y);
    insertFloat(pos.angle);
}
