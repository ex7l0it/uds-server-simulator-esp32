
#ifndef CANUDS_H
#define CANUDS_H
#include "globals.hpp"
#include "../third/cJSON.h"

void sendRandomFrame();
uint8_t *seed_generate(int sl);
uint8_t *security_algorithm(uint8_t *seed_ptr, int sl);
void send_negative_response(int sid, int nrc);
void flow_control_push_to();
void isotp_send_to(uint8_t *data, int size);
unsigned int get_did_from_frame(CanFrame frame);
void isJsonObjNull(cJSON *object, char *str);
void isValueJsonString(cJSON *object);
void isValueJsonBool(cJSON *object);
int set_diag_id(cJSON *items, char *key_name);
int DID_assignment(cJSON *items, char *key_name, int *DID_Arrary);
void can_init();
void uds_server_init(cJSON *root, char *ecu);

void reset_relevant_variables();
char int2nibble(int two_char, int position);
int isSFExisted(int sid, int sf);
int isSubFunctionSupported(int sid, int sf);
int isIncorrectMessageLengthOrInvalidFormat(CanFrame frame);
int isRequestOutOfRange(unsigned int did);
int isSecurityAccessDenied(unsigned int did);
int isNonDefaultModeTimeout(int sid);
int isRequestSecurityAccessMember(unsigned int did);
int isServiceNotSupportedInActiveSession();
namespace can_uds {
    void session_mode_change(CanFrame frame);
    void tester_present(CanFrame frame);
    void read_data_by_id(CanFrame frame);
    void write_data_by_id(CanFrame frame);
    void security_access(CanFrame frame);
    void io_control_by_did(CanFrame frame);
    void request_download_or_upload(CanFrame frame, int sid);
    void transfer_data(CanFrame frame);
    void xfer_exit(CanFrame frame);
    void handle_pkt(CanFrame frame);
}

#endif