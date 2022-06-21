#include "mbed.h"
#include "L3_msg.h"

uint8_t L3_dnd_encodeDND(void)
{
    uint8_t dnd = 1;
    return dnd; 
}
/*
uint8_t L3_dnd_getDND(void)
{
    uint8_t 
    return dnd;
}*/


//uint8_t L3_msg_pdu(uint8_t* l3_pdu/*, uint8_t seq, */,uint8_t my_id, uint8_t dnd)
//{
//   /* l3_pdu[L3_PDU_OFFSET_SEQ] = seq; */
//    l3_pdu[0] = my_id;
//    l3_pdu[1] = dnd;

//    return l3_pdu;
//}



//uint8_t L3_msg_pdu(uint8_t* l3_pdu, uint8_t dnd /*,uint8_t my_id, uint8_t seq*/)
//{
//    l3_pdu[L3_PDU_OFFSET_DND] = dnd;
//    /* l3_pdu[L3_PDU_OFFSET_ID] = my_id; */
//     /* l3_pdu[L3_PDU_OFFSET_SEQ] = seq; */
//
//    return L3_PDU;
//}


/*
uint8_t L2_msg_encodeAck(uint8_t* msg_ack, uint8_t seq)
{
    msg_ack[L2_MSG_OFFSET_TYPE] = L2_MSG_TYPE_ACK;
    msg_ack[L2_MSG_OFFSET_SEQ] = seq;
    msg_ack[L2_MSG_OFFSET_DATA] = 1;

    return L2_MSG_ACKSIZE;
}

uint8_t L2_msg_getSeq(uint8_t* msg)
{
    return msg[L2_MSG_OFFSET_SEQ];
}
*/