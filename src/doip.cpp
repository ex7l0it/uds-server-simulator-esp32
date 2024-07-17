#include "doip.hpp"
#include <Arduino.h>

NormalPayload::NormalPayload()
{
    data = NULL;
    dataLength = 0;
}

NormalPayload::NormalPayload(uint8_t *data, size_t length)
{
    this->data = (uint8_t *)malloc(length);
    if (this->data == NULL)
    {
        Serial.printf("Error: Memory allocation failed in NormalPayload construct\n");
        dataLength = 0;
        return;
    }
    memcpy(this->data, data, length);
    dataLength = length;
    Serial.printf("NormalPayload initialized\n");
}

NormalPayload::~NormalPayload()
{
    if (data != NULL)
    {
        free(data);
    }
}

uint8_t *NormalPayload::getData()
{
    return data;
}

size_t NormalPayload::getDataLength()
{
    return dataLength;
}

void NormalPayload::debug_print()
{
    Serial.printf("Normal Payload: ");
    if (data == NULL)
    {
        Serial.printf("NULL\n");
        return;
    }
    for (size_t i = 0; i < dataLength; i++)
    {
        Serial.printf("%02X ", data[i]);
    }
    Serial.printf("\n");
}

UDSPayload::UDSPayload()
{
    sourceAddress = 0;
    targetAddress = 0;
    udsPayload = NULL;
}

UDSPayload::~UDSPayload()
{
    if (udsPayload != NULL)
    {
        free(udsPayload);
    }
}

UDSPayload::UDSPayload(uint8_t *buffer, size_t length)
{
    if (length < 4)
    {
        sourceAddress = 0;
        targetAddress = 0;
        udsPayload = NULL;
        return;
    }
    sourceAddress = (buffer[0] << 8) | buffer[1];
    targetAddress = (buffer[2] << 8) | buffer[3];
    udsPayloadLength = length - 4;
    Serial.printf("UDSPayloadLength: %d\n", udsPayloadLength);
    udsPayload = (uint8_t *)malloc(udsPayloadLength);
    if (udsPayload == NULL)
    {
        Serial.printf("Error: Memory allocation failed in DoIPPayload construct\n");
        udsPayloadLength = 0;
        return;
    }
    memcpy(udsPayload, buffer + 4, udsPayloadLength);
    Serial.printf("DoIPPayload initialized\n");
}

uint16_t UDSPayload::getSourceAddress()
{
    return sourceAddress;
}

uint16_t UDSPayload::getTargetAddress()
{
    return targetAddress;
}

uint8_t *UDSPayload::getUDSPayload()
{
    return udsPayload;
}

void UDSPayload::setSourceAddress(uint16_t SourceAddress)
{
    this->sourceAddress = SourceAddress;
}

void UDSPayload::setTargetAddress(uint16_t TargetAddress)
{
    this->targetAddress = TargetAddress;
}

void UDSPayload::setUDSPayload(uint8_t *UDSPayload, size_t length)
{
    this->udsPayload = UDSPayload;
    this->udsPayloadLength = length;
}

uint8_t *UDSPayload::getData()
{
    uint8_t *data = (uint8_t *)malloc(4 + udsPayloadLength);
    data[0] = (sourceAddress >> 8) & 0xFF;
    data[1] = sourceAddress & 0xFF;
    data[2] = (targetAddress >> 8) & 0xFF;
    data[3] = targetAddress & 0xFF;
    memcpy(data + 4, udsPayload, udsPayloadLength);
    return data;
}

size_t UDSPayload::getDataLength()
{
    return 4 + udsPayloadLength;
}

void UDSPayload::debug_print()
{
    Serial.printf("Source Address: %d\n", sourceAddress);
    Serial.printf("Target Address: %d\n", targetAddress);
    Serial.printf("UDS Payload: ");
    if (udsPayload == NULL)
    {
        Serial.printf("NULL\n");
        return;
    }
    for (size_t i = 0; i < udsPayloadLength; i++)
    {
        Serial.printf("%02X ", udsPayload[i]);
    }
    Serial.printf("\n");
}

DoIPFrame::DoIPFrame()
{
    ProtocolVersion = 0x02;
    InverseProtocolVersion = 0xFD;
    PayloadType = 0;
    PayloadLength = 0;
    Payload = nullptr;
}

DoIPFrame::DoIPFrame(uint8_t *buffer, size_t length)
{
    if (length < 8)
    {
        return;
    }
    ProtocolVersion = buffer[0];
    InverseProtocolVersion = buffer[1];
    PayloadType = (buffer[2] << 8) | buffer[3];
    PayloadLength = (buffer[4] << 24) | (buffer[5] << 16) | (buffer[6] << 8) | buffer[7];
    if (length < 8 + PayloadLength)
    {
        return;
    }
    Payload = new UDSPayload(buffer + 8, PayloadLength);
    Serial.printf("DoIPFrame initialized\n");
}

DoIPFrame::~DoIPFrame()
{
}

void DoIPFrame::setPayloadType(uint16_t PayloadType)
{
    this->PayloadType = PayloadType;
}

uint16_t DoIPFrame::getPayloadType()
{
    return PayloadType;
}

size_t DoIPFrame::getDataLength()
{
    // add Header length
    return PayloadLength + 8;
}

uint8_t *DoIPFrame::getData()
{
    uint8_t *data = (uint8_t *)malloc(8 + PayloadLength);
    data[0] = ProtocolVersion;
    data[1] = InverseProtocolVersion;
    data[2] = (PayloadType >> 8) & 0xFF;
    data[3] = PayloadType & 0xFF;
    data[4] = (PayloadLength >> 24) & 0xFF;
    data[5] = (PayloadLength >> 16) & 0xFF;
    data[6] = (PayloadLength >> 8) & 0xFF;
    data[7] = PayloadLength & 0xFF;
    uint8_t *payload_data = Payload->getData();
    memcpy(data + 8, payload_data, PayloadLength);
    free(payload_data);
    return data;
}

std::pair<uint8_t *, uint32_t> parse_doip_frame(DoIPFrame frame)
{
    int type = frame.getPayloadType();
    if (type == VEHICLE_IDENTIFICATION_REQUEST)
    {
        Serial.printf("VEHICLE_IDENTIFICATION_REQUEST\n");
        // return response with DOIP_Version, VIN, Logical Address
        char VIN[18] = "12345678901234567";
        uint8_t LogicalAddress[2] = {0x0A, 0x00};
        uint8_t EID[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
        uint8_t GID[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x11};
        IdentificationResponsePayload identification_response(VIN, LogicalAddress, EID, GID);
        uint8_t *data = identification_response.getData();
        size_t dataLength = identification_response.getDataLength();
        if (data == NULL)
        {
            Serial.println("Error: IdentificationResponse data is NULL");
            return std::make_pair(nullptr, 0);
        }
        DoIPFrame response = DoIPFrame();
        response.setPayloadType(VEHICLE_ANNOUNCEMENT_OR_IDENTIFICATION_RESPONSE);
        response.setIdentificationResponsePayload(VIN, LogicalAddress, EID, GID);
        // response.debug_print();
        return std::make_pair(response.getData(), response.getDataLength());
    } else if (type == ROUTING_ACTIVATION_RESPONSE) {
        Serial.printf("ROUTING_ACTIVATION_RESPONSE\n");
        uint8_t *data = nullptr;
        
        
    }

    return std::make_pair(nullptr, 0);
}

void DoIPFrame::debug_print()
{
    Serial.printf("Protocol Version: %x\n", ProtocolVersion);
    Serial.printf("Inverse Protocol Version: %x\n", InverseProtocolVersion);
    Serial.printf("Payload Type: %x\n", PayloadType);
    Serial.printf("Payload Length: %x\n", PayloadLength);
    Payload->debug_print();
}

UDSPayload *DoIPFrame::getUDSPayload()
{
    return (UDSPayload *)Payload;
}

IdentificationResponsePayload *DoIPFrame::getIdentificationResponsePayload()
{
    if (PayloadType != VEHICLE_ANNOUNCEMENT_OR_IDENTIFICATION_RESPONSE)
    {
        return NULL;
    }
    return (IdentificationResponsePayload *)Payload;
}

IdentificationResponsePayload::IdentificationResponsePayload()
{
    memset(VIN, 0, 17);
    memset(LogicalAddress, 0, 2);
    memset(EID, 0, 6);
}

IdentificationResponsePayload::IdentificationResponsePayload(char *VIN, uint8_t *LogicalAddress, uint8_t *EID, uint8_t *GID)
{
    memcpy(this->VIN, VIN, 17);
    memcpy(this->LogicalAddress, LogicalAddress, 2);
    memcpy(this->EID, EID, 6);
    memcpy(this->GID, GID, 6);
}

IdentificationResponsePayload::~IdentificationResponsePayload()
{
}

uint8_t *IdentificationResponsePayload::getData()
{
    uint8_t *data = (uint8_t *)malloc(33);
    memcpy(data, VIN, 17);
    memcpy(data + 17, LogicalAddress, 2);
    memcpy(data + 19, EID, 6);
    memcpy(data + 25, GID, 6);
    data[31] = 0x00;
    data[32] = 0x00;
    return data;
}

size_t IdentificationResponsePayload::getDataLength()
{
    return 33;
}

void IdentificationResponsePayload::debug_print()
{
    Serial.printf("VIN: %s\n", VIN);
    Serial.printf("Logical Address: %02X %02X\n", LogicalAddress[0], LogicalAddress[1]);
    Serial.printf("EID: ");
    for (size_t i = 0; i < 6; i++)
    {
        Serial.printf("%02X ", EID[i]);
    }
    Serial.printf("\n");
    Serial.printf("GID: ");
    for (size_t i = 0; i < 6; i++)
    {
        Serial.printf("%02X ", GID[i]);
    }
    Serial.printf("\n");
}

void DoIPFrame::setUDSPayload(uint16_t payload_type, uint8_t *data, size_t len, uint16_t source, uint16_t target)
{
    this->PayloadType = payload_type;
    this->PayloadLength = len + 4;
    UDSPayload *payload = new UDSPayload();
    payload->setSourceAddress(source);
    payload->setTargetAddress(target);
    payload->setUDSPayload(data, len);
    this->Payload = payload;
}

void DoIPFrame::setIdentificationResponsePayload(char *VIN, uint8_t *LogicalAddress, uint8_t *EID, uint8_t *GID)
{
    if (this->PayloadType != VEHICLE_ANNOUNCEMENT_OR_IDENTIFICATION_RESPONSE)
    {
        return;
    }
    this->PayloadLength = 33;
    IdentificationResponsePayload *payload = new IdentificationResponsePayload(VIN, LogicalAddress, EID, GID);
    this->Payload = payload;
}
