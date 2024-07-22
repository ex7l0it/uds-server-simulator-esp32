#include "doip.hpp"
#include "globals.hpp"
#include "canuds.hpp"
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

DoIPFrame::DoIPFrame(WiFiClient *tcp_client, WiFiUDP *udp_client)
{
    // generate a response DoIP frame with default header
    if (tcp_client != nullptr)
    {
        this->tcp_client = tcp_client;
        this->udp_client = nullptr;
    }
    else
    {
        this->tcp_client = nullptr;
        this->udp_client = udp_client;
    }
    this->header = nullptr;
    this->payload = nullptr;
    this->header = (uint8_t *)malloc(8);
    if (header == NULL)
    {
        Serial.printf("Error: Memory allocation failed in DoIPFrame construct\n");
        return;
    }
    this->header[0] = 0x02;
    this->header[1] = 0xFD;
    this->header[2] = 0x00; // Default Type: GENERIC_DOIP_HEADER
    this->header[3] = 0x00;
    this->header[4] = 0x00;
    this->header[5] = 0x00;
    this->header[6] = 0x00;
    this->header[7] = 0x00;
}

DoIPFrame::DoIPFrame(WiFiClient *tcp_client, WiFiUDP *udp_client, uint8_t *buffer, size_t length)
{
    // generate a DoIP frame from request buffer
    if (tcp_client != nullptr)
    {
        this->tcp_client = tcp_client;
        this->udp_client = nullptr;
    }
    else
    {
        this->tcp_client = nullptr;
        this->udp_client = udp_client;
    }
    this->header = nullptr;
    this->payload = nullptr;
    if (length < DOIP_HEADER_LENGTH)
    {
        // ignore invalid frame
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
    // check version info (DoIP-041)
    if (this->header[0] != 0x02 || this->header[1] != 0xFD)
    {
        this->sendNACK(INCORRECT_PATTERN_FORMAT);
        return;
    }
    // check payload type (DoIP-042)
    if (!this->checkPayloadType()) {
        this->sendNACK(UNKNOWN_PAYLOAD_TYPE);
        return;
    }

    // TODO: DoIP-043 DoIP-044 check

    // check length (DoIP-045)
    if (this->getPayloadLength() + DOIP_HEADER_LENGTH != length)
    {
        this->sendNACK(INCORRECT_PATTERN_FORMAT);
        return;
    }
    
    if (this->getPayloadLength() > 0)
    {
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

void DoIPFrame::setUDSPayload(uint8_t *data, size_t len, uint16_t source, uint16_t target)
{
    if (this->header == NULL)
    {
        return;
    }
    if (this->payload != nullptr)
    {
        free(this->payload);
    }
    this->payload = (uint8_t *)malloc(len + 4);
    this->payload[0] = (source >> 8) & 0xFF;
    this->payload[1] = source & 0xFF;
    this->payload[2] = (target >> 8) & 0xFF;
    this->payload[3] = target & 0xFF;
    memcpy(this->payload + 4, data, len);
    this->setPayloadLength(len + 4);
}

/// @brief send NACK response (will free memory)
/// @param SourceAddress 
/// @param TargetAddress 
/// @param NRC 
void DoIPFrame::sendNACK(uint8_t NRC)
{
    uint8_t payload_data[] = { NRC } ;
    this->setPayloadType(GENERIC_DOIP_HEADER);
    this->setPayload(payload_data, 1);
    uint8_t *response_data = this->getData();
    if (this->tcp_client != nullptr)
    {
        this->tcp_client->write(response_data, this->getDataLength());
    }
    else if (this->udp_client != nullptr)
    {
        this->udp_client->beginPacket(this->udp_client->remoteIP(), this->udp_client->remotePort());
        this->udp_client->write(response_data, this->getDataLength());
        this->udp_client->endPacket();
    }
    free(response_data);
    free(this->payload);
    this->payload = nullptr;
    free(this->header);
    this->header = nullptr;
    if (NRC == INCORRECT_PATTERN_FORMAT || NRC == INVALID_PAYLOAD_LENGTH) {
        if (this->tcp_client != nullptr)
        {
            this->tcp_client->stop();
        }
    }
    return;
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

bool DoIPFrame::isNull()
{
    return header == nullptr;
}

bool DoIPFrame::checkPayloadType()
{
    int payloadType = this->getPayloadType();
    switch (payloadType)
    {
    case VEHICLE_IDENTIFICATION_REQUEST:
    case ROUTING_ACTIVATION_REQUEST:
    case DIAGNOSTIC_MESSAGE:
        return true;
        break;
    default:
        return false;
        break;
    }
    return false;
}

std::pair<uint8_t *, size_t> gen_identification_response_payload(char *VIN, uint16_t LogicalAddress, uint8_t *EID, uint8_t *GID)
{
    uint8_t *data = (uint8_t *)malloc(IDENTIFICATION_RESPONSE_PAYLOAD_LENGTH);
    if (data == NULL)
    {
        Serial.printf("Error: Memory allocation failed in gen_identification_response_payload\n");
        return std::make_pair(nullptr, 0);
    }
    memcpy(data, VIN, 17);
    data[17] = (LogicalAddress >> 8) & 0xFF;
    data[18] = LogicalAddress & 0xFF;
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

std::pair<uint8_t *, size_t> gen_diagnostic_positive_ack_payload(uint16_t SourceAddress, uint16_t TargetAddress)
{
    uint8_t *data = (uint8_t *)malloc(DIAGNOSTIC_POSITIVE_ACK_PAYLOAD_LENGTH);
    data[0] = (SourceAddress >> 8) & 0xFF;
    data[1] = SourceAddress & 0xFF;
    data[2] = (TargetAddress >> 8) & 0xFF;
    data[3] = TargetAddress & 0xFF;
    data[4] = 0x00;
    return std::make_pair(data, DIAGNOSTIC_POSITIVE_ACK_PAYLOAD_LENGTH);
}

std::pair<uint8_t *, size_t> gen_diagnostic_positive_nack_payload(uint16_t SourceAddress, uint16_t TargetAddress, uint8_t NRC)
{
    uint8_t *data = (uint8_t *)malloc(DIAGNOSTIC_NEGATIVE_ACK_PAYLOAD_LENGTH);
    data[0] = (SourceAddress >> 8) & 0xFF;
    data[1] = SourceAddress & 0xFF;
    data[2] = (TargetAddress >> 8) & 0xFF;
    data[3] = TargetAddress & 0xFF;
    data[4] = NRC;
    return std::make_pair(data, DIAGNOSTIC_NEGATIVE_ACK_PAYLOAD_LENGTH);
}

unsigned int doip_uds::get_did_from_frame(uint8_t *frame)
{
    return (frame[1] << 8) | frame[2];
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
        if (len >= 0x04)
            return 0;
        break;
    case UDS_SID_IO_CONTROL_BY_ID: // only support one parameter(currentStatus) in this version
        if (len >= 0x04 && len <= 0x05)
            return 0;
        break;
    case UDS_SID_TRANSFER_DATA:
        if (len <= req_transfer_data_len + 6)
            return 0;
        break;
    case UDS_SID_REQUEST_XFER_EXIT:
        if (len == 0x01)
            return 0;
    default:
        if (len >= 0x02)
            return 0;
        break;
    }
    return INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT;
}

int doip_uds::isSFExisted(WiFiClient &tcp_client, int sid, int sf)
{
    int nrc_12 = isSubFunctionSupported(sid, sf);
    if (nrc_12 != 0)
    {
        doip_uds::send_negative_response(tcp_client, sid, nrc_12);
        return -1;
    }
    return 0;
}

int doip_uds::isNonDefaultModeTimeout(WiFiClient &tcp_client, int sid)
{
    int nrc_7F = isServiceNotSupportedInActiveSession();
    if (nrc_7F != 0)
    {
        current_session_mode = 0x01;
        reset_relevant_variables();
        send_negative_response(tcp_client, sid, nrc_7F);
        return -1;
    }
    return 0;
}

void doip_uds::send_negative_response(WiFiClient &tcp_client, int sid, int nrc)
{
    uint8_t payload_data[] = {
        0x3F,
        sid,
        nrc};
    DoIPFrame response = DoIPFrame(&tcp_client, nullptr);
    response.setPayloadType(DIAGNOSTIC_MESSAGE);
    response.setUDSPayload(payload_data, sizeof(payload_data), ECULogicalAddress, TesterLogicalAddress);
    uint8_t *response_data = response.getData();
    tcp_client.write(response_data, response.getDataLength());
    free(response_data);
}

void doip_uds::session_mode_change(WiFiClient &tcp_client, uint8_t *frame)
{
    uint8_t payload_data[] = {
        frame[0] + 0x40,
        frame[1],
        0x00,
        0xc8,
        0x01,
        0xF4};
    DoIPFrame response = DoIPFrame(&tcp_client, nullptr);
    response.setPayloadType(DIAGNOSTIC_MESSAGE);
    response.setUDSPayload(payload_data, sizeof(payload_data), ECULogicalAddress, TesterLogicalAddress);
    uint8_t *response_data = response.getData();
    tcp_client.write(response_data, response.getDataLength());
    free(response_data);

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
}

void doip_uds::tester_present(WiFiClient &tcp_client, uint8_t *frame)
{
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    change_to_non_default_session_seconds = currentTime.tv_sec;
    change_to_non_default_session_microseconds = currentTime.tv_usec;

    int sf = frame[1];
    if (sf == 0x00)
    {
        uint8_t payload_data[] = {
            frame[0] + 0x40,
            frame[1]};

        DoIPFrame response = DoIPFrame(&tcp_client, nullptr);
        response.setPayloadType(DIAGNOSTIC_MESSAGE);
        response.setUDSPayload(payload_data, sizeof(payload_data), ECULogicalAddress, TesterLogicalAddress);
        uint8_t *response_data = response.getData();
        tcp_client.write(response_data, response.getDataLength());
        free(response_data);
    }
    if (sf == 0x80)
    {
        return;
    }
}

void doip_uds::read_data_by_id(WiFiClient &tcp_client, uint8_t *frame)
{
    unsigned int did = get_did_from_frame(frame);
    if (isRequestSecurityAccessMember(did) == 0)
    {
        int nrc_33 = isSecurityAccessDenied(did);
        if (nrc_33 != 0)
        {
            doip_uds::send_negative_response(tcp_client, UDS_SID_READ_DATA_BY_ID, nrc_33);
            return;
        }
    }
    int nrc_31 = isRequestOutOfRange(did);
    if (nrc_31 != 0)
    {
        doip_uds::send_negative_response(tcp_client, UDS_SID_READ_DATA_BY_ID, nrc_31);
        return;
    }

    uint8_t *data = nullptr;
    size_t data_len = 0;
    for (int i = 0; i < DID_NUM; i++)
    {
        if (pairs[i].key == did)
        {
            data_len = strlen((char *)pairs[i].value) + 3;
            data = (uint8_t *)malloc(data_len);
            if (data != nullptr)
            {
                data[0] = frame[0] + 0x40;
                data[1] = (uint8_t)((did & 0x0000ff00) >> 8);
                data[2] = (uint8_t)(did & 0x000000ff);
                memcpy(&data[3], pairs[i].value, strlen((char *)pairs[i].value));
            }
            else
            {
                Serial.printf("Memory allocation failed\n");
            }
            break;
        }
    }
    if (data != nullptr)
    {
        DoIPFrame response = DoIPFrame(&tcp_client, nullptr);
        response.setPayloadType(DIAGNOSTIC_MESSAGE);
        response.setUDSPayload(data, data_len, ECULogicalAddress, TesterLogicalAddress);
        free(data);
        data = nullptr;

        uint8_t *response_data = response.getData();
        size_t response_len = response.getDataLength();
        tcp_client.write(response_data, response_len);

        free(response_data);
    }
    else
    {
        Serial.printf("[!] DID not found\n");
    }
}

void doip_uds::write_data_by_id(WiFiClient &tcp_client, uint8_t *frame, size_t len)
{
    unsigned int did = get_did_from_frame(frame);
    int nrc_31 = isRequestOutOfRange(did);
    if (nrc_31 != 0)
    {
        doip_uds::send_negative_response(tcp_client, UDS_SID_WRITE_DATA_BY_ID, nrc_31);
        return;
    }
    int nrc_33 = isSecurityAccessDenied(did);
    if (nrc_33 != 0)
    {
        doip_uds::send_negative_response(tcp_client, UDS_SID_WRITE_DATA_BY_ID, nrc_33);
        return;
    }

    for (int i = 0; i < DID_NUM; i++)
    {
        if (pairs[i].key == did)
        {
            memset(pairs[i].value, 0, sizeof(pairs[i].value)); // clear the original value
            if (len > 256)
                len = 256;
            memcpy(pairs[i].value, &frame[3], len - 3);
        }
    }

    DoIPFrame response = DoIPFrame(&tcp_client, nullptr);
    response.setPayloadType(DIAGNOSTIC_MESSAGE);
    uint8_t payload_data[] = {
        frame[0] + 0x40,
        frame[1],
        frame[2]};
    response.setUDSPayload(payload_data, sizeof(payload_data), ECULogicalAddress, TesterLogicalAddress);
    uint8_t *response_data = response.getData();
    tcp_client.write(response_data, response.getDataLength());
    free(response_data);
}

void doip_uds::security_access(WiFiClient &tcp_client, uint8_t *frame)
{
    int sl = frame[1];
    uint8_t *seedp = nullptr;
    uint8_t *keyp = nullptr;

    if (sl == 0x03 || sl == 0x19 || sl == 0x21)
    {
        seedp = seed_generate(sl);
        DoIPFrame response = DoIPFrame(&tcp_client, nullptr);
        response.setPayloadType(DIAGNOSTIC_MESSAGE);
        uint8_t payload_data[] = {
            frame[0] + 0x40,
            frame[1],
            seedp[0],
            seedp[1],
            seedp[2],
            seedp[3],
        };
        response.setUDSPayload(payload_data, sizeof(payload_data), ECULogicalAddress, TesterLogicalAddress);
        uint8_t *response_data = response.getData();
        tcp_client.write(response_data, response.getDataLength());
        free(response_data);
        free(seedp);

        if (sl != current_security_level)
        {
            if (sl == 0x03)
            {
                current_seed_3 = seedp;
                current_security_phase_3 = 1;
            }
            if (sl == 0x19)
            {
                current_seed_19 = seedp;
                current_security_phase_19 = 1;
            }
            if (sl == 0x21)
            {
                current_seed_21 = seedp;
                current_security_phase_21 = 1;
            }
        }
        return;
    }
    if (sl == 0x04 || sl == 0x1A || sl == 0x22)
    {
        /* must request seed firstly */
        if ((sl == 0x04 && current_security_phase_3 != 1) || (sl == 0x1A && current_security_phase_19 != 1) || (sl == 0x22 && current_security_phase_21 != 1))
        {
            doip_uds::send_negative_response(tcp_client, UDS_SID_SECURITY_ACCESS, REQUEST_SEQUENCE_ERROR);
            return;
        }

        /* calculate key */
        if (sl == 0x04 && current_seed_3 != NULL && current_security_phase_3 == 1)
        {
            keyp = security_algorithm(current_seed_3, sl);
            // Serial.printf("%02x %02x %02x %02x\n", keyp[0], keyp[1], keyp[2], keyp[3]);
        }
        if (sl == 0x1A && current_seed_19 != NULL && current_security_phase_19 == 1)
        {
            keyp = security_algorithm(current_seed_19, sl);
        }
        if (sl == 0x22 && current_seed_21 != NULL && current_security_phase_21 == 1)
        {
            keyp = security_algorithm(current_seed_21, sl);
            // Serial.printf("key: %02x %02x %02x %02x\n", keyp[0], keyp[1], keyp[2], keyp[3]);
        }
        /* determine the passed key is right or not */
        if (*keyp == frame[2] && *(keyp + 1) == frame[3] && *(keyp + 2) == frame[4] && *(keyp + 3) == frame[5])
        { // key is correct
            uint8_t payload_data[] = {
                frame[0] + 0x40,
                frame[1]};
            DoIPFrame resp = DoIPFrame(&tcp_client, nullptr);
            resp.setPayloadType(DIAGNOSTIC_MESSAGE);
            resp.setUDSPayload(payload_data, sizeof(payload_data), ECULogicalAddress, TesterLogicalAddress);
            uint8_t *response_data = resp.getData();
            tcp_client.write(response_data, resp.getDataLength());
            free(response_data);

            if (sl == 0x04)
            {
                current_security_level = sl - 1;
                current_security_phase_3 = 2;
            }
            if (sl == 0x1A)
            {
                current_security_level = sl - 1;
                current_security_phase_19 = 2;
                security_access_error_attempt = 0;
            }
            if (sl == 0x22)
            {
                current_security_level = sl - 1;
                current_security_phase_21 = 2;
                security_access_error_attempt = 0;
            }
            security_access_lock_time = 0;
        }
        else
        { // key is incorrect
            doip_uds::send_negative_response(tcp_client, UDS_SID_SECURITY_ACCESS, INVALID_KEY);
            if (sl == 0x04)
            {
                current_security_phase_3 = 0;
            }
            if (sl == 0x1A)
            {
                current_security_phase_19 = 0;
                security_access_error_attempt += 1;
            }
            if (sl == 0x22)
            {
                current_security_phase_21 = 0;
                security_access_error_attempt += 1;
            }
        }
        /* determine the service 27 error attempt number is exceed or not */
        if (security_access_error_attempt >= SECURITY_ACCESS_ERROR_LIMIT_NUM)
        {
            doip_uds::send_negative_response(tcp_client, UDS_SID_SECURITY_ACCESS, EXCEED_NUMBER_OF_ATTEMPTS);
            // store currect time
            security_access_lock_time = time(NULL);
        }
        /* reset the global variables current_seed_* */
        if (sl == 0x04)
        {
            current_seed_3 = NULL;
        }
        if (sl == 0x1A)
        {
            current_seed_19 = NULL;
        }
        if (sl == 0x22)
        {
            current_seed_21 = NULL;
        }
        return;
    }
}

void doip_uds::io_control_by_did(WiFiClient &tcp_client, uint8_t *frame)
{
    // TODO
}

void doip_uds::request_download_or_upload(WiFiClient &tcp_client, uint8_t *frame, int sid, size_t len)
{
    u_int8_t dataFormatIdentifier = frame[1];
    u_int8_t addressAndLengthFormatIdentifier = frame[2];
    // high 4 bits is memoryAddressLength, low 4 bits is memorySizeLength
    u_int8_t memoryAddressLength = (addressAndLengthFormatIdentifier & 0xF0) >> 4;
    u_int8_t memorySizeLength = addressAndLengthFormatIdentifier & 0x0F;
    u_int32_t memoryAddress = 0;
    u_int32_t memorySize = 0;

    // nrc13 check
    if (len != 3 + memoryAddressLength + memorySizeLength)
    {
        doip_uds::send_negative_response(tcp_client, sid, INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT);
        return;
    }
    if (memoryAddressLength > 2 || memorySizeLength > 2)
    {
        doip_uds::send_negative_response(tcp_client, sid, REQUEST_OUT_OF_RANGE);
        return;
    }
    // get memoryAddress and memorySize
    for (int i = 0; i < memoryAddressLength; i++)
    {
        memoryAddress = memoryAddress << 8;
        memoryAddress += frame[3 + i];
    }
    for (int i = 0; i < memorySizeLength; i++)
    {
        memorySize = memorySize << 8;
        memorySize += frame[3 + memoryAddressLength + i];
    }

    if (memoryAddress + memorySize >= SPACE_SIZE)
    {
        doip_uds::send_negative_response(tcp_client, UDS_SID_REQUEST_UPLOAD, REQUEST_OUT_OF_RANGE);
        return;
    }
    req_transfer_data_add = memoryAddress;
    req_transfer_data_len = memorySize;

    req_transfer_type = sid;
    // Calculate the number of data blocks required (divide by 127 to round up the result)
    req_transfer_block_num = (req_transfer_data_len + 127 - 1) / 127;

    DoIPFrame response = DoIPFrame(&tcp_client, nullptr);
    response.setPayloadType(DIAGNOSTIC_MESSAGE);
    uint8_t payload_data[] = {
        frame[0] + 0x40,
        0x20,
        0x00,
        0x81,
    };
    response.setUDSPayload(payload_data, sizeof(payload_data), ECULogicalAddress, TesterLogicalAddress);
    uint8_t *response_data = response.getData();
    tcp_client.write(response_data, response.getDataLength());
    free(response_data);
}

void doip_uds::transfer_data(WiFiClient &tcp_client, uint8_t *frame)
{
    u_int8_t sequenceNumber = frame[1];

    if (sequenceNumber > req_transfer_block_num || sequenceNumber == 0)
    {
        doip_uds::send_negative_response(tcp_client, UDS_SID_TRANSFER_DATA, REQUEST_OUT_OF_RANGE);
        return;
    }
    if (sequenceNumber != req_transfer_block_counter + 1 && sequenceNumber != req_transfer_block_counter)
    {
        doip_uds::send_negative_response(tcp_client, UDS_SID_TRANSFER_DATA, REQUEST_SEQUENCE_ERROR);
        return;
    }
    if (sequenceNumber == req_transfer_block_counter)
    {
        req_transfer_block_counter--;
    }

    if (req_transfer_type == UDS_SID_REQUEST_DOWNLOAD)
    {
        int max_block_len = req_transfer_data_len > 127 ? 127 : req_transfer_data_len;

        memset(&firmwareSpace[req_transfer_data_add + (sequenceNumber - 1) * max_block_len], 0, max_block_len);
        memcpy(&firmwareSpace[req_transfer_data_add + (sequenceNumber - 1) * max_block_len], &frame[2], max_block_len);

        DoIPFrame response = DoIPFrame(&tcp_client, nullptr);
        response.setPayloadType(DIAGNOSTIC_MESSAGE);
        uint8_t payload_data[] = {
            frame[0] + 0x40,
            ++req_transfer_block_counter,
        };
        response.setUDSPayload(payload_data, sizeof(payload_data), ECULogicalAddress, TesterLogicalAddress);
        uint8_t *response_data = response.getData();
        tcp_client.write(response_data, response.getDataLength());
        free(response_data);
        return;
    }
    if (req_transfer_type == UDS_SID_REQUEST_UPLOAD)
    {
        int max_block_len = req_transfer_data_len > 127 ? 127 : req_transfer_data_len;
        int block_len = (req_transfer_data_len - (sequenceNumber - 1) * 127) > 127 ? max_block_len : (req_transfer_data_len - (sequenceNumber - 1) * 127);

        DoIPFrame response = DoIPFrame(&tcp_client, nullptr);
        response.setPayloadType(DIAGNOSTIC_MESSAGE);
        uint8_t payload_data[2 + block_len] = {0};
        payload_data[0] = frame[0] + 0x40;
        payload_data[1] = sequenceNumber;
        memcpy(payload_data + 2, &firmwareSpace[req_transfer_data_add + (sequenceNumber - 1) * 127], block_len);
        response.setUDSPayload(payload_data, sizeof(payload_data), ECULogicalAddress, TesterLogicalAddress);
        uint8_t *response_data = response.getData();
        tcp_client.write(response_data, response.getDataLength());
        free(response_data);
        req_transfer_block_counter++;
        return;
    }
}

void doip_uds::xfer_exit(WiFiClient &tcp_client, uint8_t *frame)
{
    DoIPFrame response = DoIPFrame(&tcp_client, nullptr);
    response.setPayloadType(DIAGNOSTIC_MESSAGE);
    uint8_t payload_data[] = {
        frame[0] + 0x40};
    response.setUDSPayload(payload_data, sizeof(payload_data), ECULogicalAddress, TesterLogicalAddress);
    uint8_t *response_data = response.getData();
    tcp_client.write(response_data, response.getDataLength());
    free(response_data);

    // reset
    req_transfer_data_add = 0;
    req_transfer_data_len = 0;
    req_transfer_block_num = 0;
    req_transfer_block_counter = 0;
    req_transfer_type = 0x00;
}

void doip_uds::handle_pkt(WiFiClient &tcp_client, uint8_t *frame, size_t len)
{
    /* GET SID & SF */
    int sid, sf;
    sid = frame[0];
    sf = frame[1];
    // NRC 0x13
    int nrc_13 = doip_uds::isIncorrectMessageLengthOrInvalidFormat(frame, len);
    if (nrc_13 == -1) // do not give a response
        return;
    if (nrc_13 != 0)
    {
        doip_uds::send_negative_response(tcp_client, sid, nrc_13);
        return;
    }

    switch (sid)
    {
    case UDS_SID_DIAGNOSTIC_CONTROL: // SID 0x10
        if (doip_uds::isSFExisted(tcp_client, sid, sf) == -1)
            return;
        doip_uds::session_mode_change(tcp_client, frame);
        return;
    case UDS_SID_TESTER_PRESENT: // SID 0x3E
        if (current_session_mode != 0x01)
        {
            if (doip_uds::isSFExisted(tcp_client, sid, sf) == -1)
                return;
            doip_uds::tester_present(tcp_client, frame);
            return;
        }
        else
        { // default session mode
            doip_uds::send_negative_response(tcp_client, sid, SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION);
            return;
        }
    case UDS_SID_READ_DATA_BY_ID: // SID 0x22
        if (doip_uds::isSFExisted(tcp_client, sid, sf) == -1)
            return;
        doip_uds::read_data_by_id(tcp_client, frame);
        return;
    case UDS_SID_WRITE_DATA_BY_ID: // SID 0x2E
        if (current_session_mode != 0x01)
        { // only support SID 0x2E in non-default session mode
            if (doip_uds::isNonDefaultModeTimeout(tcp_client, sid) == -1)
                return;
            if (doip_uds::isSFExisted(tcp_client, sid, sf) == -1)
                return;
            doip_uds::write_data_by_id(tcp_client, frame, len);
            return;
        }
        else
        { // default session mode
            doip_uds::send_negative_response(tcp_client, sid, SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION);
            return;
        }
    case UDS_SID_SECURITY_ACCESS: // SID 0x27
        if (current_session_mode != 0x01)
        { // only support SID 0x27 in non-default session mode
            // Serial.printf("## [security_access] current_session_mode = %#x\n", current_session_mode);
            if (doip_uds::isNonDefaultModeTimeout(tcp_client, sid) == -1)
                return;
            if (doip_uds::isSFExisted(tcp_client, sid, sf) == -1)
                return;
            if (security_access_lock_time != 0 && (time(NULL) - security_access_lock_time) < SECURITY_ACCESS_LOCK_DELAY)
            {
                doip_uds::send_negative_response(tcp_client, sid, REQUIRED_TIME_DELAY_NOT_EXPIRED);
                return;
            }
            doip_uds::security_access(tcp_client, frame);
            return;
        }
        else
        { // default session mode
            doip_uds::send_negative_response(tcp_client, sid, SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION);
            return;
        }
    case UDS_SID_IO_CONTROL_BY_ID: // SID 0X2F
        if (current_session_mode != 0x01)
        { // only support SID 0x2F in non-default session mode
            if (doip_uds::isNonDefaultModeTimeout(tcp_client, sid) == -1)
                return;
            if (doip_uds::isSFExisted(tcp_client, sid, sf) == -1)
                return;
            doip_uds::io_control_by_did(tcp_client, frame);
            return;
        }
        else
        { // default session mode
            doip_uds::send_negative_response(tcp_client, sid, SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION);
            return;
        }
    case UDS_SID_REQUEST_DOWNLOAD:
    case UDS_SID_REQUEST_UPLOAD: // SID 0x34 or 0x35
        if (current_session_mode != 0x02)
        {
            // MUST in programming session mode
            doip_uds::send_negative_response(tcp_client, sid, SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION);
            return;
        }
        else if (current_security_level == 0x00)
        {
            // MUST unlock the security access
            doip_uds::send_negative_response(tcp_client, sid, SECURITY_ACCESS_DENIED);
            return;
        }
        else if (req_transfer_type != 0)
        {
            doip_uds::send_negative_response(tcp_client, sid, CONDITIONS_NOT_CORRECT);
            return;
        }
        else
        {
            doip_uds::request_download_or_upload(tcp_client, frame, sid, len);
            return;
        }
    case UDS_SID_TRANSFER_DATA: // SID 0x36
        if (current_session_mode != 0x02)
        {
            // MUST in programming session mode
            doip_uds::send_negative_response(tcp_client, sid, SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION);
            return;
        }
        else if (current_security_level == 0x00)
        {
            // MUST unlock the security access
            doip_uds::send_negative_response(tcp_client, sid, SECURITY_ACCESS_DENIED);
            return;
        }
        else if (req_transfer_type != 0x34 && req_transfer_type != 0x35)
        {
            doip_uds::send_negative_response(tcp_client, sid, REQUEST_SEQUENCE_ERROR);
            return;
        }
        else
        {
            doip_uds::transfer_data(tcp_client, frame);
            return;
        }
    case UDS_SID_REQUEST_XFER_EXIT:
        if (current_session_mode != 0x02)
        {
            // MUST in programming session mode
            doip_uds::send_negative_response(tcp_client, sid, SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION);
            return;
        }
        else if (req_transfer_type != 0x34 && req_transfer_type != 0x35 || (req_transfer_block_counter != req_transfer_block_num))
        {
            doip_uds::send_negative_response(tcp_client, sid, REQUEST_SEQUENCE_ERROR);
            return;
        }
        else
        {
            doip_uds::xfer_exit(tcp_client, frame);
            return;
        }
    default:
        doip_uds::send_negative_response(tcp_client, sid, SERVICE_NOT_SUPPORTED);
        return;
    }
}

void handle_doip_frame(DoIPFrame *frame, WiFiClient *tcp_client, WiFiUDP *udp_client, int client_type)
{
    DoIPFrame responseFrame = DoIPFrame(tcp_client, udp_client);
    int type = frame->getPayloadType();
    char VIN[18] = "DEFAULT_VIN123456";
    // check if VIN is in the Global pairs
    for (int i = 0; i < DID_NUM; i++)
    {
        if (pairs[i].key == 0xF190)
        {
            memcpy(VIN, pairs[i].value, strlen((char *)pairs[i].value));
            break;
        }
    }
    // get MAC address
    String WiFiMAC = WiFi.macAddress();
    uint8_t EID[6] = {0};
    for (int i = 0; i < 6; i++)
    {
        EID[i] = (uint8_t)strtol(WiFiMAC.substring(i * 3, i * 3 + 2).c_str(), NULL, 16);
    }
    uint8_t GID[6] = {0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    uint8_t *requestPayload = nullptr;
    uint16_t SourceAddress = 0;
    uint16_t TargetAddress = 0;
    uint8_t *payload = nullptr;
    size_t payload_len = 0;
    uint8_t *response_data = nullptr;

    switch (type)
    {
    case VEHICLE_IDENTIFICATION_REQUEST:
        // Serial.printf("VEHICLE_IDENTIFICATION_REQUEST\n");
        std::tie(payload, payload_len) = gen_identification_response_payload(VIN, ECULogicalAddress, EID, GID);
        responseFrame.setPayloadType(VEHICLE_ANNOUNCEMENT_OR_IDENTIFICATION_RESPONSE);
        break;
    case ROUTING_ACTIVATION_REQUEST:
        // Serial.printf("ROUTING_ACTIVATION_REQUEST\n");
        requestPayload = frame->getPayload();
        SourceAddress = (requestPayload[0] << 8) | requestPayload[1];

        if (requestPayload[2] != 0x00) {
            // DoIP-151
            std::tie(payload, payload_len) = gen_routing_activation_response_payload(SourceAddress, ECULogicalAddress, ROUTING_UNSUPPORTED_TYPE);
        } else {
            TesterLogicalAddress = SourceAddress;
            std::tie(payload, payload_len) = gen_routing_activation_response_payload(TesterLogicalAddress, ECULogicalAddress, ROUTING_ACTIVATION_SUCCESSFUL);
        }

        responseFrame.setPayloadType(ROUTING_ACTIVATION_RESPONSE);
        break;
    case DIAGNOSTIC_MESSAGE:
        // Serial.printf("DIAGNOSTIC_MESSAGE\n");
        requestPayload = frame->getPayload();
        SourceAddress = (requestPayload[0] << 8) | requestPayload[1];
        TargetAddress = (requestPayload[2] << 8) | requestPayload[3];

        if (SourceAddress != TesterLogicalAddress)
        {
            // DoIP-070
            std::tie(payload, payload_len) = gen_diagnostic_positive_nack_payload(TargetAddress, SourceAddress, INVALID_SOURCE_ADDRESS);
            responseFrame.setPayloadType(DIAGNOSTIC_MESSAGE_NEGATIVE_ACK);
        } else if (TargetAddress != ECULogicalAddress) {
            // DoIP-071
            std::tie(payload, payload_len) = gen_diagnostic_positive_nack_payload(TargetAddress, SourceAddress, UNKNOWN_TARGET_ADDRESS);
            responseFrame.setPayloadType(DIAGNOSTIC_MESSAGE_NEGATIVE_ACK);
        } else {
            std::tie(payload, payload_len) = gen_diagnostic_positive_ack_payload(TargetAddress,ECULogicalAddress);
            responseFrame.setPayloadType(DIAGNOSTIC_MESSAGE_POSITIVE_ACK);
        }
        
        responseFrame.setPayload(payload, payload_len);
        free(payload);
        response_data = responseFrame.getData();
        if (!responseFrame.isNull())
        {
            tcp_client->write(response_data, responseFrame.getDataLength());
        }
        free(response_data);

        if (SourceAddress != TesterLogicalAddress)
        {
            // close tcp connection
            tcp_client->stop();
            return;
        } else if (TargetAddress != ECULogicalAddress)
        {
            return;
        } else {
            doip_uds::handle_pkt(*tcp_client, requestPayload + 4, frame->getPayloadLength() - 4);
            return;
        }
    default:
        // Serial.printf("Unknown DoIP Frame Type\n");
        return;
    }

    responseFrame.setPayload(payload, payload_len);
    free(payload);
    response_data = responseFrame.getData();

    // Serial.printf("[*] Response Frame:\n");
    // responseFrame.debug_print();
    if (!responseFrame.isNull())
    {
        if (client_type == UDP_CLIENT)
        {
            Serial.println("UDP write");
            udp_client->beginPacket(udp_client->remoteIP(), udp_client->remotePort());
            udp_client->write(response_data, responseFrame.getDataLength());
            udp_client->endPacket();
        }
        else if (client_type == TCP_CLIENT)
        {
            Serial.println("TCP write");
            tcp_client->write(response_data, responseFrame.getDataLength());
        }
    }
    else
    {
        Serial.println("Error: gen response data is null");
    }
    free(response_data);

    return;
}