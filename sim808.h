/*
 * File Name: sim808.h
 *
 * Author: Ram Krishnan
 *         rkris@wisense.in
 *
 * Created: Feb/6/2018
 */

#ifndef __SIM808_H__
#define __SIM808_H__

#include <modem.h>

#define SIM808_PDP_CNTXT_ID_MIN_VAL  0x1
#define SIM808_PDP_CNTXT_ID_MAX_VAL  0x5

typedef enum
{
  SIM808_CREG_STAT_NOT_REGD_NOT_SEARCHING = 0,
  SIM808_CREG_STAT_REG_DONE_HOME_NWK = 1,
  SIM808_CREG_STAT_NOT_REGD_SEARCHING = 2,
  SIM808_CREG_STAT_REGN_DENIED = 3,
  SIM808_CREG_STAT_UNKNOWN =  4,
  SIM808_CREG_STAT_REG_DONE_ROAMING = 5,
} SIM808_creg_stat_t;

extern int SIM808_checkForIPAddr(MODEM_cntxt_s *cntxt_p);

extern MODEM_sts_t SIM808_getIPAddr(MODEM_cntxt_s *cntxt_p, const char *apn_p);

extern MODEM_sts_t SIM808_sendTCPData(MODEM_cntxt_s *cntxt_p,
                                      unsigned char *msg_p,
                                      unsigned int msgLen);

extern void SIM808_closeTCPConn(MODEM_cntxt_s *cntxt_p);
 

extern MODEM_sts_t SIM808_openTCPConn(MODEM_cntxt_s *cntxt_p,
                                      char *remoteIP_p,
                                      int remotePort);

extern MODEM_sts_t SIM808_disableEcho(MODEM_cntxt_s *cntxt_p);

extern MODEM_sts_t SIM808_checkSIMPresence(MODEM_cntxt_s *cntxt_p);

extern MODEM_sts_t SIM808_verifyMfrAndPartNr(MODEM_cntxt_s *cntxt_p);


extern MODEM_sts_t SIM808_reqGSMRegDeregOpn(MODEM_cntxt_s *cntxt_p, 
                                            MODEM_gsmRegDeregOpn_t opn,                                  
                                            const char *nwkCode_p);

extern MODEM_sts_t SIM808_getOperator(MODEM_cntxt_s *cntxt_p);

extern MODEM_sts_t SIM808_getGSMRegSts(MODEM_cntxt_s *cntxt_p);

extern MODEM_sts_t SIM808_getBattInfo(MODEM_cntxt_s *cntxt_p);

extern MODEM_sts_t SIM808_getGPSPwrCntrlPinSts(MODEM_cntxt_s *cntxt_p);

extern MODEM_sts_t SIM808_doGPSPwrCntrlOpn(MODEM_cntxt_s *cntxt_p, char opn);

extern MODEM_sts_t SIM808_getGPSOpInfo(MODEM_cntxt_s *cntxt_p);

#endif
