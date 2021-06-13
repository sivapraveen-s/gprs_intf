/*
 * File Name: gl868.h
 *
 * Author: Ram Krishnan
 *         rkris@wisense.in
 *
 * Created: Dec/18/2016
 */

#ifndef __GL868_H__
#define __GL868_H_ 

#define GL868_DISABLE_ECHO_CONTROL_CMD_ID  0x2
#define GL868_DISABLE_FLOW_CONTROL_CMD_ID  0x4
#define GL868_GET_GSM_REG_STS_CMD_ID  0x6
#define GL868_GET_NWK_OPERATOR_CMD_ID  0x7
#define GL868_DEL_PDP_CNTXT_CMD_ID  0x9
#define GL868_GET_NWK_OPS_IN_VICINITY 0xe

/*
 * AT+CGDCONT=?
 * Test command returns values supported as a compound value
 *
 * Example:
 *
 * AT+CGDCONT=?
 * +CGDCONT: (1-5),"IP",,,(0,1),(0,1)
 * +CGDCONT: (1-5),"IPV6",,,(0,1),(0,1)
 *
 * OK
 */
#define GL868_PDP_CNTXT_ID_MIN_VAL  0x1
#define GL868_PDP_CNTXT_ID_MAX_VAL  0x5

/*
 * Always use one CID
 */
#define GL868_PDP_CNTXT_ACTIVE_CNTXT_ID  0x1
#define GL868_ACTIVE_SOCKET_CONN_ID  0x1

typedef enum
{
  GL868_CREG_STAT_NOT_REGD_NOT_SEARCHING = 0,
  GL868_CREG_STAT_REG_DONE_HOME_NWK = 1,
  GL868_CREG_STAT_NOT_REGD_SEARCHING = 2,
  GL868_CREG_STAT_REGN_DENIED = 3,
  GL868_CREG_STAT_UNKNOWN =  4,
  GL868_CREG_STAT_REG_DONE_ROAMING = 5,
} GL868_creg_stat_t;


extern MODEM_sts_t GL868_getGSMRegSts(MODEM_cntxt_s  *cntxt_p);
extern MODEM_sts_t GL868_getOperator(MODEM_cntxt_s  *cntxt_p);
extern MODEM_sts_t GL868_delAllPDPCntxt(MODEM_cntxt_s  *cntxt_p);
extern MODEM_sts_t GL868_createPDPCntxt(MODEM_cntxt_s *cntxt_p, const char *apn_p);
extern MODEM_sts_t GL868_getIPAddr(MODEM_cntxt_s *cntxt_p, const char *apn_p);
extern MODEM_sts_t GL868_deactivatePDPCntxt(MODEM_cntxt_s *cntxt_p);
extern MODEM_sts_t GL868_openTCPConn(MODEM_cntxt_s *cntxt_p, char *ip, int tcpPort);
extern MODEM_sts_t GL868_suspendTCPConn(MODEM_cntxt_s *cntxt_p);
extern MODEM_sts_t GL868_disableEcho(MODEM_cntxt_s *cntxt_p);
extern MODEM_sts_t GL868_checkSIMPresence(MODEM_cntxt_s *cntxt_p);
unsigned char *GL868_removeSockSuspendEscSeq(unsigned char *msg_p, int *msgLen_p);
extern MODEM_sts_t GL868_verifyMfrAndPartNr(MODEM_cntxt_s *cntxt_p);
extern MODEM_sts_t GL868_cfgOperatorSelMode(MODEM_cntxt_s *cntxt_p, MODEM_operatorSelMode_t regMode);
extern MODEM_sts_t GL868_reqGSMRegDeregOpn(MODEM_cntxt_s *cntxt_p, 
                                           MODEM_gsmRegDeregOpn_t opn, 
                                           const char *nwkNode_p);
extern MODEM_sts_t GL868_sendTCPData(MODEM_cntxt_s *cntxt_p,
                                     unsigned char *msg_p,
                                     unsigned int msgLen);
extern void GL868_closeTCPConn(MODEM_cntxt_s *cntxt_p);
extern MODEM_sts_t GL868_getNwkOpsInVicinity(MODEM_cntxt_s *cntxt_p);


#endif
