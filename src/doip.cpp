#include "doip.hpp"
#include "globals.hpp"
#include <Arduino.h>

/* DoIP Server Init Start */
void doip_server_init()
{
    // AP
    WiFiAP.softAPConfig(local_IP, gateway, subnet);
    WiFiAP.softAP(ssid, password);
    // Turn on LED
    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);

    // Listen
    udp.begin(DOIP_PORT);
    server.begin();
}
/* DoIP Server Init End */

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

DoIPFrame::DoIPFrame(bool isNull = false)
{
    this->header = nullptr;
    this->payload = nullptr;
    if (isNull)
    {
        return;
    }
    this->header = (uint8_t *)malloc(8);
    if (header == NULL)
    {
        Serial.printf("Error: Memory allocation failed in DoIPFrame construct\n");
        return;
    }
    this->header[0] = 0x02;
    this->header[1] = 0xFD;
    this->header[2] = 0x00;
    this->header[3] = 0x00;
    this->header[4] = 0x00;
    this->header[5] = 0x00;
    this->header[6] = 0x00;
    this->header[7] = 0x00;
    Serial.printf("DoIPFrame initialized\n");
}

DoIPFrame::DoIPFrame(uint8_t *buffer, size_t length)
{
    this->header = nullptr;
    this->payload = nullptr;
    if (length < DOIP_HEADER_LENGTH)
    {
        return;
    }
    // TODO: add max length check
    this->header = (uint8_t *)malloc(DOIP_HEADER_LENGTH);
    if (this->header == NULL)
    {
        Serial.printf("Error: Memory allocation failed in DoIPFrame construct (header)\n");
        return;
    }
    memcpy(this->header, buffer, DOIP_HEADER_LENGTH);
    // check length
    if (this->getPayloadLength() + DOIP_HEADER_LENGTH != length)
    {
        Serial.printf("Error: DoIPFrame length mismatch\n");
        free(this->header);
        this->header = nullptr;
        return;
    }
    if (this->getPayloadLength() > 0)
    {
        Serial.printf("Continue to copy payload.\n");
        this->payload = (uint8_t *)malloc(this->getPayloadLength());
        if (this->payload == NULL)
        {
            Serial.printf("Error: Memory allocation failed in DoIPFrame construct (payload)\n");
            free(this->header);
            this->header = nullptr;
            this->payload = nullptr;
            return;
        }
        memcpy(this->payload, buffer + DOIP_HEADER_LENGTH, this->getPayloadLength());
    }

    Serial.printf("DoIPFrame initialized\n");
}

DoIPFrame::~DoIPFrame()
{
    if (payload != nullptr)
    {
        free(payload);
        this->payload = nullptr;
    }
    if (header != nullptr)
    {
        free(header);
        this->header = nullptr;
    }
    Serial.printf("[-] DoIPFrame destroyed\n");
}

void DoIPFrame::setPayload(uint8_t *data, size_t length)
{
    if (this->header == NULL)
    {
        return;
    }
    if (this->payload != nullptr)
    {
        free(this->payload);
    }
    this->payload = (uint8_t *)malloc(length);
    memcpy(this->payload, data, length);
    this->setPayloadLength(length);
}

void DoIPFrame::setPayloadType(uint16_t PayloadType)
{
    if (this->header != NULL)
    {
        this->header[2] = (PayloadType >> 8) & 0xFF;
        this->header[3] = PayloadType & 0xFF;
    }
}

void DoIPFrame::setPayloadLength(size_t PayloadLength)
{
    if (this->header != NULL)
    {
        this->header[4] = (PayloadLength >> 24) & 0xFF;
        this->header[5] = (PayloadLength >> 16) & 0xFF;
        this->header[6] = (PayloadLength >> 8) & 0xFF;
        this->header[7] = PayloadLength & 0xFF;
    }
}

uint16_t DoIPFrame::getPayloadType()
{
    if (this->header != NULL)
    {
        return (this->header[2] << 8) | this->header[3];
    }
    return 0;
}

size_t DoIPFrame::getPayloadLength()
{
    if (this->header != NULL)
    {
        return (this->header[4] << 24) | (this->header[5] << 16) | (this->header[6] << 8) | this->header[7];
    }
    return 0;
}

size_t DoIPFrame::getDataLength()
{
    if (this->header == NULL)
    {
        return 0;
    }
    // add Header length
    return this->getPayloadLength() + 8;
}

uint8_t *DoIPFrame::getData()
{
    if (this->header == NULL)
    {
        return nullptr;
    }
    // concat header and payload
    uint8_t *data = (uint8_t *)malloc(this->getDataLength() + DOIP_HEADER_LENGTH);
    if (data == NULL)
    {
        Serial.printf("Error: Memory allocation failed in DoIPFrame getData\n");
        return nullptr;
    }
    memcpy(data, header, DOIP_HEADER_LENGTH);
    memcpy(data + DOIP_HEADER_LENGTH, payload, this->getPayloadLength());
    return data;
}

uint8_t *DoIPFrame::getPayload()
{
    return this->payload;
}

void DoIPFrame::debug_print()
{
    Serial.printf("DoIP Frame: ");
    Serial.printf("+++++ Header +++++\n");
    if (header == NULL)
    {
        Serial.printf("NULL\n");
        return;
    }
    for (size_t i = 0; i < DOIP_HEADER_LENGTH; i++)
    {
        Serial.printf("%02X ", header[i]);
    }
    Serial.printf("\n+++++ Payload +++++\n");
    if (payload != NULL)
    {
        for (size_t i = 0; i < this->getPayloadLength(); i++)
        {
            Serial.printf("%02X ", payload[i]);
        }
        Serial.printf("\n");
    }
    else
    {
        Serial.printf("Payload: NULL\n");
    }
}

void DoIPFrame::setUDSPayload(uint16_t payload_type, uint8_t *data, size_t len, uint16_t source, uint16_t target)
{
    // this->PayloadType = payload_type;
    // this->PayloadLength = len + 4;
    // UDSPayload *payload = new UDSPayload();
    // payload->setSourceAddress(source);
    // payload->setTargetAddress(target);
    // payload->setUDSPayload(data, len);
    // this->Payload = payload;
}

bool DoIPFrame::isNull()
{
    return header == nullptr;
}

std::pair<uint8_t *, size_t> gen_identification_response_payload(char *VIN, uint8_t *LogicalAddress, uint8_t *EID, uint8_t *GID)
{
    uint8_t *data = (uint8_t *)malloc(IDENTIFICATION_RESPONSE_PAYLOAD_LENGTH);
    if (data == NULL)
    {
        Serial.printf("Error: Memory allocation failed in gen_identification_response_payload\n");
        return std::make_pair(nullptr, 0);
    }
    memcpy(data, VIN, 17);
    memcpy(data + 17, LogicalAddress, 2);
    memcpy(data + 19, EID, 6);
    memcpy(data + 25, GID, 6);
    data[31] = 0x00;
    data[32] = 0x00;
    return std::make_pair(data, IDENTIFICATION_RESPONSE_PAYLOAD_LENGTH);
}

std::pair<uint8_t *, size_t> gen_routing_activation_response_payload(uint16_t ClientLogicalAddress, uint16_t DoIPLogicalAddress, uint8_t code)
{
    uint8_t *data = (uint8_t *)malloc(ROUTING_ACTIVATION_RESPONSE_PAYLOAD_LENGTH);
    data[0] = (ClientLogicalAddress >> 8) & 0xFF;
    data[1] = ClientLogicalAddress & 0xFF;
    data[2] = (DoIPLogicalAddress >> 8) & 0xFF;
    data[3] = DoIPLogicalAddress & 0xFF;
    data[4] = code;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    data[8] = 0x00;
    return std::make_pair(data, ROUTING_ACTIVATION_RESPONSE_PAYLOAD_LENGTH);
}

void doip_uds::session_mode_change(uint8_t *frame)
{
    uint8_t *data = (uint8_t *)malloc(6);
    data[0] = frame[0] + 0x40;
    data[1] = frame[1];
    data[2] = 0x00;
    data[3] = 0x32;
    data[4] = 0x01;
    data[5] = 0xF4;

    if (current_session_mode != frame[1])
    {
        current_session_mode = frame[1];
        reset_relevant_variables();
    }
    if (current_session_mode == 0x01)
    { // init defauit session time
        change_to_non_default_session_seconds = 0;
        change_to_non_default_session_microseconds = 0;
    }
    else
    { // init non-defauit session time
        struct timeval currentTime;
        gettimeofday(&currentTime, NULL);
        change_to_non_default_session_seconds = currentTime.tv_sec;
        change_to_non_default_session_microseconds = currentTime.tv_usec;
    }

    free(data);
}

int doip_uds::isIncorrectMessageLengthOrInvalidFormat(uint8_t *frame, size_t len)
{
    int sid = frame[0];
    switch (sid)
    {
    case UDS_SID_DIAGNOSTIC_CONTROL:
    case UDS_SID_TESTER_PRESENT:
        if (len == 0x02)
            return 0;
        break;
    case UDS_SID_SECURITY_ACCESS: // not support the field "securityAccessDataRecord" in this verison
        if (len == 0x02 || len == 0x06)
            return 0;
        break;
    case UDS_SID_READ_DATA_BY_ID: // only support to read a DID per request frame in this version
        if (len == 0x03)
            return 0;
        break;
    case UDS_SID_WRITE_DATA_BY_ID: // support write single frame
        if (len >= 0x04 && len <= 0x07)
            return 0;
        break;
    case UDS_SID_IO_CONTROL_BY_ID: // only support one parameter(currentStatus) in this version
        if (len >= 0x04 && len <= 0x05)
            return 0;
        break;
    case UDS_SID_REQUEST_XFER_EXIT:
        if (len == 0x01)
            return 0;
    default:
        if (len >= 0x02 && len <= 0x07)
            return 0;
        break;
    }
    return INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT;
}

// void doip_uds::handle_pkt(uint8_t *frame, size_t len)
// {
//     /* GET SID & SF */
//     int sid, sf;
//     sid = frame[0];
//     sf = frame[1];
//     // NRC 0x13
//     int nrc_13 = doip_uds::isIncorrectMessageLengthOrInvalidFormat(frame, len);
//     if (nrc_13 == -1) // do not give a response
//         return;
//     if (nrc_13 != 0)
//     {
//         send_negative_response(sid, nrc_13);
//         return;
//     }

//     if (first_char == '0' || first_char == '1')
//     {
//         switch (sid)
//         {
//         case UDS_SID_DIAGNOSTIC_CONTROL: // SID 0x10
//             if (isSFExisted(sid, sf) == -1)
//                 return;
//             can_uds::session_mode_change(frame);
//             return;
//         case UDS_SID_TESTER_PRESENT: // SID 0x3E
//             if (current_session_mode != 0x01)
//             {
//                 if (isSFExisted(sid, sf) == -1)
//                     return;
//                 can_uds::tester_present(frame);
//                 return;
//             }
//             else
//             { // default session mode
//                 send_negative_response(sid, SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION);
//                 return;
//             }
//         case UDS_SID_READ_DATA_BY_ID: // SID 0x22
//             if (isSFExisted(sid, sf) == -1)
//                 return;
//             can_uds::read_data_by_id(frame);
//             return;
//         case UDS_SID_WRITE_DATA_BY_ID: // SID 0x2E
//             if (current_session_mode != 0x01)
//             { // only support SID 0x2E in non-default session mode
//                 if (isNonDefaultModeTimeout(sid) == -1)
//                     return;
//                 if (isSFExisted(sid, sf) == -1)
//                     return;
//                 can_uds::write_data_by_id(frame);
//                 return;
//             }
//             else
//             { // default session mode
//                 send_negative_response(sid, SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION);
//                 return;
//             }
//         case UDS_SID_SECURITY_ACCESS: // SID 0x27
//             if (current_session_mode != 0x01)
//             { // only support SID 0x27 in non-default session mode
//                 // Serial.printf("## [security_access] current_session_mode = %#x\n", current_session_mode);
//                 if (isNonDefaultModeTimeout(sid) == -1)
//                     return;
//                 if (isSFExisted(sid, sf) == -1)
//                     return;
//                 if (security_access_lock_time != 0 && (time(NULL) - security_access_lock_time) < SECURITY_ACCESS_LOCK_DELAY)
//                 {
//                     send_negative_response(sid, REQUIRED_TIME_DELAY_NOT_EXPIRED);
//                     return;
//                 }
//                 can_uds::security_access(frame);
//                 return;
//             }
//             else
//             { // default session mode
//                 send_negative_response(sid, SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION);
//                 return;
//             }
//         case UDS_SID_IO_CONTROL_BY_ID: // SID 0X2F
//             if (current_session_mode != 0x01)
//             { // only support SID 0x2F in non-default session mode
//                 if (isNonDefaultModeTimeout(sid) == -1)
//                     return;
//                 if (isSFExisted(sid, sf) == -1)
//                     return;
//                 can_uds::io_control_by_did(frame);
//                 return;
//             }
//             else
//             { // default session mode
//                 send_negative_response(sid, SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION);
//                 return;
//             }
//         case UDS_SID_REQUEST_DOWNLOAD:
//         case UDS_SID_REQUEST_UPLOAD: // SID 0x34 or 0x35
//             if (current_session_mode != 0x02)
//             {
//                 // MUST in programming session mode
//                 send_negative_response(sid, SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION);
//                 return;
//             }
//             else if (current_security_level == 0x00)
//             {
//                 // MUST unlock the security access
//                 send_negative_response(sid, SECURITY_ACCESS_DENIED);
//                 return;
//             }
//             else if (req_transfer_type != 0)
//             {
//                 send_negative_response(sid, CONDITIONS_NOT_CORRECT);
//                 return;
//             }
//             else
//             {
//                 can_uds::request_download_or_upload(frame, sid);
//                 return;
//             }
//         case UDS_SID_TRANSFER_DATA: // SID 0x36
//             if (current_session_mode != 0x02)
//             {
//                 // MUST in programming session mode
//                 send_negative_response(sid, SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION);
//                 return;
//             }
//             else if (current_security_level == 0x00)
//             {
//                 // MUST unlock the security access
//                 send_negative_response(sid, SECURITY_ACCESS_DENIED);
//                 return;
//             }
//             else if (req_transfer_type != 0x34 && req_transfer_type != 0x35)
//             {
//                 send_negative_response(sid, REQUEST_SEQUENCE_ERROR);
//                 return;
//             }
//             else
//             {
//                 can_uds::transfer_data(frame);
//                 return;
//             }
//         case UDS_SID_REQUEST_XFER_EXIT:
//             if (current_session_mode != 0x02)
//             {
//                 // MUST in programming session mode
//                 send_negative_response(sid, SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION);
//                 return;
//             }
//             else if (req_transfer_type != 0x34 && req_transfer_type != 0x35 || (req_transfer_block_counter != req_transfer_block_num))
//             {
//                 send_negative_response(sid, REQUEST_SEQUENCE_ERROR);
//                 return;
//             }
//             else
//             {
//                 can_uds::xfer_exit(frame);
//                 return;
//             }
//         default:
//             send_negative_response(sid, SERVICE_NOT_SUPPORTED);
//             return;
//         }
//     }

//     if (first_char == '2')
//     {
//         if (ggBufLengthRemaining > 0)
//         {
//             if (isNonDefaultModeTimeout(tmp_store[0]) == -1)
//             {
//                 return;
//             }

//             if (ggBufCounter != frame.data[0])
//             {
//                 memset(ggBuffer, 0, sizeof(ggBuffer));
//                 return;
//             }

//             if (ggBufLengthRemaining > 7)
//             {
//                 memcpy(ggBuffer + (ggBufSize - ggBufLengthRemaining), &frame.data[1], 7);
//                 ggBufCounter++;
//                 if (ggBufCounter == 0x30)
//                     ggBufCounter = 0x20;
//                 ggBufLengthRemaining -= 7;
//             }
//             else
//             {
//                 memcpy(ggBuffer + (ggBufSize - ggBufLengthRemaining), &frame.data[1], ggBufLengthRemaining);
//                 ggBufLengthRemaining = 0;
//                 if (ggSID == UDS_SID_WRITE_DATA_BY_ID)
//                 {
//                     unsigned char bytes[] = {tmp_store[1], tmp_store[2]}; //	two bytes of DID
//                     unsigned int did = (bytes[0] << 8) | bytes[1];        //	DID hex int value
//                     for (int i = 0; i < DID_NUM; i++)
//                     {
//                         if (pairs[i].key == did)
//                         {
//                             memset(pairs[i].value, 0, sizeof(pairs[i].value)); // clear the original value
//                             memcpy(pairs[i].value, ggBuffer, strlen((char *)ggBuffer));
//                         }
//                     }
//                     CanFrame resp = {0};
//                     resp.identifier = diag_phy_resp_id;
//                     resp.data_length_code = 8;
//                     resp.data[0] = 0x03;
//                     resp.data[1] = tmp_store[0] + 0x40;
//                     resp.data[2] = tmp_store[1];
//                     resp.data[3] = tmp_store[2];
//                     resp.data[4] = 0x00;
//                     resp.data[5] = 0x00;
//                     resp.data[6] = 0x00;
//                     resp.data[7] = 0x00;
//                     ESP32Can.writeFrame(resp);
//                     memset(tmp_store, 0, sizeof(tmp_store));
//                     memset(ggBuffer, 0, sizeof(ggBuffer));
//                 }
//                 if (ggSID == UDS_SID_TRANSFER_DATA)
//                 {
//                     memcpy(ggBuffer + (ggBufSize - ggBufLengthRemaining), &frame.data[1], ggBufLengthRemaining);
//                     int max_block_len = req_transfer_data_len > 127 ? 127 : req_transfer_data_len;
//                     int start_add = req_transfer_data_add + req_transfer_block_counter * max_block_len;
//                     memcpy(&firmwareSpace[start_add], ggBuffer, max_block_len);

//                     CanFrame resp = {0};
//                     resp.identifier = diag_phy_resp_id;
//                     resp.data_length_code = 8;
//                     resp.data[0] = 0x02;
//                     resp.data[1] = tmp_store[0] + 0x40;
//                     resp.data[2] = ++req_transfer_block_counter;
//                     resp.data[3] = 0x00;
//                     resp.data[4] = 0x00;
//                     resp.data[5] = 0x00;
//                     resp.data[6] = 0x00;
//                     resp.data[7] = 0x00;
//                     ESP32Can.writeFrame(resp);
//                     memset(ggBuffer, 0, sizeof(ggBuffer));
//                 }
//             }
//         }
//         return;
//     }

//     if (first_char == '3')
//     {
//         if (flow_control_flag)
//         { // 0-false 1-true
//             // uint8_t FS = frame.data[0] & 0x0F;
//             // uint8_t BS = frame.data[1];
//             uint8_t STmin = frame.data[2];

//             // fs = FS;
//             // bs = BS;
//             if (STmin > 0 && STmin <= 0x7F)
//             { // 0x0-0x7F ms
//                 st_min = STmin * 1000;
//             }
//             else if (STmin >= 0xF1 && STmin <= 0xF9)
//             { // 100-900 us
//                 st_min = (STmin & 0x0F) * 100;
//             }
//             else
//             { // 1 ms
//                 st_min = 1000;
//             }

//             flow_control_push_to();
//             flow_control_flag = 0;
//         }
//     }
// }

void handle_doip_frame(DoIPFrame *frame, WiFiClient *tcp_client, WiFiUDP *udp_client, int client_type)
{
    DoIPFrame *responseFrame = new DoIPFrame();
    int type = frame->getPayloadType();
    char VIN[18] = "12345678901234567";
    uint8_t LogicalAddress[2] = {0x0A, 0x00};
    uint8_t EID[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    uint8_t GID[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x11};
    uint8_t *requestPayload = nullptr;
    uint16_t SourceAddress = 0;
    uint8_t *payload = nullptr;
    size_t payload_len = 0;

    switch (type)
    {
    case VEHICLE_IDENTIFICATION_REQUEST:
        Serial.printf("VEHICLE_IDENTIFICATION_REQUEST\n");

        std::tie(payload, payload_len) = gen_identification_response_payload(VIN, LogicalAddress, EID, GID);
        responseFrame->setPayloadType(VEHICLE_ANNOUNCEMENT_OR_IDENTIFICATION_RESPONSE);
        break;
    case ROUTING_ACTIVATION_REQUEST:
        Serial.printf("ROUTING_ACTIVATION_REQUEST\n");

        requestPayload = frame->getPayload();
        SourceAddress = (requestPayload[0] << 8) | requestPayload[1];

        Serial.printf("Source Address: 0x%x\n", SourceAddress);
        Serial.printf("Activation Type: 0x%x\n", requestPayload[2]);

        std::tie(payload, payload_len) = gen_routing_activation_response_payload(SourceAddress, 0x0A00, ROUTING_ACTIVATION_SUCCESSFUL);
        responseFrame->setPayloadType(ROUTING_ACTIVATION_RESPONSE);
        break;
    case DIAGNOSTIC_MESSAGE:
        Serial.printf("DIAGNOSTIC_MESSAGE\n");

        delete responseFrame;
        responseFrame = nullptr;
        break;
    default:
        Serial.printf("Unknown DoIP Frame Type\n");

        delete responseFrame;
        responseFrame = nullptr;
    }

    if (responseFrame != nullptr)
    {
        responseFrame->setPayload(payload, payload_len);
        free(payload);
        uint8_t *response_data = responseFrame->getData();

        Serial.printf("[*] Response Frame:\n");
        responseFrame->debug_print();
        if (responseFrame != nullptr && !responseFrame->isNull())
        {
            if (client_type == UDP_CLIENT)
            {
                Serial.println("UDP write");
                udp_client->beginPacket(udp_client->remoteIP(), udp_client->remotePort());
                udp_client->write(response_data, responseFrame->getDataLength());
                udp_client->endPacket();
            } else if (client_type == TCP_CLIENT) {
                Serial.println("TCP write");
                tcp_client->write(response_data, responseFrame->getDataLength());
            } 
            
        } else {
            Serial.println("Error: gen response data is null");
        }
        free(response_data);
        delete responseFrame;
        responseFrame = nullptr;
    }
    return;
}