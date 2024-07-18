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
// Routing activation response code values
#define ROUTING_ACTIVATION_SUCCESSFUL 0x10
/* End */

/* Others */
#define DOIP_HEADER_LENGTH 8
#define IDENTIFICATION_RESPONSE_PAYLOAD_LENGTH 33
#define ROUTING_ACTIVATION_RESPONSE_PAYLOAD_LENGTH 9
/* End */

class FramePayload
{
public:
    virtual uint8_t *getData() = 0;
    virtual size_t getDataLength() = 0;
    virtual void debug_print() = 0;
};

class NormalPayload: public FramePayload
{
public:
    NormalPayload();
    NormalPayload(uint8_t *data, size_t length);
    uint8_t *getData();
    size_t getDataLength();
    void debug_print();
    ~NormalPayload();
private:
    uint8_t *data;
    size_t dataLength;
};

class UDSPayload : public FramePayload
{
public:
    UDSPayload();
    ~UDSPayload();
    UDSPayload(uint8_t *buffer, size_t length);
    uint16_t getSourceAddress();
    uint16_t getTargetAddress();
    uint8_t *getUDSPayload();
    uint8_t *getData();
    size_t getDataLength();
    void setSourceAddress(uint16_t SourceAddress);
    void setTargetAddress(uint16_t TargetAddress);
    void setUDSPayload(uint8_t *UDSPayload, size_t length);
    void debug_print();

private:
    uint16_t sourceAddress;
    uint16_t targetAddress;
    uint8_t *udsPayload;
    size_t udsPayloadLength;
};


class DoIPFrame
{
public:
    DoIPFrame(bool isNull);
    DoIPFrame(uint8_t *buffer, size_t length);
    ~DoIPFrame();
    void setPayloadType(uint16_t PayloadType);
    void setPayloadLength(size_t PayloadLength);
    bool isNull();
    size_t getPayloadLength();
    uint8_t *getData();
    size_t getDataLength();
    uint8_t* getPayload();
    uint16_t getPayloadType();
    UDSPayload *getUDSPayload();
    void setPayload(uint8_t *data, size_t length);
    void setUDSPayload(uint16_t payload_type, uint8_t *data, size_t len, uint16_t source, uint16_t target);
    void debug_print();

private:
    uint8_t *header;
    uint8_t *payload;
};


void doip_server_init();
void handle_doip_frame(DoIPFrame *frame, WiFiClient *tcp_client, WiFiUDP* udp_client, int client_type);
std::pair<uint8_t*, size_t> gen_identification_response_payload(char *VIN, uint8_t *LogicalAddress, uint8_t *EID, uint8_t *GID);
std::pair<uint8_t*, size_t> gen_routing_activation_response_payload(uint16_t ClientLogicalAddress, uint16_t DoIPLogicalAddress, uint8_t code);

namespace doip_uds {
    int isIncorrectMessageLengthOrInvalidFormat(uint8_t* frame, size_t len);

    void session_mode_change(uint8_t* frame);
    void tester_present(uint8_t* frame);
    void read_data_by_id(uint8_t* frame);
    void write_data_by_id(uint8_t* frame);
    void security_access(uint8_t* frame);
    void io_control_by_did(uint8_t* frame);
    void request_download_or_upload(uint8_t* frame, int sid);
    void transfer_data(uint8_t* frame);
    void xfer_exit(uint8_t* frame);

    void handle_pkt(uint8_t* frame, size_t len);
}

extern void reset_relevant_variables();

#endif