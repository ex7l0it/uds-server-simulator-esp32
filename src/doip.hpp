#ifndef DOIP_H
#define DOIP_H
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>

/* DoIP Payload Type */
// Node management messages
#define GENERIC_DOIP_HEADER 0x0000
#define VEHICLE_IDENTIFICATION_REQUEST 0x0001
#define VEHICLE_IDENTIFICATION_REQUEST_EID 0x0002
#define VEHICLE_IDENTIFICATION_REQUEST_VIN 0x0003
#define VEHICLE_ANNOUNCEMENT_OR_IDENTIFICATION_RESPONSE 0x0004
#define ROUTING_ACTIVATION_REQUEST 0x0005
#define ROUTING_ACTIVATION_RESPONSE 0x0006
#define ALIVE_CHECK_REQUEST 0x0007
#define ALIVE_CHECK_RESPONSE 0x0008
// Stateful information messages
#define DoIP_ENTITY_STATE_REQUEST 0x4001
#define DoIP_ENTITY_STATE_RESPONSE 0x4002
#define DIAGNOSTIC_POWER_MODE_INFO_REQUEST 0x4003
#define DIAGNOSTIC_POWER_MODE_INFO_RESPONSE 0x4004
// Diagnostic messages
#define DIAGNOSTIC_MESSAGE 0x8001
#define DIAGNOSTIC_MESSAGE_POSITIVE_ACK 0x8002
#define DIAGNOSTIC_MESSAGE_NEGATIVE_ACK 0x8003
/* End */

/* Generic DoIP header NACK codes  */
#define INCORRECT_PATTERN_FORMAT        0x00  // Close socket
#define UNKNOWN_PAYLOAD_TYPE            0x01  // Discard DoIP message
#define MESSAGE_TOO_LARGE               0x02  // Discard DoIP message
#define OUT_OF_MEMORY                   0x03  // Discard DoIP message
#define INVALID_PAYLOAD_LENGTH          0x04  // Close socket
/* End */

/* Diagnostic message negative acknowledge codes */
#define INVALID_SOURCE_ADDRESS          0x02
#define UNKNOWN_TARGET_ADDRESS          0x03
#define DIAGNOSTIC_MESSAGE_TOO_LARGE    0x04
#define DMSG_OUT_OF_MEMORY              0x05
/* End */

// Routing activation response code values
#define ROUTING_ACTIVATION_SUCCESSFUL 0x10

/* Others */
#define DOIP_HEADER_LENGTH 8
#define IDENTIFICATION_RESPONSE_PAYLOAD_LENGTH 33
#define ROUTING_ACTIVATION_RESPONSE_PAYLOAD_LENGTH 9
#define DIAGNOSTIC_POSITIVE_ACK_PAYLOAD_LENGTH 5
#define DIAGNOSTIC_NEGATIVE_ACK_PAYLOAD_LENGTH 5
/* End */

class DoIPFrame
{
public:
    DoIPFrame(WiFiClient *tcp_client, WiFiUDP *udp_client);
    DoIPFrame(WiFiClient *tcp_client, WiFiUDP *udp_client, uint8_t *buffer, size_t length);
    ~DoIPFrame();
    void setPayloadType(uint16_t PayloadType);
    void setPayloadLength(size_t PayloadLength);
    bool isNull();
    bool checkPayloadType();
    size_t getPayloadLength();
    uint8_t *getData();
    size_t getDataLength();
    uint8_t* getPayload();
    uint16_t getPayloadType();
    void setPayload(uint8_t *data, size_t length);
    void setUDSPayload(uint8_t *data, size_t len, uint16_t source, uint16_t target);
    void sendNACK(uint8_t NRC);
    void debug_print();

private:
    WiFiClient *tcp_client;
    WiFiUDP *udp_client;
    uint8_t *header;
    uint8_t *payload;
};


void doip_server_init();
void handle_doip_frame(DoIPFrame *frame, WiFiClient *tcp_client, WiFiUDP* udp_client, int client_type);
std::pair<uint8_t*, size_t> gen_identification_response_payload(char *VIN, uint16_t LogicalAddress, uint8_t *EID, uint8_t *GID);
std::pair<uint8_t*, size_t> gen_routing_activation_response_payload(uint16_t ClientLogicalAddress, uint16_t DoIPLogicalAddress, uint8_t code);
std::pair<uint8_t*, size_t> gen_diagnostic_positive_ack_payload(uint16_t SourceAddress, uint16_t TargetAddress);
std::pair<uint8_t*, size_t> gen_diagnostic_positive_nack_payload(uint16_t SourceAddress, uint16_t TargetAddress, uint8_t NRC);

namespace doip_uds {
    unsigned int get_did_from_frame(uint8_t* frame);

    int isIncorrectMessageLengthOrInvalidFormat(uint8_t* frame, size_t len);
    int isSFExisted(WiFiClient &tcp_client, int sid, int sf);
    int isNonDefaultModeTimeout(WiFiClient &tcp_client, int sid);

    void send_negative_response(WiFiClient &tcp_client, int sid, int nrc);

    void session_mode_change(WiFiClient &tcp_client, uint8_t* frame);
    void tester_present(WiFiClient &tcp_client, uint8_t* frame);
    void read_data_by_id(WiFiClient &tcp_client, uint8_t* frame);
    void write_data_by_id(WiFiClient &tcp_client, uint8_t* frame, size_t len);
    void security_access(WiFiClient &tcp_client, uint8_t* frame);
    void io_control_by_did(WiFiClient &tcp_client, uint8_t* frame);
    void request_download_or_upload(WiFiClient &tcp_client, uint8_t* frame, int sid, size_t len);
    void transfer_data(WiFiClient &tcp_client, uint8_t* frame);
    void xfer_exit(WiFiClient &tcp_client, uint8_t* frame);

    void handle_pkt(WiFiClient &tcp_client, uint8_t* frame, size_t len);
}

extern void reset_relevant_variables();

#endif