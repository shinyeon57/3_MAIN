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

static uint8_t sdu[200];

//***********************************************************************************************//
static uint8_t originalMode[200];           //mode설정에서 누른 키가 inputword함수로 실행되는 기현상.....
static uint8_t modeLen=0;

static uint8_t check_dnd=0;                 //dnd mode 확인용 변수 (0 : dnd(x) || 1 : dnd(o))
//***********************************************************************************************//

//serial port interface
static Serial pc(USBTX, USBRX);


//application event handler : generating SDU from keyboard input
static void L3service_processInputWord(void)
{
    char c = pc.getc();
    if (!L3_event_checkEventFlag(L3_event_dataToSend))                      
    {
        if (c == '\n' || c == '\r')
        {
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
            else
            {
                if (strncmp((const char*)originalWord, "#DND ", 4) == 0)             ////#DND입력                                        
                {    
                    check_dnd = 1;                                                  //dnd mode check용   
                    L3_event_setEventFlag(L3_event_MODEctrlRcvd_DND);
                    pc.printf("\n DND MODE ON!\n");
                    L3_dnd_timer_startTimer();                                      ////dnd용 2시간 timer시작한다 
                }
                else if (strncmp((const char*)originalWord, "#EXIT ", 5) == 0)       ////#EXIT입력
                {
                    L3_event_setEventFlag(L3_event_MODEctrlRcvd_EXIT);               
                    pc.printf("\nEXITING THIS CHATTING\n");
                }
            }
        }
    }
    //L3_event_clearEventFlag(L3_event_MODEctrlRcvd);   
}


//mode 진입을 위한 제어 key SDU 생성=====================================
void L3service_processInputMode(void)
{ 
    L3_event_setEventFlag(L3_event_MODEctrlRcvd);              

    pc.printf("\n:: ENTER THE MODE ::\n1 : DND MODE, 2 : CONNECTION MODE\n");      //**************************************************//
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
        L3_LLI_configReqFunc(L2L3_CFGTYPE_DSTID, dstid);  
        dstid_return = dstid;
        pc.printf("\n:: ID for the destination : %i\n", dstid_return);     
    }  

    return dstid_return;
}

void L3service_checkDstDNDmode(void)
{
    if(check_dnd == 0)
    {
        //내가 방해금지모드 설정해놨어 그러면 상대방이 내 아이디로 연결 요청을 했을 때 거절해야됨. 
        //거절한다 = 거절 문구와 상대방 state를 idle로 보낸다. & 나는 그대로 dnd mode를 유지한다.
        //DNDstate에서 else if(즉, dnd on 상태일 때) 이때 뭔가 계속 거절의사를 쏘는 방향으로 해야할 거 같은데?
        
    }
}

void L3_initFSM()
{
    //L3service_processInputMode();
    //initialize service layer
    pc.attach(&L3service_processInputWord, Serial::RxIrq);

    pc.printf("::Give a word to send : ");                          //////////:: for test
}

void L3_FSMrun(void)
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
            
            //L3service_processInputMode();                   

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
                L3_event_setEventFlag(L3_event_MODEctrl_CNN);
                main_state = L3STATE_CNN;
            }
            else if(L3_event_checkEventFlag(L3_event_MODEctrlRcvd_DND))
            {
                L3_event_clearEventFlag(L3_event_MODEctrlRcvd_DND);
                L3_event_setEventFlag(L3_event_MODEctrl_DND);
                main_state = L3STATE_DND;
            }
            break;

        case L3STATE_DND :                                               ///////////////////state = dnd
            if(L3_event_checkEventFlag(L3_event_MODEctrl_DND))       
            {
                if(L3_dnd_timer_getTimerStatus() == 0)                    ///////////////////dnd_timeout event happens
                {
                    check_dnd = 0;                                                  //dnd mode check용 
                    L3_event_clearEventFlag(L3_event_MODEctrl_DND);
                    pc.printf("DND MODE END!\n");
                    main_state = L3STATE_IDLE ;
                }
                else
                {
                    pc.printf("%i is on DND MODE!\n", L3service_settingInputId());      
                }
            }
            break;

        case L3STATE_CNN :                                              ///////////////////state = cnn

            if(L3_event_checkEventFlag(L3_event_MODEctrl_CNN))
            {  
                L3service_settingInputId(); 
                L3_event_clearEventFlag(L3_event_MODEctrl_CNN);
                //main_state = L3STATE_CNN;
            }
            else if(L3_event_checkEventFlag(L3_event_MODEctrlRcvd_EXIT))
            {
                pc.printf("\nEXITING THIS CHATTING\n");
                main_state = L3STATE_IDLE;                                          ///////state=CNN이고
            }
        break;

        default :
            break;
        
    }
}