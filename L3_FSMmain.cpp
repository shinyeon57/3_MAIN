#include "L3_FSMevent.h"
#include "L3_msg.h"
#include "L3_timer.h"
#include "L3_LLinterface.h"
#include "protocol_parameters.h"
#include "mbed.h"


//FSM state -------------------------------------------------
#define L3STATE_IDLE                0
#define L3STATE_DND                 1               //우리가 추가한 state
#define L3STATE_CNN                 2               //우리가 추가한 state


//state variables
static uint8_t main_state = L3STATE_IDLE; //protocol state
static uint8_t prev_state = main_state;

//SDU (input)
static uint8_t originalWord[200];
static uint8_t wordLen=0;
static uint8_t dstid;
static uint8_t sdu[200];

//***********************************************************************************************//
static uint8_t originalMode[200];           //mode설정에서 누른 키가 inputword함수로 실행되는 기현상.....
static uint8_t modeLen=0;

static uint8_t check_pdu[2];
static uint8_t l3_pdu[2];


static uint8_t dst_dnd ;
static uint8_t my_dnd ;                 //dnd mode 확인용 변수 (0 : dnd(x) || 1 : dnd(o))
//***********************************************************************************************//

//serial port interface
static Serial pc(USBTX, USBRX);


//application event handler : generating SDU from keyboard input
static void L3service_processInputWord(void)
{
    char c = pc.getc();
    if (main_state == L3STATE_IDLE)
    {
        if (c == '\n' || c == '\r')
        {
            //originalWord[wordLen++] = '\0';
            if(originalWord[0] == '1')                                      //dnd mode 
            {
                //check_dnd = 1;                                                  //dnd mode check용 
                //pc.printf("\n! DND MODE ON !\n");
                L3_event_setEventFlag(L3_event_MODEctrlRcvd_DND);
                //L3_dnd_timer_startTimer();                             //////timer_dnd on 
            }
            else if(originalWord[0] == '2')                                 //connection mode
            {
                if (wordLen < 3)
                {
                    pc.printf("[ERROR] WRONG INPUT! no dest ID (%i), TRY AGAIN\n", c);
                    pc.printf("\n:: ENTER THE MODE ::\n1 : DND MODE, 2 : CONNECTION MODE\n");
                }
                else
                {
                    dstid = originalWord[2] - '0';
                    debug_if(DBGMSG_L3, "[L3] CNN MODE ON! dest id %i\n", dstid);
                    L3_event_setEventFlag(L3_event_MODEctrlRcvd_CNN);
                }
            }
            else                                  
            {
                pc.printf("[ERROR] WRONG INPUT! (%i) TRY AGAIN\n", originalWord[0]);
                pc.printf("\n:: ENTER THE MODE ::\n1 : DND MODE, 2 : CONNECTION MODE\n");
            }
            #if 1 //only for SH's macbook
            pc.getc();
            #endif
            wordLen = 0;
        }
        else
        {
            originalWord[wordLen++] = c;
        }
    }
    else if (main_state == L3STATE_CNN)
    {
        if (!L3_event_checkEventFlag(L3_event_dataToSend))                      
        {
            if (c == '\n' || c == '\r')
            {
                originalWord[wordLen++] = '\0'; //여기서 마지막이나 어딘가를 dnd status로 채워 그다음에 if문을 써서 
                L3_event_setEventFlag(L3_event_dataToSend);
                debug_if(DBGMSG_L3,"word is ready! ::: %s\n", originalWord);
            }
            else
            {
                originalWord[wordLen++] = c;
                if (wordLen >= L3_MAXDATASIZE-1)
                {
                    originalWord[wordLen++] = '\0';
                    L3_event_setEventFlag(L3_event_dataToSend);
                    pc.printf("\n max reached! word forced to be ready :::: %s\n", originalWord);
                }
                else
                {
                    if (strncmp((const char*)originalWord, "#DND ", 4) == 0)             ////#DND입력                                        
                    {    
                        //check_dnd = 1;                                                  //dnd mode check용   
                        L3_event_setEventFlag(L3_event_MODEctrlRcvd_DND);
                        debug_if(DBGMSG_L3, "\n DND MODE ON!\n");
                        //L3_dnd_timer_startTimer();                                      ////dnd용 2시간 timer시작한다 
                    }
                    else if (strncmp((const char*)originalWord, "#EXIT ", 5) == 0)       ////#EXIT입력
                    {
                        L3_event_setEventFlag(L3_event_MODEctrlRcvd_EXIT);               
                        debug_if(DBGMSG_L3, "\nEXITING THIS CHATTING\n");
                    }
                }
            }
        }
    }
    //L3_event_clearEventFlag(L3_event_MODEctrlRcvd);   
}

//mode 진입을 위한 제어 key SDU 생성=====================================
#if 0
void L3service_processInputMode(void)
{ 
    L3_event_setEventFlag(L3_event_MODEctrlRcvd);              

    pc.printf("\n:: ENTER THE MODE ::\n1 : DND MODE, 2 : CONNECTION MODE\n");      
    char input_mode = pc.getc();  

    if (L3_event_checkEventFlag(L3_event_MODEctrlRcvd))
    {    
        originalMode[modeLen++] = input_mode;

        if(input_mode == '1')                                      //dnd mode 
        {
            check_dnd = 1;                                                  //dnd mode check용 
            pc.printf("\n! DND MODE ON !\n");
            L3_event_setEventFlag(L3_event_MODEctrlRcvd_DND);
            L3_dnd_timer_startTimer();                             //////timer_dnd on 
        }
        else if(input_mode == '2')                                 //connection mode
        {
            pc.printf("\n! CNN MODE ON !\n");
            L3_event_setEventFlag(L3_event_MODEctrlRcvd_CNN);
        }
        else                                  
        {
            pc.printf("[ERROR] WRONG INPUT! TRY AGAIN\n");
            L3service_processInputMode();
        }
    }
    
   L3_event_clearEventFlag(L3_event_MODEctrlRcvd);                  
}
//destination ID 설정을 위한 함수=====================================
//static uint8_t timerStatus = 0;
uint8_t L3service_settingInputId(void)
{     
    uint8_t dstid_return = 0;

    pc.printf("\n:: Give ID for the destination \n ex) DstID : x\n ");    

    if (strncmp((const char*)originalWord, "DstID: ",5) == 0)
    {
        uint8_t dstid = originalWord[9] - '0';
        debug("[L3] requesting to set to dest id %i\n", dstid);
        //L3_LLI_configReqFunc(L2L3_CFGTYPE_DSTID, dstid);  
        //dstid_return = dstid;
        //pc.printf("\n:: ID for the destination : %i\n", dstid_return);     
    }  

    return dstid_return;
}
#endif

void L3_initFSM()
{
    //L3service_processInputMode();
    //initialize service layer
    pc.attach(&L3service_processInputWord, Serial::RxIrq);

    pc.printf("\n:: ENTER THE MODE ::\n1 : DND MODE, 2 : CONNECTION MODE\n");
}

void L3_FSMrun(uint8_t input_thisID)
{   
    
    if (prev_state != main_state)
    {
        debug_if(DBGMSG_L3, "\n[L3] State transition from %i to %i\n", prev_state, main_state);
        prev_state = main_state;
    }

    //FSM should be implemented here! ---->>>>
    switch (main_state)
    {
        case L3STATE_IDLE: //IDLE state description                

            if (L3_event_checkEventFlag(L3_event_msgRcvd)) //if data reception event happens
            {
                //여기서 상대방 메세지가 처음으로 올때 보내는 애의 아이디 정보를 받아서 연결을 해버리는 거임 어떻게 하나면
                //L3_LLI_configReqFunc(L2L3_CFGTYPE_DSTID, dstid); //dstid 에다가 받아온 아이디 정보를 넣어버리면
                //debug_if(DBGMSG_L3, "\n[L3]CHANGING ID!!! [%i]\n", dstid);
                main_state = L3STATE_CNN;
                /*
                //Retrieving data info.
                uint8_t* dataPtr = L3_LLI_getMsgPtr();
                uint8_t size = L3_LLI_getSize();

                debug("\n -------------------------------------------------\nRCVD MSG : %s (length:%i)\n -------------------------------------------------\n", 
                            dataPtr, size);
                
                pc.printf("Give a word to send : ");
                
                L3_event_clearEventFlag(L3_event_msgRcvd);
                */
            }
            else if (L3_event_checkEventFlag(L3_event_dataToSend)) //if data needs to be sent (keyboard input)
            {
#ifdef ENABLE_CHANGEIDCMD
                if (strncmp((const char*)originalWord, "changeDstID: ",9) == 0)
                {
                    uint8_t dstid = originalWord[9] - '0';
                    debug("[L3] requesting to change to dest id %i\n", dstid);
                    L3_LLI_configReqFunc(L2L3_CFGTYPE_DSTID, dstid);
                }
                else if (strncmp((const char*)originalWord, "changeSrcID: ",9) == 0)
                {
                    uint8_t myid = originalWord[9] - '0';
                    debug("[L3] requesting to change to srce id %i\n", myid);
                    L3_LLI_configReqFunc(L2L3_CFGTYPE_SRCID, myid);
                }
                else
#endif
                {
                    //msg header setting
                    strcpy((char*)sdu, (char*)originalWord);
                    L3_LLI_dataReqFunc(sdu, wordLen);
                    debug_if(DBGMSG_L3, "[L3] sending msg....\n");
                }
                
                wordLen = 0;

                pc.printf("Give a word to send : ");

                L3_event_clearEventFlag(L3_event_dataToSend);
            }
            else if(L3_event_checkEventFlag(L3_event_MODEctrlRcvd_CNN))
            {
                L3_event_clearEventFlag(L3_event_MODEctrlRcvd_CNN);
                //L3_LLI_configReqFunc(L2L3_CFGTYPE_DSTID, dstid);  
                L3_event_setEventFlag(L3_event_MODEctrl_CNN);
                main_state = L3STATE_CNN;
            }
            else if(L3_event_checkEventFlag(L3_event_MODEctrlRcvd_DND))
            {
                L3_event_clearEventFlag(L3_event_MODEctrlRcvd_DND);
                L3_event_setEventFlag(L3_event_MODEctrl_DND);
                L3_dnd_timer_startTimer(); 
                my_dnd = L3_dnd_encodeDND();
                debug_if(DBGMSG_L3, "[L3] MY DND STATE = (%i)\n", my_dnd);
                pc.printf("is on DND MODE!\n");
                main_state = L3STATE_DND;
            }
            else if(L3_event_checkEventFlag(L3_event_MODEctrlRcvd_EXIT))
            {
                pc.printf("\nEXITING THIS CHATTING\n");
                L3_event_clearEventFlag(L3_event_MODEctrlRcvd_EXIT);
                //L3service_processInputMode();                                     
            }
            break;

        case L3STATE_DND :                                               ///////////////////state = dnd
            if(L3_event_checkEventFlag(L3_event_MODEctrl_DND))       
            {
                if(L3_dnd_timer_getTimerStatus() == 0)                    ///////////////////dnd_timeout event happens
                {
                    //check_dnd = 0;                                                  //dnd mode check용 
                    L3_event_clearEventFlag(L3_event_MODEctrl_DND);
                    pc.printf("DND MODE END!\n");
                    main_state = L3STATE_IDLE ;
                    pc.printf("\n:: ENTER THE MODE ::\n1 : DND MODE, 2 : CONNECTION MODE\n");
                    
                }
                else if(L3_dnd_timer_getTimerStatus() == 1)
                {
                    check_pdu[0] = input_thisID ;
                    check_pdu[1] = my_dnd;
                    //check_pdu = L3_msg_pdu(l3_pdu, input_thisID, my_dnd);   /////나의 방해금지 모드를 알리기 위해서
                    debug_if(DBGMSG_L3, "[L3] IN DND id(%i), dnd_status(%i)\n", check_pdu[0], check_pdu[1]);
                    
                   //check_pdu = L3_msg_pdu(uint8_t* l3_pdu, uint8_t dnd /*,uint8_t my_id, uint8_t seq*/)

                    //if (L3_event_checkEventFlag(L3_event_msgRcvd)) //if data reception event happens
                    //{
                    //    //나의 dnd 상태를 상대방한테 보내야됨 // 매번 보내는 메세지 마다 dnd 상태를 같이 보내야됨 
                    //}
                }
            }
            break;

        case L3STATE_CNN :                                              ///////////////////state = cnn

            if (L3_event_checkEventFlag(L3_event_msgRcvd)) //if data reception event happens
            {
                //Retrieving data info.
                uint8_t* dataPtr = L3_LLI_getMsgPtr();
                uint8_t size = L3_LLI_getSize();

                debug("\n -------------------------------------------------\nRCVD MSG : %s (length:%i)\n -------------------------------------------------\n", 
                            dataPtr, size);
                
                pc.printf("Give a word to send : ");
                
                L3_event_clearEventFlag(L3_event_msgRcvd);
            }
            else if (L3_event_checkEventFlag(L3_event_dataToSend)) //if data needs to be sent (keyboard input)
            {
                //여기서 상대방의 dnd 상태를 받아서 IDLE 상태로 돌아가게 만들면 된다. 그리고 dstID 초기화
                if(check_pdu[0] == dstid)
                {
                
                debug_if(DBGMSG_L3, "\n[L3]COMPARISION IS WORKING []\n");

                  if(check_pdu[1] == '1')
                    {
                        dstid = 0;
                        main_state = L3STATE_IDLE;
                    }   
                }
                else
                {
                    debug_if(DBGMSG_L3, "\n[L3]COMPARISION ISN'T WORKING []\n");
                }

#ifdef ENABLE_CHANGEIDCMD
                if (strncmp((const char*)originalWord, "changeDstID: ",9) == 0)
                {
                    uint8_t dstid = originalWord[9] - '0';
                    debug("[L3] requesting to change to dest id %i\n", dstid);
                    L3_LLI_configReqFunc(L2L3_CFGTYPE_DSTID, dstid);
                }
                else if (strncmp((const char*)originalWord, "changeSrcID: ",9) == 0)
                {
                    uint8_t myid = originalWord[9] - '0';
                    debug("[L3] requesting to change to srce id %i\n", myid);
                    L3_LLI_configReqFunc(L2L3_CFGTYPE_SRCID, myid);
                }
                else
#endif
                {
                    //msg header setting
                    strcpy((char*)sdu, (char*)originalWord);
                    L3_LLI_dataReqFunc(sdu, wordLen);

                    debug_if(DBGMSG_L3, "[L3] sending msg....\n");
                }
                
                wordLen = 0;

                pc.printf("Give a word to send : ");

                L3_event_clearEventFlag(L3_event_dataToSend);
            }
            else if(L3_event_checkEventFlag(L3_event_MODEctrl_CNN))
            {  
                L3_LLI_configReqFunc(L2L3_CFGTYPE_DSTID, dstid);
                debug_if(DBGMSG_L3, "\n[L3]CHANGING ID!!! [%i]\n", dstid);
                L3_event_clearEventFlag(L3_event_MODEctrl_CNN);
                //main_state = L3STATE_IDLE;
                //pc.printf("\n:: ENTER THE MODE ::\n1 : DND MODE, 2 : CONNECTION MODE\n");
            }
           /* else if(L3_event_checkEventFlag(L3_event_MODEctrlRcvd_EXIT))
            {
                pc.printf("\nEXITING THIS CHATTING\n");
                main_state = L3STATE_IDLE;                                          ///////state=CNN이고
            }
            */
        break;

        default :
            break;
        
    }
}