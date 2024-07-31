#include "controls.hpp"

void send_speed()
{
    CanFrame frame = {0};
    frame.identifier = DEFAULT_SPEED_ID;
    frame.data_length_code = 8;
    frame.data[0] = 0x02;
    if (current_speed > 0) {
        uint16_t speed_value = current_speed * 100;
        frame.data[1] = ((speed_value) >> 8) & 0xFF; 
        frame.data[2] = speed_value & 0xFF;
    } else {
        frame.data[1] = 0x01;
        // random speed
        frame.data[2] = random(0, 255);
    }
    frame.data[3] = 0;
    frame.data[4] = 0;
    frame.data[5] = 0;
    frame.data[6] = 0;
    frame.data[7] = 0;
    ESP32Can.writeFrame(frame);
}

void lock_door(uint8_t door)
{
    CanFrame frame = {0};
    frame.identifier = DEFAULT_DOOR_ID;
    frame.data_length_code = 8;
    frame.data[0] = 2;
    frame.data[1] = 1;
    frame.data[2] = door;
    frame.data[3] = 0;
    frame.data[4] = 0;
    frame.data[5] = 0;
    frame.data[6] = 0;
    frame.data[7] = 0;
    ESP32Can.writeFrame(frame);
}

void unlock_door(uint8_t door)
{
    CanFrame frame = {0};
    frame.identifier = DEFAULT_DOOR_ID;
    frame.data_length_code = 8;
    frame.data[0] = 2;
    frame.data[1] = 2;
    frame.data[2] = door;
    frame.data[3] = 0;
    frame.data[4] = 0;
    frame.data[5] = 0;
    frame.data[6] = 0;
    frame.data[7] = 0;
    ESP32Can.writeFrame(frame);
}

void send_turn_signal(uint8_t action)
{
    CanFrame frame = {0};
    frame.identifier = DEFAULT_TURN_SIGNAL_ID;
    frame.data_length_code = 8;
    frame.data[0] = 2;
    frame.data[1] = action;
    frame.data[2] = 0;
    frame.data[3] = 0;
    frame.data[4] = 0;
    frame.data[5] = 0;
    frame.data[6] = 0;
    frame.data[7] = 0;
    ESP32Can.writeFrame(frame);
}

void check_gpio()
{
    // check speed
    if (gpio_get_level(SPEED_UP_GPIO) == HIGH) {
        if (current_speed < MAX_SPEED) {
            current_speed += 1;
        }
    } else {
        if (current_speed > 0) {
            current_speed -= 1;
        }
    }
}
