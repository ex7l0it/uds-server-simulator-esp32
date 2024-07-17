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

class IdentificationResponsePayload : public FramePayload
{
public:
    IdentificationResponsePayload();
    IdentificationResponsePayload(char *VIN, uint8_t *LogicalAddress, uint8_t *EID, uint8_t *GID);
    uint8_t *getData();
    size_t getDataLength();
    void debug_print();
    ~IdentificationResponsePayload();

private:
    uint8_t VIN[17];
    uint8_t LogicalAddress[2];
    uint8_t EID[6];
    uint8_t GID[6];
    uint8_t FurtherActionRequired;
    uint8_t SyncStatus;
};

class DoIPFrame
{
public:
    DoIPFrame();
    DoIPFrame(uint8_t *buffer, size_t length);
    ~DoIPFrame();
    uint16_t getPayloadType();
    void setPayloadType(uint16_t PayloadType);
    uint8_t *getData();
    size_t getDataLength();
    UDSPayload *getUDSPayload();
    IdentificationResponsePayload *getIdentificationResponsePayload();
    void setUDSPayload(uint16_t payload_type, uint8_t *data, size_t len, uint16_t source, uint16_t target);
    void setIdentificationResponsePayload(char *VIN, uint8_t *LogicalAddress, uint8_t *EID, uint8_t *GID);
    void debug_print();

private:
    uint8_t ProtocolVersion;
    uint8_t InverseProtocolVersion;
    uint16_t PayloadType;
    size_t PayloadLength;
    FramePayload *Payload;
};

std::pair<uint8_t *, uint32_t> parse_doip_frame(DoIPFrame frame);
