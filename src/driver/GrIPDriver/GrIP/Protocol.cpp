#include "Protocol.h"
#include "GrIP.h"
#include <QDebug>


void Protocol_RequestDeviceInfo()
{
    uint8_t msg = SYSTEM_REPORT_INFO;
    GrIP_Pdu_t p = {&msg, 1};

    GrIP_Transmit(PROT_GrIP, MSG_SYSTEM_CMD, RET_OK, &p);
}


void Protocol_SetStatusLED(StatusLedState_e state)
{
    uint8_t msg[2] = {};
    GrIP_Pdu_t p = {msg, 2};

    msg[0] = SYSTEM_SET_STATUS;
    switch (state)
    {
    case LED_OFF:
        msg[1] = 0;
        break;

    case LED_RED:
        msg[1] = 1;
        break;

    case LED_GREEN:
        msg[1] = 2;
        break;

    case LED_ORANGE:
        msg[1] = 3;
        break;

    default:
        msg[1] = 0;
        break;
    }

    GrIP_Transmit(PROT_GrIP, MSG_SYSTEM_CMD, RET_OK, &p);
}


void Protocol_SendCANCfg(uint32_t can1_baud, uint32_t can2_baud)
{
    uint8_t msg[9] = {};
    GrIP_Pdu_t p = {msg, 9};

    // Set cmd
    msg[0] = SYSTEM_SEND_CAN_CFG;

    msg[1] = (can1_baud >> 24) & 0xFF;
    msg[2] = (can1_baud >> 16) & 0xFF;
    msg[3] = (can1_baud >> 8) & 0xFF;
    msg[4] = (can1_baud) & 0xFF;

    msg[5] = (can2_baud >> 24) & 0xFF;
    msg[6] = (can2_baud >> 16) & 0xFF;
    msg[7] = (can2_baud >> 8) & 0xFF;
    msg[8] = (can2_baud) & 0xFF;

    GrIP_Transmit(PROT_GrIP, MSG_SYSTEM_CMD, RET_OK, &p);
}


void Protocol_SendLINCfg(Protocol_LinCfg_t *lin1, Protocol_LinCfg_t *lin2)
{
    uint8_t msg[15] = {};
    GrIP_Pdu_t p = {msg, 15};

    // Set cmd
    msg[0] = SYSTEM_SEND_LIN_CFG;

    // LIN1
    msg[1] = (lin1->Baudrate >> 8) & 0xFF;
    msg[2] = (lin1->Baudrate) & 0xFF;

    msg[3] = lin1->Timebase;

    msg[4] = (lin1->Jitter >> 8) & 0xFF;
    msg[5] = (lin1->Jitter) & 0xFF;

    msg[6] = lin1->Mode;

    msg[7] = lin1->Protocol;

    // LIN2
    msg[8] = (lin2->Baudrate >> 8) & 0xFF;
    msg[9] = (lin2->Baudrate) & 0xFF;


    msg[10] = lin2->Timebase;

    msg[11] = (lin2->Jitter >> 8) & 0xFF;
    msg[12] = (lin2->Jitter) & 0xFF;

    msg[13] = lin2->Mode;

    msg[14] = lin2->Protocol;

    GrIP_Transmit(PROT_GrIP, MSG_SYSTEM_CMD, RET_OK, &p);
}


void Protocol_StartStopCAN(bool start_can1, bool start_can2)
{
    uint8_t msg[3] = {};
    GrIP_Pdu_t p = {msg, 3};

    // Set cmd
    msg[0] = SYSTEM_START_CAN;
    msg[1] = static_cast<uint8_t>(start_can1);
    msg[2] = static_cast<uint8_t>(start_can2);

    GrIP_Transmit(PROT_GrIP, MSG_SYSTEM_CMD, RET_OK, &p);
}


void Protocol_StartStopLIN(bool start_lin1, bool start_lin2)
{
    uint8_t msg[3] = {};
    GrIP_Pdu_t p = {msg, 3};

    // Set cmd
    msg[0] = SYSTEM_START_LIN;
    msg[1] = static_cast<uint8_t>(start_lin1);
    msg[2] = static_cast<uint8_t>(start_lin2);

    GrIP_Transmit(PROT_GrIP, MSG_SYSTEM_CMD, RET_OK, &p);
}


void Protocol_AddCANFrame(CAN_Msg_t *can)
{
    uint8_t msg[20] = {};
    GrIP_Pdu_t p = {msg, 20};

    // Set cmd
    msg[0] = SYSTEM_SEND_CAN_FRAME;

    msg[1] = can->Channel;

    msg[2] = (can->ID >> 24) & 0xFF;
    msg[3] = (can->ID >> 16) & 0xFF;
    msg[4] = (can->ID >> 8) & 0xFF;
    msg[5] = (can->ID) & 0xFF;

    msg[6] = can->DLC;

    msg[7] = can->Flags;

    msg[8] = (can->Time >> 24) & 0xFF;
    msg[9] = (can->Time >> 16) & 0xFF;
    msg[10] = (can->Time >> 8) & 0xFF;
    msg[11] = (can->Time) & 0xFF;

    for(int i = 0; i < can->DLC; i++)
    {
        msg[12+i] = can->Data[i];
    }

    GrIP_Transmit(PROT_GrIP, MSG_SYSTEM_CMD, RET_OK, &p);
}


void Protocol_AddLINFrame(LIN_Frame_t *lin)
{
    uint8_t msg[15] = {};
    GrIP_Pdu_t p = {msg, 15};

    // Set cmd
    msg[0] = SYSTEM_ADD_LIN_FRAME;

    msg[1] = lin->Channel;

    msg[2] = lin->Direction;

    msg[3] = (lin->Delay >> 8) & 0xFF;
    msg[4] = (lin->Delay) & 0xFF;

    msg[5] = lin->ID;

    msg[6] = lin->Len;

    for(int i = 0; i < lin->Len; i++)
    {
        msg[7+i] = lin->Data[i];
    }

    GrIP_Transmit(PROT_GrIP, MSG_SYSTEM_CMD, RET_OK, &p);
}

