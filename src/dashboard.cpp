#include "dashboard.hpp"
#include <WebServer.h>

uint16_t DashboardCANID[DASHBOARD_CAN_ID_COUNT] = {DEFAULT_SPEED_ID, DEFAULT_DOOR_ID, DEFAULT_TURN_SIGNAL_ID};
long current_speed = 0; 
uint8_t door_state = 0;
uint8_t turn_signal_state = 0;

long kmh_to_mph(long kmh) {
    return kmh * 1.609344;
}

void update_speed(CanFrame &frame) {
    current_speed = (frame.data[1] * 256 + frame.data[2]) / 100.0;
    // check if key exists
    for (int i = 0; i < DID_NUM; i++) {
        if (pairs[i].key == DID_OBD_VEHICLE_SPEED_SENSOR) {
            pairs[i].value[0] = current_speed;
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
    String state = "<ul>";
    state += (door_state & DOOR_FRONT_LEFT_LOCK) ? "<li>Front Left: Locked</li>" : "<li>Front Left: Unlocked</li>";
    state += (door_state & DOOR_FRONT_RIGHT_LOCK) ? "<li>Front Right: Locked</li>" : "<li>Front Right: Unlocked</li>";
    state += (door_state & DOOR_REAR_LEFT_LOCK) ? "<li>Rear Left: Locked</li>" : "<li>Rear Left: Unlocked</li>";
    state += (door_state & DOOR_REAR_RIGHT_LOCK) ? "<li>Rear Right: Locked</li>" : "<li>Rear Right: Unlocked</li>";
    state += "</ul>";
    return state;
}

String check_turn_signal_state() {
    String state = "<ul>";
    state += (turn_signal_state & TURN_LEFT_SIGNAL) ? "<li>Left Signal: On</li>" : "<li>Left Signal: Off</li>";
    state += (turn_signal_state & TURN_RIGHT_SIGNAL) ? "<li>Right Signal: On</li>" : "<li>Right Signal: Off</li>";
    state += "</ul>";
    return state;
}

void dashboard_init()
{
    // AP
    WiFiAP.softAPConfig(local_IP, gateway, subnet);
    WiFiAP.softAP(dash_ssid, password);

    dash_server.on("/",  []() {
        // show dashboard info
        char buffer[1024];
        snprintf(buffer, 1024, 
            "<html>"
            "<head>"
            "<title>DashBoard</title>"
            "<style>"
            "body { font-family: Arial, sans-serif; margin: 20px; }"
            "h3 { color: #333; }"
            "p { font-size: 16px; }"
            "ul { list-style-type: none; padding: 0; }"
            "li { margin: 5px 0; }"
            "</style>"
            "</head>"
            "<body>"
            "<h3>DashBoard</h3>"
            "<p><strong>Speed:</strong> %ld km/h</p>"
            "<p><strong>Door:</strong> %s</p>"
            "<p><strong>Turn Signal:</strong> %s</p>"
            "</body>"
            "</html>",
            current_speed, 
            check_door_state().c_str(), 
            check_turn_signal_state().c_str()
        );
        dash_server.send(200, "text/html", buffer);
    });
    dash_server.onNotFound([]() {
        dash_server.send(404, "text/plain", "Not Found");
    });
    dash_server.begin();
}
