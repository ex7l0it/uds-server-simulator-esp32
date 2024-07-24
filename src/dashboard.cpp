#include "dashboard.hpp"

uint16_t DashboardCANID[DASHBOARD_CAN_ID_COUNT] = {DEFAULT_SPEED_ID, DEFAULT_DOOR_ID, DEFAULT_TURN_SIGNAL_ID};
long current_speed = 0; 
uint8_t door_state = 0;
uint8_t turn_signal_state = 0;

long mph_to_kmh(long mph) {
    return (current_speed / 0.6213751) * 100;
}

void update_speed(CanFrame &frame) {
    current_speed = frame.data[1] * 256 + frame.data[2];
    // check if key exists
    for (int i = 0; i < DID_NUM; i++) {
        if (pairs[i].key == DID_OBD_VEHICLE_SPEED_SENSOR) {
            pairs[i].value[0] = (uint8_t) mph_to_kmh(current_speed);
            return;
        }
    }
}

void handle_dashboard_frame(CanFrame &frame)
{
    switch(frame.identifier) {
        case DEFAULT_SPEED_ID:
            Serial.println("Speed Frame");
            // check length
            if (frame.data_length_code > 0 && frame.data[0] >= DEFAULT_SPEED_POS) {
                update_speed(frame);
            }
            break;
        case DEFAULT_DOOR_ID:
            Serial.println("Door Frame");
            // check length
            if (frame.data_length_code > 0 && frame.data[0] >= DEFAULT_DOOR_POS) {
                uint8_t action = frame.data[1];
                uint8_t door = frame.data[2];
                if (action == 1) {
                    door_state |= door;
                } else if (action == 2) {
                    door_state &= ~door;
                }
            }
            break;
        case DEFAULT_TURN_SIGNAL_ID:
            Serial.println("Signal Frame");
            // check length
            if (frame.data_length_code > 0 && frame.data[0] >= DEFAULT_TURN_SIGNAL_POS) {
                uint8_t action = frame.data[1];
                if (action < 0) {
                    turn_signal_state ^= TURN_LEFT_SIGNAL;
                } else if (action > 0) {
                    turn_signal_state ^= TURN_RIGHT_SIGNAL;
                } else {
                    turn_signal_state = 0;
                }
            }
            break;
        default:
            Serial.println("Unknown Frame");
            break;
    }
}

String check_door_state() {
    String state = "";
    if (door_state & DOOR_FRONT_LEFT_LOCK) {
        state += "Front Left: Locked\t";
    } else {
        state += "Front Left: Unlocked\t";
    }
    if (door_state & DOOR_FRONT_RIGHT_LOCK) {
        state += "Front Right: Locked\t";
    } else {
        state += "Front Right: Unlocked\t";
    }
    if (door_state & DOOR_REAR_LEFT_LOCK) {
        state += "Rear Left: Locked\t";
    } else {
        state += "Rear Left: Unlocked\t";
    }
    if (door_state & DOOR_REAR_RIGHT_LOCK) {
        state += "Rear Right: Locked\n";
    } else {
        state += "Rear Right: Unlocked\n";
    }
    return state;
}

String check_turn_signal_state() {
    String state = "";
    if (turn_signal_state & TURN_LEFT_SIGNAL) {
        state += "Left Signal: On\t";
    } else {
        state += "Left Signal: Off\t";
    }
    if (turn_signal_state & TURN_RIGHT_SIGNAL) {
        state += "Right Signal: On\n";
    } else {
        state += "Right Signal: Off\n";
    }
    return state;
}

void show_dashboard()
{
    // print dashboard info to serial
    Serial.println("+++++++++ [Dashboard Start] +++++++++");
    Serial.println("Speed: " + String(mph_to_kmh(current_speed)) + " km/h");
    Serial.println("Door: ");
    Serial.println(check_door_state());
    Serial.println("Turn Signal: ");
    Serial.println(check_turn_signal_state());
    Serial.println("+++++++++ [ Dashboard End ] +++++++++");
}
