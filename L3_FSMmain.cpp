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
            if(originalWord[0] == '1')                                      //dnd mode 
            {
                L3_event_setEventFlag(L3_event_MODEctrlRcvd_DND);

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
            #if 1 //only for SY's macbook
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
                if (strncmp((const char*)originalWord, "#DND ", 4) == 0)             ////#DND입력                                        
                {                                                
                    L3_event_setEventFlag(L3_event_MODEctrlRcvd_DND);
                    pc.printf("\n DND MODE ON!\n");
                }
                else if (strncmp((const char*)originalWord, "#EXIT ", 5) == 0)       ////#EXIT입력
                {
                    L3_event_setEventFlag(L3_event_MODEctrlRcvd_EXIT);               
                    debug_if(DBGMSG_L3, "\nEXITING THIS CHATTING\n");
                }
                originalWord[wordLen++] = '\0'; 
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
            }
        }
    }
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

void L3_FSMrun()
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
            if(L3_event_checkEventFlag(L3_event_MODEctrlRcvd_CNN))
            {
                L3_event_clearEventFlag(L3_event_MODEctrlRcvd_CNN);  
                L3_event_setEventFlag(L3_event_MODEctrl_CNN);
                main_state = L3STATE_CNN;
            }
            else if(L3_event_checkEventFlag(L3_event_MODEctrlRcvd_DND))
            {
                L3_event_clearEventFlag(L3_event_MODEctrlRcvd_DND);
                L3_event_setEventFlag(L3_event_MODEctrl_DND);
                L3_dnd_timer_startTimer();
                pc.printf("DND MODE ON!\n"); 
                main_state = L3STATE_DND;
            }
            break;

        case L3STATE_DND :                                               ///////////////////state = dnd
            if(L3_event_checkEventFlag(L3_event_MODEctrl_DND))       
            { 
                if(L3_dnd_timer_getTimerStatus() == 0)                    ///////////////////dnd_timeout event happens
                {
                    L3_event_clearEventFlag(L3_event_MODEctrl_DND);
                    pc.printf("DND MODE END!\n");
                    //dstid 초기화(연결끊기)
                    dstid = 0; 
                    L3_LLI_configReqFunc(L2L3_CFGTYPE_DSTID, dstid);                    
                    main_state = L3STATE_IDLE ;
                    pc.printf("\n:: ENTER THE MODE ::\n1 : DND MODE, 2 : CONNECTION MODE\n");                  
                }
            }
            break;

        case L3STATE_CNN :                                             
            
            if(L3_event_checkEventFlag(L3_event_msgRcvd)) 
            {
                //Retrieving data info.
                uint8_t* dataPtr = L3_LLI_getMsgPtr();
                uint8_t size = L3_LLI_getSize();

                if(strncmp((const char*)dataPtr, "#DND",4) == 0)
                {
                    pc.printf("\n%i is on DND MODE! EXIT THE CHATTING!\n", dstid);      //상대가 dnd 모드일때
                    main_state = L3STATE_IDLE;
                    dstid = 0; 
                    L3_LLI_configReqFunc(L2L3_CFGTYPE_DSTID, dstid); 
                    pc.printf("\n:: ENTER THE MODE ::\n1 : DND MODE, 2 : CONNECTION MODE\n");
                    
                }
                else if(strncmp((const char*)dataPtr, "#EXIT",5) == 0)
                {
                    pc.printf("\nDst :%i is EXIT! EXIT THE CHATTING!\n", dstid);
                    main_state = L3STATE_IDLE;
                    //dstid 초기화(연결끊기)
                    dstid = 0; 
                    L3_LLI_configReqFunc(L2L3_CFGTYPE_DSTID, dstid); 
                    pc.printf("\n:: ENTER THE MODE ::\n1 : DND MODE, 2 : CONNECTION MODE\n");
                }
                else
                {
                debug("\n -------------------------------------------------\nRCVD MSG : %s (length:%i)\n -------------------------------------------------\n", 
                            dataPtr, size);
                }
                pc.printf("::: ");
                
                L3_event_clearEventFlag(L3_event_msgRcvd);
            }
            else if (L3_event_checkEventFlag(L3_event_dataToSend)) //if data needs to be sent (keyboard input)
            {
                if(L3_event_checkEventFlag(L3_event_MODEctrl_DND))       
                {
                if(L3_dnd_timer_getTimerStatus() == 0)                   
                {                                                
                    L3_event_clearEventFlag(L3_event_MODEctrl_DND);
                    pc.printf("DND MODE END!\n");                   //dnd 모드 설정한 애한테
                    main_state = L3STATE_IDLE ;
                    dstid = 0; 
                    L3_LLI_configReqFunc(L2L3_CFGTYPE_DSTID, dstid);
                    pc.printf("\n:: ENTER THE MODE ::\n1 : DND MODE, 2 : CONNECTION MODE\n");               
                }
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

                pc.printf("::: ");

                L3_event_clearEventFlag(L3_event_dataToSend);
            }
            else if(L3_event_checkEventFlag(L3_event_MODEctrl_CNN))
            {  
                L3_LLI_configReqFunc(L2L3_CFGTYPE_DSTID, dstid);
                //여기서 방해금지모드라고 알려줘야됨.
                pc.printf("DstID is %i \n", dstid);
                debug_if(DBGMSG_L3, "\n[L3]CHANGING ID!!! [%i]\n", dstid);
                L3_event_clearEventFlag(L3_event_MODEctrl_CNN);
            }
            else if(L3_event_checkEventFlag(L3_event_MODEctrlRcvd_DND))
            {
                L3_event_clearEventFlag(L3_event_MODEctrlRcvd_DND);
                L3_event_setEventFlag(L3_event_MODEctrl_DND);
                L3_dnd_timer_startTimer();
                wordLen = 0; 
                main_state = L3STATE_DND;
            }            
            else if(L3_event_checkEventFlag(L3_event_MODEctrlRcvd_EXIT))
            {
                pc.printf("\nEXITING THIS CHATTING!\n");
                L3_event_clearEventFlag(L3_event_MODEctrlRcvd_EXIT);
                dstid = 0; 
                L3_LLI_configReqFunc(L2L3_CFGTYPE_DSTID, dstid);
                wordLen = 0; 
                pc.printf("\n:: ENTER THE MODE ::\n1 : DND MODE, 2 : CONNECTION MODE\n"); 
                main_state = L3STATE_IDLE;                                     
            }
        break;

        default :
            break;
        
    }
}