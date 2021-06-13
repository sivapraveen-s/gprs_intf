/*
 * File Name: modem.h
 *
 * Author: Ram Krishnan
 *         rkris@wisense.in
 *
 * Created: Dec/18/2016
 */

#ifndef __MODEM_H__
#define __MODEM_H__ 

#include <ip.h>
#include <wsuart.h>

typedef struct
{
  char operator[32];
  char nwkCode[32];
  char apn[32];
} GSM_operatorInfo_s;

typedef enum
{
   MODEM_STS_SUCCESS, 
   MODEM_STS_GENERIC_FLR, 
   MODEM_STS_INVALID_RESPONSE, 
   MODEM_STS_INVALID_PARAM, 
   MODEM_STS_INCOMPLETE_RESPONSE, 
   MODEM_STS_RESPONSE_FMT_BAD, 
   MODEM_STS_NO_RESPONSE, 
   MODEM_STS_UART_OPEN_FLR,
   MODEM_STS_UART_CFG_FLR,
   MODEM_STS_UART_WRITE_FLR,
   MODEM_STS_UART_READ_FLR,
   MODEM_STS_SIM_NOT_PRESENT,
   MODEM_STS_GSM_REG_NOT_DONE,
   MODEM_STS_TCP_CONN_DOWN,
   MODEM_STS_SELECT_FLR,
   MODEM_STS_REQUESTED_OPN_FAILED,
   MODEM_STS_IPADDR_NOT_ASSIGNED,
   MODEM_STS_TCP_CONN_ALREADY_OPEN
} MODEM_sts_t;

typedef enum
{
   MODEM_GSM_REG_STS_UNKNOWN,
   MODEM_GSM_REG_STS_DONE_HOME,
   MODEM_GSM_REG_STS_DONE_ROAMING,
   MODEM_GSM_REG_STS_NOT_DONE_SEARCHING,
   MODEM_GSM_REG_STS_NOT_DONE_NOT_SEARCHING,
   MODEM_GSM_REG_STS_REGN_DENIED
} MODEM_gsmRegSts_t;

typedef enum
{
  MODEM_OPERATOR_SEL_MODE_MANUAL = 1,
  MODEM_OPERATOR_SEL_MODE_AUTO,
  MODEM_OPERATOR_SEL_MODE_AUTO_ON_MANUAL_FLR
} MODEM_operatorSelMode_t;

typedef enum
{
  MODEM_REQ_GSM_DEREG_OPN = 1,
  MODEM_REQ_GSM_REG_OPN = 2
} MODEM_gsmRegDeregOpn_t;

typedef enum
{
   MODEM_TCP_SOCK_STATE_UNKNOWN,
   MODEM_TCP_SOCK_STATE_OPEN,
   MODEM_TCP_SOCK_STATE_SUSPENDED,
   MODEM_TCP_SOCK_STATE_CLOSED
} MODEM_tcpSockState_t;

typedef struct
{
   int cmdTxCnt;
   int cmdRespRcvdCnt;
   int cmdRespTmoCnt;
} MODEM_stats_s;

typedef struct
{
  char gpsEna;
  char battMonEna;
  int gsmRegStatus;
  MODEM_operatorSelMode_t operatorSelMode;
  MODEM_tcpSockState_t tcpSockState;
  unsigned char ipv4Addr[IP_V4_ADDR_LEN];
  int dataTxTimeoutSecs;
  int connEstTmoSecs;
  int dataExchangeTmoSecs;
  char mfr[32];
  char partNr[32];
  UART_cntxt_s  UART_cntxt;
#ifdef MODEM_PART_NR_SIM808
  char gpsPwrOn;
  double latInDeg, longInDeg;
  char utcTimeStr1[16];
  char utcTimeStr2[8];

  int battChargeLevel;  // In %
  int battV; // In milli-volts
#endif
  MODEM_stats_s  stats; 
  char operatorStr[256];
} MODEM_cntxt_s;

extern MODEM_sts_t MODEM_sendCmd(MODEM_cntxt_s *cntxt_p, char *cmdStr);
extern MODEM_sts_t MODEM_getResp(MODEM_cntxt_s *cntxt_p, char *buff_p, 
                                 const int buffLen, int *respLen_p);

extern MODEM_sts_t MODEM_waitForExactResp(MODEM_cntxt_s *cntxt_p, char *rxBuff_p,
                                          char *expResp_p, int expRespLen, int tmoSecs);

#endif
