/*
 * File Name: modem_api.h
 *
 * Author: Ram Krishnan
 *         rkris@wisense.in
 *
 * Created: Dec/18/2016
 */

#ifndef __MODEM_API_H__
#define __MODEM_API_H_ 

#if defined(MODEM_PART_NR_GL868)

#define MODEM_getGSMRegSts(cntxt_p)  GL868_getGSMRegSts(cntxt_p)
#define MODEM_getOperator(cntxt_p)  GL868_getOperator(cntxt_p)
#define MODEM_openTCPConn(cntxt_p, ipAddr, tcpPort)  GL868_openTCPConn(cntxt_p, ipAddr, tcpPort)
#define MODEM_suspendTCPConn(cntxt_p)  GL868_suspendTCPConn(cntxt_p)
#define MODEM_getIPAddr(cntxt_p, apn_p)  GL868_getIPAddr(cntxt_p, apn_p)
#define MODEM_disableEcho(cntxt_p)  GL868_disableEcho(cntxt_p)
#define MODEM_checkSIMPresence(cntxt_p)  GL868_checkSIMPresence(cntxt_p) 
#define MODEM_verifyMfrAndPartNr(cntxt_p)  GL868_verifyMfrAndPartNr(cntxt_p) 
#define MODEM_cfgOperatorSelMode(cntxt_p, regMode)  GL868_cfgOperatorSelMode(cntxt_p, regMode)
#define MODEM_reqGSMRegDeregOpn(cntxt_p, opn, param)  GL868_reqGSMRegDeregOpn(cntxt_p, opn, param)
#define MODEM_sendTCPData(cntxt_p, msg_p, msgLen)  GL868_sendTCPData(cntxt_p, msg_p, msgLen)  
#define MODEM_closeTCPConn(cntxt_p) GL868_closeTCPConn(cntxt_p)
#define MODEM_getNwkOpsInVicinity(cntxt_p) GL868_getNwkOpsInVicinity(cntxt_p)
#define MODEM_delAllPDPCntxt(cntxt_p)  GL868_delAllPDPCntxt(cntxt_p)

#elif defined(MODEM_PART_NR_SIM808)

#define MODEM_getGSMRegSts(cntxt_p)  SIM808_getGSMRegSts(cntxt_p)
#define MODEM_getOperator(cntxt_p)  SIM808_getOperator(cntxt_p)
#define MODEM_openTCPConn(cntxt_p, ipAddr, tcpPort)  SIM808_openTCPConn(cntxt_p, ipAddr, tcpPort)
#define MODEM_getIPAddr(cntxt_p, apn_p)  SIM808_getIPAddr(cntxt_p, apn_p)
#define MODEM_disableEcho(cntxt_p)  SIM808_disableEcho(cntxt_p)
#define MODEM_checkSIMPresence(cntxt_p)  SIM808_checkSIMPresence(cntxt_p) 
#define MODEM_verifyMfrAndPartNr(cntxt_p)  SIM808_verifyMfrAndPartNr(cntxt_p) 
#define MODEM_reqGSMRegDeregOpn(cntxt_p, opn, param)  SIM808_reqGSMRegDeregOpn(cntxt_p, opn, param)
#define MODEM_sendTCPData(cntxt_p, msg_p, msgLen)  SIM808_sendTCPData(cntxt_p, msg_p, msgLen)  
#define MODEM_closeTCPConn(cntxt_p)  SIM808_closeTCPConn(cntxt_p)
#define MODEM_doGPSPwrCntrlOpn(cntxt_p, opn)  SIM808_doGPSPwrCntrlOpn(cntxt_p, opn)
#define MODEM_getGPSOpInfo(cntxt_p)  SIM808_getGPSOpInfo(cntxt_p)
#define MODEM_getBattInfo(cntxt_p)  SIM808_getBattInfo(cntxt_p)

#else
#error Modem Part Number Not Specified  !!
#endif

#endif
