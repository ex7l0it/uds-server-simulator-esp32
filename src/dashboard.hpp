#include "../third/ESP32-TWAI-CAN.hpp"
#include "globals.hpp"

#define DASHBOARD_CAN_ID_COUNT  3

#define DEFAULT_SPEED_POS       1
#define DEFAULT_DOOR_POS        2
#define DEFAULT_TURN_SIGNAL_POS 1

#define TURN_LEFT_SIGNAL        1
#define TURN_RIGHT_SIGNAL       2

#define DID_OBD_VEHICLE_SPEED_SENSOR    0xF40D

extern uint16_t DashboardCANID[DASHBOARD_CAN_ID_COUNT];
extern long current_speed; //  km/h
extern uint8_t door_state;
extern uint8_t turn_signal_state;

void handle_dashboard_frame(CanFrame &frame);

void dashboard_init();