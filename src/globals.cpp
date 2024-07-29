#include "globals.hpp"

/* AP Config */
WiFiAPClass WiFiAP;
const char *doip_ssid = "ESP32_DoIP";
const char *dash_ssid = "ESP32_Dashboard";
const char *password = "12345678";
IPAddress local_IP(192, 168, 77, 1);
IPAddress gateway(192, 168, 77, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress boardcast_IP(255, 255, 255, 255);
// UDP Listen 13400
WiFiUDP udp;
// TCP Listen 13400
WiFiServer server(DOIP_PORT);
WebServer dash_server(80);

CanFrame rxFrame;
/* Globals */
char *version = "v1.0.0";

int mode = RUN_MODE_CAN; // 0: Can  1: DoIP  2: Can + Dashboard

int io_control_id_flag = 0; // 0-false 1-true
long io_control_seconds = 0;
long io_control_microseconds = 0;

int diag_func_req_id = 0;  // default 0
int diag_phy_req_id = 0;   // default 0
int diag_phy_resp_id = 0;  // default 0
bool random_frame = false; // default false

long change_to_non_default_session_seconds = 0;      // seconds part
long change_to_non_default_session_microseconds = 0; // microseconds part
const long S3Server_timer = 5;                       // default 5s

int current_session_mode = 1;                // default session mode(1), only support 1 and 3 in this version.
int current_security_level = 0;              // default not unlocked(0), only support 3, 19 and 21 in this version.
int current_security_phase_3 = 0;            // default 0, there are two phases: 1 and 2.
int current_security_phase_19 = 0;           // default 0, there are two phases: 1 and 2.
int current_security_phase_21 = 0;           // default 0, there are two phases: 1 and 2.
uint8_t *current_seed_3 = NULL;              // store the current 27's seed of security level 0x03.
uint8_t *current_seed_19 = NULL;             // store the current 27's seed of security level 0x19.
uint8_t *current_seed_21 = NULL;             // store the current 27's seed of security level 0x21.
int security_access_error_attempt = 0;       // store service 27 error attempt number, only limit sl 19 and 21.
unsigned long security_access_lock_time = 0; // store the lock time of security access, only limit sl 19 and 21.

uint8_t key_27[4] = {0}; // store 27's 4-byte key
uint8_t tmp_store[8] = {0};

int flow_control_flag = 0; // 0-false 1-true
uint8_t st_min = 0;        // us

/* This space is for $34 and $35 */
uint8_t *firmwareSpace = NULL;
int SPACE_SIZE = 64 * 1024; // 64K
int req_transfer_data_len = 0;
int req_transfer_data_add = 0;
int req_transfer_type = 0; // 0x34 or 0x35
int req_transfer_block_num = 0;
int req_transfer_block_counter = 0;

/* This is for $22 flow control packets */
uint8_t gBuffer[256] = {0};
int gBufSize = 0;
int gBufLengthRemaining = 0;
int gBufCounter = 0;

/* This is for $2E flow control packets */
uint8_t ggBuffer[256] = {0};
int ggBufSize = 0;
int ggBufLengthRemaining = 0;
int ggBufCounter = 0;
int ggSID = 0;

/********************************** DID Supported Start **********************************/
/* DID for 22 & 2E services, write DID value without authentication */
int DID_No_Security[100] = {0};
int DID_No_Security_Num = 0;

/* DID for 22 & 2E services, write DID value with 27 03 */
int DID_Security_03[100] = {0};
int DID_Security_03_Num = 0;

/* DID for 22 & 2E services, write DID value with 27 19 */
int DID_Security_19[100] = {0};
int DID_Security_19_Num = 0;

/* DID for 22 & 2E services, write DID value with 27 21 */
int DID_Security_21[100] = {0};
int DID_Security_21_Num = 0;

/* DID number for 22 & 2E services */
int DID_NUM = 0;

/* DID key-value for 22 & 2E services */
DIDKeyValuePair pairs[256];

/* DID for 2F service */
int DID_IO_Control[100] = {0};
int DID_IO_Control_Num = 0;
/********************************** DID Supported End **********************************/

/* DoIP */
uint16_t TesterLogicalAddress = 0;
uint16_t ECULogicalAddress = 0x0A00;
/* End. */