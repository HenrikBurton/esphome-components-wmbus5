#include "transceiver.h"

#include "esphome/core/log.h"

#include "freertos/FreeRTOS.h"

namespace esphome {
namespace wmbus_radio {
static const char *TAG = "wmbus.transceiver";

bool RadioTransceiver::read_in_task(uint8_t *buffer, size_t length, uint32_t offset) {
  while (!this->get_frame(buffer, length, offset)) {
    if (!ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1)))
      return false;
  }

  return true;
}

void RadioTransceiver::set_reset_pin(InternalGPIOPin *reset_pin) {
  this->reset_pin_ = reset_pin;
}

void RadioTransceiver::set_irq_pin(InternalGPIOPin *irq_pin) {
  this->irq_pin_ = irq_pin;
}

void RadioTransceiver::reset() {
  this->reset_pin_->digital_write(0);
  delay(5);
  this->reset_pin_->digital_write(1);
  delay(5);
}

void RadioTransceiver::common_setup() {
  this->reset_pin_->setup();
  this->irq_pin_->setup();
  this->spi_setup();
}

uint8_t RadioTransceiver::spi_transaction(uint8_t command, std::initializer_list<uint8_t> data) {
  this->delegate_->begin_transaction();
  auto rval = this->delegate_->transfer(command);
  for (auto byte : data)
    rval = this->delegate_->transfer(byte);
  this->delegate_->end_transaction();
  return rval;
}

void RadioTransceiver::spi_read_frame(uint8_t command, std::initializer_list<uint8_t> data, uint8_t *buffer, size_t length) {
  this->delegate_->begin_transaction();
  auto rval = this->delegate_->transfer(command);
  for (auto byte : data)
    rval = this->delegate_->transfer(byte);
  for (size_t i = 0; i < length; i++)
    *buffer++ = this->delegate_->transfer(0x55);
  this->delegate_->end_transaction();
  return;
}

uint8_t RadioTransceiver::spi_read(uint8_t command) {
  return this->spi_transaction(command, {0});
}

void RadioTransceiver::spi_write(uint8_t command,
                                 std::initializer_list<uint8_t> data) {
  this->spi_transaction(command, data);
}

void RadioTransceiver::spi_write(uint8_t command, uint8_t data) {
  this->spi_write(command, {data});
}

void RadioTransceiver::dump_config() {
  ESP_LOGCONFIG(TAG, "Transceiver: %s", this->get_name());
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
  LOG_PIN("  IRQ Pin: ", this->irq_pin_);
}
} // namespace wmbus_radio
} // namespace esphome
