#ifndef GLOBALS_H
#define GLOBALS_H
#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WebServer.h>
#include "../uds-server-simulator.h"
#include "../third/ESP32-TWAI-CAN.hpp"
#include "doip.hpp"


// Default for ESP32
#define CAN_TX 5
#define CAN_RX 4

#define RUN_MODE_CAN                  0x0
#define RUN_MODE_DOIP                 0x1
#define RUN_MODE_CAN_DASHBOARD        0x2
#define RUN_MODE_CAN_CONTROL          0x3

#define DEFAULT_SPEED_ID            0x255
#define DEFAULT_DOOR_ID             0x166
#define DEFAULT_TURN_SIGNAL_ID      0x177

#define DOOR_FRONT_LEFT_LOCK    1
#define DOOR_FRONT_RIGHT_LOCK   2
#define DOOR_REAR_LEFT_LOCK     4
#define DOOR_REAR_RIGHT_LOCK    8

#define DOIP_PORT 13400
#define UDP_CLIENT 0
#define TCP_CLIENT 1

extern WiFiAPClass WiFiAP;
extern const char *doip_ssid;
extern const char *dash_ssid;
extern const char *password;
extern IPAddress local_IP;
extern IPAddress gateway;
extern IPAddress subnet;
extern IPAddress boardcast_IP;
extern WiFiUDP udp;
extern WiFiServer server;
extern WebServer dash_server;

extern CanFrame rxFrame;
extern char *version;
extern int mode;
extern int io_control_id_flag;
extern long io_control_seconds;
extern long io_control_microseconds;
extern int diag_func_req_id;
extern int diag_phy_req_id;
extern int diag_phy_resp_id;
extern bool random_frame;
extern long change_to_non_default_session_seconds;
extern long change_to_non_default_session_microseconds;
extern const long S3Server_timer;
extern int current_session_mode;
extern int current_security_level;
extern int current_security_phase_3;
extern int current_security_phase_19;
extern int current_security_phase_21;
extern uint8_t *current_seed_3;
extern uint8_t *current_seed_19;
extern uint8_t *current_seed_21;
extern int security_access_error_attempt;
extern unsigned long security_access_lock_time;
extern uint8_t key_27[4];
extern uint8_t tmp_store[8];
extern int flow_control_flag;
extern uint8_t st_min;
extern uint8_t *firmwareSpace;
extern int SPACE_SIZE;
extern int req_transfer_data_len;
extern int req_transfer_data_add;
extern int req_transfer_type;
extern int req_transfer_block_num;
extern int req_transfer_block_counter;

extern uint8_t gBuffer[256];
extern int gBufSize;
extern int gBufLengthRemaining;
extern int gBufCounter;
extern uint8_t ggBuffer[256];
extern int ggBufSize;
extern int ggBufLengthRemaining;
extern int ggBufCounter;
extern int ggSID;

extern int DID_No_Security[100];
extern int DID_No_Security_Num;
extern int DID_Security_03[100];
extern int DID_Security_03_Num;
extern int DID_Security_19[100];
extern int DID_Security_19_Num;
extern int DID_Security_21[100];
extern int DID_Security_21_Num;
extern int DID_NUM;

struct DIDKeyValuePair {
  int key;
  uint8_t value[256];
};
extern DIDKeyValuePair pairs[256];
extern int DID_IO_Control[100];
extern int DID_IO_Control_Num;

extern uint16_t TesterLogicalAddress;
extern uint16_t ECULogicalAddress;

#endif // GLOBALS_H