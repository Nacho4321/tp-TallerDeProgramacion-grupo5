#include <netinet/in.h>

#include "protocol.h"

void Protocol::insertUint16(std::uint16_t value) {
    uint16_t _value = htons(value);
    appendValue(_value);
}

uint16_t Protocol::exportUint16(const std::vector<uint8_t>& buffer, size_t& idx) {
    uint16_t net_value = readValue<uint16_t>(buffer, idx);
    return ntohs(net_value);
}

bool Protocol::exportBoolFromNitroStatus(const std::vector<uint8_t>& buffer, size_t& idx) {
    uint8_t value = readValue<uint8_t>(buffer, idx);
    if (value == 0x07)
        return true;  // nitro activado
    else
        return false;  // nitro expirado
}
