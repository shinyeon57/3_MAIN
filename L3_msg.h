#include "mbed.h"

/* #define L3_PDU_OFFSET_SEQ  0 */
#define L3_PDU_OFFSET_ID   0//1
#define L3_PDU_OFFSET_DND  1//2

#define L3_PDU  2//3

uint8_t L3_msg_pdu(uint8_t* l3_pdu/*, uint8_t seq*/ ,uint8_t my_id, uint8_t dnd);
//uint8_t L3_msg_pdu(uint8_t* l3_pdu, uint8_t dnd /*,uint8_t my_id, uint8_t seq*/);
uint8_t L3_dnd_encodeDND(void);
//uint8_t L3_dnd_getDND(uint8_t dnd);

