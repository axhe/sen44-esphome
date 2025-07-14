#include "sen44.h"
#include "esphome/core/log.h"

namespace esphome {
namespace sen44_uart {

static const char *const TAG = "sen44_uart";
static const uint8_t SEN44_UART_REQUEST_LENGTH = 8;
static const uint8_t SEN44_UART_RESPONSE_LENGTH = 26;

void SEN44UARTComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up SEN44 UART...");
}

void SEN44UARTComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "SEN44 UART:");
  this->check_uart_settings(115200);
}

void SEN44UARTComponent::loop() {
  std::vector<unsigned char> data;
  while (this->available()) {
    uint8_t byte;
    this->read_byte(&byte);
    data.push_back(byte);
  }

  if (!data.empty()) {
    this->handle_data(data);
  }
}

void SEN44UARTComponent::update() {
  this->request_data();
}

float SEN44UARTComponent::get_setup_priority() const {
  return setup_priority::DATA;
}

uint8_t SEN44UARTComponent::crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0xFF;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if ((crc & 0x80) != 0) {
        crc = (uint8_t) ((crc << 1) ^ 0x31);
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

uint16_t SEN44UARTComponent::get_16_bit_uint(uint8_t start_index, const std::vector<unsigned char> &data) {
  return (uint16_t(data[start_index]) << 8) | uint16_t(data[start_index + 1]);
}

void SEN44UARTComponent::handle_data(const std::vector<unsigned char> &data) {
  if (data.size() != SEN44_UART_RESPONSE_LENGTH) {
    ESP_LOGW(TAG, "Invalid data length: %d", data.size());
    return;
  }

  if (data[0] != 0x7E || data[SEN44_UART_RESPONSE_LENGTH - 1] != 0x7E) {
    ESP_LOGW(TAG, "Invalid start or end byte");
    return;
  }

  uint8_t crc = this->crc8(data.data() + 1, SEN44_UART_RESPONSE_LENGTH - 3);
  if (crc != data[SEN44_UART_RESPONSE_LENGTH - 2]) {
    ESP_LOGW(TAG, "Invalid CRC");
    return;
  }

  float pm_1_0 = this->get_16_bit_uint(3, data) / 10.0;
  float pm_2_5 = this->get_16_bit_uint(5, data) / 10.0;
  float pm_4_0 = this->get_16_bit_uint(7, data) / 10.0;
  float pm_10_0 = this->get_16_bit_uint(9, data) / 10.0;
  float voc = this->get_16_bit_uint(11, data) / 10.0;
  float humidity = this->get_16_bit_uint(13, data) / 100.0;
  float temperature = this->get_16_bit_uint(15, data) / 200.0;

  if (this->pm_1_0_sensor_ != nullptr)
    this->pm_1_0_sensor_->publish_state(pm_1_0);
  if (this->pm_2_5_sensor_ != nullptr)
    this->pm_2_5_sensor_->publish_state(pm_2_5);
  if (this->pm_4_0_sensor_ != nullptr)
    this->pm_4_0_sensor_->publish_state(pm_4_0);
  if (this->pm_10_0_sensor_ != nullptr)
    this->pm_10_0_sensor_->publish_state(pm_10_0);
  if (this->voc_sensor_ != nullptr)
    this->voc_sensor_->publish_state(voc);
  if (this->humidity_sensor_ != nullptr)
    this->humidity_sensor_->publish_state(humidity);
  if (this->temperature_sensor_ != nullptr)
    this->temperature_sensor_->publish_state(temperature);
}

void SEN44UARTComponent::request_data() {
  uint8_t request[SEN44_UART_REQUEST_LENGTH] = {0x7E, 0x00, 0x03, 0x01, 0xFB, 0x7E};
  this->write_array(request, SEN44_UART_REQUEST_LENGTH);
}

}  // namespace sen44_uart
}  // namespace esphome
