#ifdef MODEM_PART_NR_SIM808

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <math.h>

#include <ip.h>
#include <modem.h>
#include <sim808.h>

char MODEM_respBuff[1024];

extern int verbose;



/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t SIM808_waitForResp(MODEM_cntxt_s *cntxt_p, char *respBuff_p, 
                               int buffLen, int *respLen_p, int tmoMilliSecs)
{
   MODEM_sts_t sts;
   time_t  time0, time1;
   int tmoSecs = (tmoMilliSecs/1000) + 1;

   printf("<%s> tmo - %d milli / %d secs \n",
          __FUNCTION__, tmoMilliSecs, tmoSecs);

   if (tmoMilliSecs > 0)
       time(&time0);

   do
   {
       if (verbose)
           printf("<%s> Calling MODEM_getResp() \n", __FUNCTION__);
 
       sts = MODEM_getResp(cntxt_p, respBuff_p, buffLen, respLen_p);
       if (sts == MODEM_STS_SUCCESS)
           break;

       if (sts == MODEM_STS_NO_RESPONSE)
       {
           printf("<%s> No response yet !! \n", __FUNCTION__);
       }
       else
       {
           printf("<%s> error  !!  \n", __FUNCTION__);
       }
    
       if (tmoMilliSecs > 0)
       {
           int diffT;

           time(&time1);
           diffT = (int)difftime(time1, time0);
           if (diffT > tmoSecs)
           {
               printf("<%s> timed out .. no  response for %d milli-secs \n",
                      __FUNCTION__, tmoMilliSecs);
               break;
           }
       }

       sleep(1);

   } while (1);

   return sts;
}


/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t SIM808_disableEcho(MODEM_cntxt_s *cntxt_p)
{
   MODEM_sts_t sts;
   int respLen;

   sts = MODEM_sendCmd(cntxt_p, "ATE0\r\n");
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>MODEM_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }

   sleep(1);

   return sts;
}


/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t SIM808_checkSIMPresence(MODEM_cntxt_s *cntxt_p)
{
   MODEM_sts_t sts;
   char cmdBuff[64];
   int respLen;
   
   /*
    * With local echo disabled ...
    *
    * > When SIM is inserted:
    * AT+CPIN? 
    *
    * +CPIN: READY
    *
    * OK
    *
    * > When SIM is not inserted:
    * AT+CPIN?
    *
    * ERROR
    */

   sprintf(cmdBuff, "AT+CPIN?\r\n");
   printf("<%s> Sending Cmd %s \n", __FUNCTION__,  cmdBuff);
   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>MODEM_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }

   sts = SIM808_waitForResp(cntxt_p, MODEM_respBuff, 
                            sizeof(MODEM_respBuff), &respLen, 10000);
   if (sts != MODEM_STS_SUCCESS)
       return sts;

   MODEM_respBuff[respLen] = '\0';

   printf("<%s>Received response - len<%d> / Resp <%s> \n",
          __FUNCTION__, respLen, MODEM_respBuff);

   if (strstr(MODEM_respBuff, "+CPIN: READY") != NULL)
   {
       sts =  MODEM_STS_SUCCESS;
   }
   else
   {
       if (strstr(MODEM_respBuff, "ERROR") != NULL)
           sts =  MODEM_STS_SIM_NOT_PRESENT;
       else
           sts =  MODEM_STS_INVALID_RESPONSE;
   }

   return sts;
}


/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
int SIM808_checkForIPAddr(MODEM_cntxt_s *cntxt_p)
{
   MODEM_sts_t sts;
   char cmdBuff[64];
   int respLen;
   
   sprintf(cmdBuff, "AT+CIFSR\r\n");
   printf("<%s> Sending Cmd %s \n", __FUNCTION__,  cmdBuff);
   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>MODEM_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }

   sts = MODEM_getResp(cntxt_p, MODEM_respBuff, 
                       sizeof(MODEM_respBuff), &respLen);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s> MODEM_getResp() failed !! \n", __FUNCTION__);
       return sts;
   }
    
   MODEM_respBuff[respLen] = '\0';

   printf("<%s>Received response - len<%d> / Resp <%s> \n",
          __FUNCTION__, respLen, MODEM_respBuff);

   sts = MODEM_STS_IPADDR_NOT_ASSIGNED;

   if (strstr(MODEM_respBuff, "ERROR") == NULL)
   {
      int fieldCnt, ipv4[4];

      fieldCnt = sscanf(MODEM_respBuff, "%d.%d.%d.%d", 
                        &(ipv4[0]), &(ipv4[1]), &(ipv4[2]), &(ipv4[3]));
      if (fieldCnt == 4)
      {
          printf("<%s> Assigned IP address is <%d.%d.%d.%d> \n",
                 __FUNCTION__, ipv4[0], ipv4[1], ipv4[2], ipv4[3]);
          sts = MODEM_STS_SUCCESS;
      }
   }

   return sts;
}


/*
 * Command: AT+CIPSTATUS\r\n
 * 0 IP INITIAL
 * 1 IP START
 * 2 IP CONFIG
 * 3 IP GPRSACT
 * 4 IP STATUS
 * 5 TCP CONNECTING/UDP CONNECTING/SERVER LISTENING
 * 6 CONNECT OK
 * 7 TCP CLOSING/UDP CLOSING
 * 8 TCP CLOSED/UDP CLOSED
 * 9 PDP DEACT
 */


/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t SIM808_getIPAddr(MODEM_cntxt_s *cntxt_p, const char *apn_p)
{
   char cmdBuff[64];
   MODEM_sts_t sts;
   int loopIdx = 0, respLen = 0;

   sprintf(cmdBuff, "AT+CGATT?\r\n");
   printf("<%s> Sending Cmd %s \n", __FUNCTION__,  cmdBuff);
   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>MODEM_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }

   sts = MODEM_getResp(cntxt_p, MODEM_respBuff, 
                       sizeof(MODEM_respBuff), &respLen);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s> MODEM_getResp() failed !! \n", __FUNCTION__);
       return sts;
   }

   MODEM_respBuff[respLen] = '\0';

   printf("<%s>Received response - len<%d> / Resp <%s> \n",
          __FUNCTION__, respLen, MODEM_respBuff);
   
   // First check if IP address is already assigned
   sts = SIM808_checkForIPAddr(cntxt_p);
   if (sts == MODEM_STS_SUCCESS)
   {
       printf("<%s> IP address already assigned .... \n", __FUNCTION__);
       // RAM-HACK
       return sts;
   }
   else
       printf("<%s> IP address not assigned .... \n", __FUNCTION__);

_loop:
   // Start Task and Set APN, USER NAME, PASSWORD 
   // This often returns error - ?
   // This command is valid only when the state is IP_INITIAL.
   sprintf(cmdBuff, "AT+CSTT=\"%s\"\r\n", apn_p);
   printf("<%s> Sending Cmd %s \n", __FUNCTION__,  cmdBuff);
   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>MODEM_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }

   sts = MODEM_getResp(cntxt_p, MODEM_respBuff, 
                       sizeof(MODEM_respBuff), &respLen);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s> MODEM_getResp() failed !! \n", __FUNCTION__);
       return sts;
   }
    
   MODEM_respBuff[respLen] = '\0';

   printf("<%s>Received response - len<%d> / Resp <%s> \n",
          __FUNCTION__, respLen, MODEM_respBuff);
   
   if (strcmp(MODEM_respBuff, "ERROR") == 0)
   {
       // AT+CIPSHUT: Deactivate GPRS PDP context
       sprintf(cmdBuff, "AT+CIPSHUT\r\n", apn_p);
       printf("<%s> Sending Cmd %s \n", __FUNCTION__,  cmdBuff);
       sts = MODEM_sendCmd(cntxt_p, cmdBuff);
       if (sts != MODEM_STS_SUCCESS)
       {
           printf("<%s>MODEM_sendCmd() failed !! \n", __FUNCTION__);
           return sts;
       }

       sts = MODEM_getResp(cntxt_p, MODEM_respBuff, 
                           sizeof(MODEM_respBuff), &respLen);
       if (sts != MODEM_STS_SUCCESS)
       {
           printf("<%s> MODEM_getResp() failed !! \n", __FUNCTION__);
           return sts;
       }
   
       MODEM_respBuff[respLen] = '\0';

       printf("<%s>Received response - len<%d> / Resp <%s> \n",
              __FUNCTION__, respLen, MODEM_respBuff);
       
       if (strcmp(MODEM_respBuff, "SHUT OK") != 0)
       {
           printf("<%s> expected %s, received %s \n", 
                  __FUNCTION__, "SHUT OK", MODEM_respBuff);
           sts = MODEM_STS_INVALID_RESPONSE;
       }

       sts = MODEM_STS_INVALID_RESPONSE;
   }
   else
   {
       if (strcmp(MODEM_respBuff, "OK") != 0)
       {
           printf("<%s> expected %s, received %s \n", 
                  __FUNCTION__, "OK", MODEM_respBuff);
           sts = MODEM_STS_INVALID_RESPONSE;
       }
   }

   if (sts != MODEM_STS_SUCCESS)
       return sts;

   // AT + CIICR : Bring Up Wireless Connection with GPRS or CSD 

   sprintf(cmdBuff, "AT+CIICR\r\n");
   printf("<%s> Sending Cmd %s \n", __FUNCTION__,  cmdBuff);
   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>MODEM_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }

   do
   {
      printf("<%s> Calling _getResp(%d) \n", __FUNCTION__, loopIdx++);
      sts = MODEM_getResp(cntxt_p, MODEM_respBuff, 
                          sizeof(MODEM_respBuff), &respLen);
      if (sts != MODEM_STS_SUCCESS)
      {
          printf("<%s> MODEM_getResp() failed !! \n", __FUNCTION__);
      }  
   } while (sts != MODEM_STS_SUCCESS);

   MODEM_respBuff[respLen] = '\0';

   printf("<%s>Received response - len<%d> / Resp <%s> \n",
          __FUNCTION__, respLen, MODEM_respBuff);
   
   if (strcmp(MODEM_respBuff, "OK") != 0)
   {
       printf("<%s> expected %s, received %s \n", 
              __FUNCTION__, "OK", MODEM_respBuff);
       sts = MODEM_STS_INVALID_RESPONSE;
   }

   sts = SIM808_checkForIPAddr(cntxt_p);
   if (sts == MODEM_STS_SUCCESS)
   {
       printf("<%s> IP address assigned .... \n", __FUNCTION__);
       // RAM-HACK
   }
   else
       printf("<%s> IP address not assigned .... \n", __FUNCTION__);

   return sts;
}


/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t SIM808_sendTCPData(MODEM_cntxt_s *cntxt_p,
                               unsigned char *msg_p,
                               unsigned int msgLen) 
{
   MODEM_sts_t sts;
   char cmdBuff[64];
   int respLen;

   // AT+CIPSEND
   sprintf(cmdBuff, "AT+CIPSEND\r\n");
   printf("<%s> Sending Cmd %s \n", __FUNCTION__,  cmdBuff);
   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>MODEM_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }

   printf("<%s> Waiting for > indicating modem is ready to receive payload .. \n",
          __FUNCTION__);

   sts = MODEM_waitForExactResp(cntxt_p, MODEM_respBuff, "\r\n>", 3, 5);
   if (sts != MODEM_STS_SUCCESS)
       return sts;
   
   printf("<%s>Received response - Resp <%s> \n",
          __FUNCTION__, MODEM_respBuff);

   printf("<%s> Sending Data String : %s \n", __FUNCTION__, msg_p);
   sts = MODEM_sendCmd(cntxt_p, msg_p);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>MODEM_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }

   cmdBuff[0] = 0x1a; // ctrl-z
   cmdBuff[1] = '\0';

   printf("<%s> Sending ctrl-z to initiate data transfer .. \n", __FUNCTION__);
   
   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>MODEM_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }

   sts = SIM808_waitForResp(cntxt_p, MODEM_respBuff, 
                            sizeof(MODEM_respBuff), 
                            &respLen, 10000);
   if (sts != MODEM_STS_SUCCESS)
       return sts;
   
   MODEM_respBuff[respLen] = '\0';

   printf("<%s>Received response - len<%d> / Resp <%s> \n",
          __FUNCTION__, respLen, MODEM_respBuff);

   if (strcmp(MODEM_respBuff, "CLOSED") == 0)
   {
       printf("<%s> TCP connection has been closed !! \n", __FUNCTION__);
       sts = MODEM_STS_TCP_CONN_DOWN;
   }
   else
   {
      if (strcmp(MODEM_respBuff, "SEND OK") != 0)
      {
          printf("<%s> expected %s, received %s \n", 
                 __FUNCTION__, "SEND OK", MODEM_respBuff);
          sts = MODEM_STS_INVALID_RESPONSE;
      }
   }

   return sts;
}


/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
void SIM808_closeTCPConn(MODEM_cntxt_s *cntxt_p)
{
   MODEM_sts_t sts;
   char cmdBuff[64];
   int respLen;

   /*
    * AT+CIPCLOSE: Close TCP connection
    */

   sprintf(cmdBuff, "AT+CIPCLOSE\r\n");
   printf("<%s> Sending Cmd %s \n", __FUNCTION__,  cmdBuff);
   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>MODEM_sendCmd() failed !! \n", __FUNCTION__);
       return;
   }

   sts = SIM808_waitForResp(cntxt_p, MODEM_respBuff,
                            sizeof(MODEM_respBuff),
                            &respLen, cntxt_p->connEstTmoSecs*1000);
   if (sts != MODEM_STS_SUCCESS)
       return;

   MODEM_respBuff[respLen] = '\0';

   printf("<%s>Received response - len<%d> / Resp <%s> \n",
          __FUNCTION__, respLen, MODEM_respBuff);

   if (strcmp(MODEM_respBuff, "CLOSE OK") != 0)
       sts = MODEM_STS_INVALID_RESPONSE;

   return;
}


/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t SIM808_openTCPConn(MODEM_cntxt_s *cntxt_p,
                               char *remoteIP_p,
                               int remotePort) 
{
   MODEM_sts_t sts;
   char cmdBuff[64];
   int respLen;
   static int newConn = 0;

   // AT+CIPSTART Start Up TCP or UDP Connection 
   
   sprintf(cmdBuff, "AT+CIPSTART=\"TCP\",\"%s\",\"%d\"\r\n",
           remoteIP_p, remotePort);
   printf("<%s> Sending Cmd %s \n", __FUNCTION__,  cmdBuff);
   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>MODEM_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }

   sts = MODEM_getResp(cntxt_p, MODEM_respBuff, 
                       sizeof(MODEM_respBuff), &respLen);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s> MODEM_getResp() failed !! \n", __FUNCTION__);
       return sts;
   }
    
   MODEM_respBuff[respLen] = '\0';
   
   printf("<%s>Received response - len<%d> / Resp <%s> \n",
          __FUNCTION__, respLen, MODEM_respBuff);
   
   if (strcmp(MODEM_respBuff, "OK") != 0)
   {
       printf("<%s> expected %s, received %s \n", 
              __FUNCTION__, "OK", MODEM_respBuff);
       sts = MODEM_STS_INVALID_RESPONSE;
   }

   sts = SIM808_waitForResp(cntxt_p, MODEM_respBuff, 
                            sizeof(MODEM_respBuff), 
                            &respLen, cntxt_p->connEstTmoSecs*1000);
   if (sts != MODEM_STS_SUCCESS)
       return sts;

   MODEM_respBuff[respLen] = '\0';

   printf("<%s>Received response - len<%d> / Resp <%s> \n",
          __FUNCTION__, respLen, MODEM_respBuff);
  
   // If TCP connection is aleady open, returns "ALREADY CONNECT".

   if (strcmp(MODEM_respBuff, "ALREADY CONNECT") == 0)
   {
       printf("<%s> TCP connection already open - newConn<%d> \n", __FUNCTION__, newConn);

       if (newConn == 0)
       {
           // Close TCP connection - must be old one !!
           SIM808_closeTCPConn(cntxt_p);
           sts = MODEM_STS_TCP_CONN_ALREADY_OPEN;
       }
   }
   else
   {
       if (strcmp(MODEM_respBuff, "CONNECT OK") != 0)
       {
           printf("<%s> expected %s, received %s \n", 
                  __FUNCTION__, "CONNECT OK", MODEM_respBuff);
           sts = MODEM_STS_INVALID_RESPONSE;
       }
       else
       {
           newConn = 1;
       }
   }

   return sts;
}


/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t SIM808_verifyMfrAndPartNr(MODEM_cntxt_s *cntxt_p)
{
   MODEM_sts_t sts;
   int respLen;
   char cmdBuff[64];

   printf("<%s> Entry .... \n", __FUNCTION__);

   sprintf(cmdBuff, "ATI3\r\n");
   printf("<%s> Sending Cmd %s \n", __FUNCTION__,  cmdBuff);
   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>MODEM_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }

   sts = SIM808_waitForResp(cntxt_p, MODEM_respBuff, 
                           sizeof(MODEM_respBuff), &respLen, 10000);
   if (sts != MODEM_STS_SUCCESS)
       return sts;

   MODEM_respBuff[respLen] = '\0';

   if (verbose)
       printf("<%s>Received response - len<%d> / Resp <%s> \n",
              __FUNCTION__, respLen, MODEM_respBuff);

   if (strstr(MODEM_respBuff, "SIMCOM") != NULL
       && strstr(MODEM_respBuff, "simcom") != NULL)
   {
       return MODEM_STS_INVALID_RESPONSE;
   }

   strncpy(cntxt_p->mfr, MODEM_respBuff, sizeof(cntxt_p->mfr));

   sts = MODEM_sendCmd(cntxt_p, "ATI4\r\n");
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>MODEM_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }

   sts = SIM808_waitForResp(cntxt_p, MODEM_respBuff, 
                           sizeof(MODEM_respBuff), &respLen, 10000);
   if (sts != MODEM_STS_SUCCESS)
       return sts;

   MODEM_respBuff[respLen] = '\0';

   if (verbose)
       printf("<%s>Received response - len<%d> / Resp <%s> \n",
              __FUNCTION__, respLen, MODEM_respBuff);

   if (strstr(MODEM_respBuff, "SIM808") != NULL
       && strstr(MODEM_respBuff, "sim808") != NULL)
   {
       sts = MODEM_STS_INVALID_RESPONSE;
   }

   strncpy(cntxt_p->partNr, MODEM_respBuff, sizeof(cntxt_p->partNr));

   return sts;
}


/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t SIM808_reqGSMRegDeregOpn(MODEM_cntxt_s *cntxt_p, 
                                     MODEM_gsmRegDeregOpn_t opn,                                  
                                     const char *nwkCode_p)
{
   MODEM_sts_t sts;
   int respLen, opSelMode = -1;
   char cmdBuff[20];

   /*
    * AT+COPS= [<mode> [,<format> [,<oper>]]]
    *
    * Set command forces an attempt to select and register the GSM network operator.
    * <mode> parameter defines whether the operator selection is done automatically or
    * it is forced by this command to operator <oper>.  The operator <oper> shall be 
    * given in format <format>.
    *
    * Parameters:
    * <mode>
    *   0 - automatic choice (the parameter <oper> will be ignored) (factory default)
    *   1 - manual choice (<oper> field shall be present)
    *   2 - deregister from GSM network; the MODULE is kept unregistered until a
    *       +COPS with <mode>=0, 1 or 4 is issued
    *   3 - set only <format> parameter (the parameter <oper> will be ignored)
    *   4 - manual/automatic (<oper> field shall be present); if manual selection fails,
    *       automatic mode (<mode>=0) is entered
    *
    * <format>
    *   0 - alphanumeric long form (max length 16 digits)
    *   2 - Numeric 5 or 6 digits [country code (3) + network code (2 or 3)]
    * 
    * <oper>: network operator in format defined by <format> parameter.
    * 
    * Note: <mode> parameter setting is stored in NVM and available at next reboot, if it
    *       is not 3 (i.e.: set only <format> parameter).
    *
    * Note: if <mode>=1 or 4, the selected network is stored in NVM too and is
    *       available at next reboot (this will happen even with a new SIM inserted)
    * 
    * Note: <format> parameter setting is never stored in NVM
    *
    *
    */

   if (opn == MODEM_REQ_GSM_REG_OPN)
   {
       switch (cntxt_p->operatorSelMode)
       {
          case MODEM_OPERATOR_SEL_MODE_MANUAL:
               opSelMode = 1;
               break; 

          case MODEM_OPERATOR_SEL_MODE_AUTO:
               opSelMode = 0;
               break; 

          case MODEM_OPERATOR_SEL_MODE_AUTO_ON_MANUAL_FLR:
               opSelMode = 4;
               break; 

          default:
               assert(80);
               break; 
       }
   }
   else
   {
       if (opn == MODEM_REQ_GSM_DEREG_OPN)
           opSelMode = 2;
       else
           assert(81);
   }

   if (opn == MODEM_REQ_GSM_REG_OPN)
       sprintf(cmdBuff, "AT+COPS=%d,2,\"%s\"\r\n", 
               opSelMode, nwkCode_p);
   else
       sprintf(cmdBuff, "AT+COPS=2\r\n");

   printf("<%s> Sending Cmd %s \n", __FUNCTION__,  cmdBuff);

   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>MODEM_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }

   /*
    * If MODEM is currently registered and it is requested to de-register,
    * it will simply return OK
    */

   sts = SIM808_waitForResp(cntxt_p, MODEM_respBuff, 
                            sizeof(MODEM_respBuff), &respLen, 10000);
   if (sts != MODEM_STS_SUCCESS)
       return sts;

   MODEM_respBuff[respLen] = '\0';

   if (1) // verbose)
       printf("<%s>Received response - len<%d> / Resp <%s> \n",
              __FUNCTION__, respLen, MODEM_respBuff);

   if (strcmp(MODEM_respBuff, "OK") != 0)
   {
       printf("<%s> expected %s, received %s \n", 
              __FUNCTION__, "OK", MODEM_respBuff);

       sts = MODEM_STS_INVALID_RESPONSE;
   }

   return sts;
}


/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t SIM808_getOperator(MODEM_cntxt_s *cntxt_p)
{
   MODEM_sts_t sts;
   int mode, stat, fieldCnt, respLen = 0;
   char cmdBuff[20];

   printf("<%s> Entry ... \n", __FUNCTION__);

   sprintf(cmdBuff, "AT+COPS?\r\n");
   printf("<%s> Sending Cmd %s \n", __FUNCTION__,  cmdBuff);
   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s> MODEM_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }
   
   printf("<%s> MODEM_sendCmd() done \n", __FUNCTION__);

   sts = MODEM_getResp(cntxt_p, MODEM_respBuff, 
                       sizeof(MODEM_respBuff), &respLen);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s> MODEM_getResp() failed !! \n", __FUNCTION__);
       return sts;
   }

   printf("<%s> MODEM_getResp() passed \n", __FUNCTION__);

   MODEM_respBuff[respLen] = '\0';

   printf("<%s>Received response - len<%d> / Resp <%s> \n",
          __FUNCTION__, respLen, MODEM_respBuff);
      
   strncpy(cntxt_p->operatorStr, MODEM_respBuff, sizeof(cntxt_p->operatorStr));

   return sts;
}


/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t SIM808_getGSMRegSts(MODEM_cntxt_s *cntxt_p)
{
   MODEM_sts_t sts;
   int mode, stat, fieldCnt, respLen = 0;
   char cmdBuff[20];

   printf("<%s> Entry ... \n", __FUNCTION__);

   sprintf(cmdBuff, "AT+CREG?\r\n");
   printf("<%s> Sending Cmd %s \n", __FUNCTION__,  cmdBuff);
   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s> MODEM_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }
   
   printf("<%s> MODEM_sendCmd() done \n", __FUNCTION__);

   sts = MODEM_getResp(cntxt_p, MODEM_respBuff, 
                       sizeof(MODEM_respBuff), &respLen);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s> MODEM_getResp() failed !! \n", __FUNCTION__);
       return sts;
   }

   printf("<%s> MODEM_getResp() passed \n", __FUNCTION__);

   MODEM_respBuff[respLen] = '\0';

   printf("<%s>Received response - len<%d> / Resp <%s> \n",
          __FUNCTION__, respLen, MODEM_respBuff);

   // UTIL_rmvSpaces(MODEM_respBuff);

   fieldCnt = sscanf(MODEM_respBuff, "+CREG: %d,%d", &mode, &stat);
   if (fieldCnt != 2)
       sts = MODEM_STS_RESPONSE_FMT_BAD;
   else
   {
       printf("<%s> mode<%d> / stat<%d> \n", __FUNCTION__, mode, stat);

       if (mode >= 0 && mode <= 2)
       {
           switch (stat)
           {
              case SIM808_CREG_STAT_UNKNOWN:
                   {
                       cntxt_p->gsmRegStatus= MODEM_GSM_REG_STS_UNKNOWN;
                   }
                   break;

              case SIM808_CREG_STAT_REG_DONE_HOME_NWK:
                   {
                       cntxt_p->gsmRegStatus = MODEM_GSM_REG_STS_DONE_HOME;
                   }
                   break;

              case SIM808_CREG_STAT_REG_DONE_ROAMING:
                   {
                       cntxt_p->gsmRegStatus = MODEM_GSM_REG_STS_DONE_ROAMING;
                   }
                   break;

              case SIM808_CREG_STAT_NOT_REGD_SEARCHING:
                   {
                       cntxt_p->gsmRegStatus = MODEM_GSM_REG_STS_NOT_DONE_SEARCHING;
                   }
                   break;

              case SIM808_CREG_STAT_NOT_REGD_NOT_SEARCHING:
                   {
                       cntxt_p->gsmRegStatus = MODEM_GSM_REG_STS_NOT_DONE_NOT_SEARCHING;
                   }
                   break;

              case SIM808_CREG_STAT_REGN_DENIED:
                   {
                       cntxt_p->gsmRegStatus = MODEM_GSM_REG_STS_REGN_DENIED;
                   }
                   break;

              default:
                   sts = MODEM_STS_INVALID_RESPONSE;
                   break;
           }
       }
       else      
           sts = MODEM_STS_RESPONSE_FMT_BAD;
   }

   return sts;
}


/*
 * ------------------------------------------------------------------------------
 * Code source: https://www.geodatasource.com/developers/c
 * ------------------------------------------------------------------------------
 */

const double piVal = 3.14159265358979323846;

/*
 ********************************************************************
 *
 * This function converts decimal degrees to radians 
 *
 *
 ********************************************************************
 */
double deg2rad(double deg)
{
  return (deg * piVal / 180);
}


/*
 ********************************************************************
 *
 * This function converts radians to decimal degrees 
 *
 *
 *
 ********************************************************************
 */
double rad2deg(double rad)
{
  return (rad * 180 / piVal);
}


/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
static double __getInterCoordDist(double lat1, double long1,
                                  double lat2, double long2)
{                         
  double theta, dist;

  theta = long1 - long2;
  dist = sin(deg2rad(lat1)) * sin(deg2rad(lat2)); 
  dist += (cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(theta)));
  dist = acos(dist);
  dist = rad2deg(dist);
  dist = dist * 60 * 1.1515;
  dist = dist * 1.609344;

  return (dist);
}

/*
 * ------------------------------------------------------------------------------
 */


/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
static double __convCoordtoDeg(int high, int low)
{
   int minVal = high;
   double outVal = low;

   minVal %= 100; // minutes part
   high /= 100;   // degrees part

   while (outVal >= 1)
          outVal /= 10;

   outVal += minVal;   // minutes part
   outVal /= 60;   // in degrees
   outVal += high;

   return outVal;
}


/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t SIM808_getGPSOpInfo(MODEM_cntxt_s *cntxt_p)
{
   MODEM_sts_t sts;
   int respLen = 0;
   char cmdBuff[20];

   printf("<%s> Entry ... \n", __FUNCTION__);
   
   sprintf(cmdBuff, "AT+CGPSINF=0\r\n");
   printf("<%s> Sending Cmd %s \n", __FUNCTION__,  cmdBuff);
   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s> MODEM_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }
   
   printf("<%s> MODEM_sendCmd() done \n", __FUNCTION__);

   sts = MODEM_getResp(cntxt_p, MODEM_respBuff, 
                       sizeof(MODEM_respBuff), &respLen);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s> MODEM_getResp() failed !! \n", __FUNCTION__);
       return sts;
   }

   printf("<%s> MODEM_getResp() passed \n", __FUNCTION__);

   MODEM_respBuff[respLen] = '\0';

   printf("<%s>Received response - len<%d> / Resp <%s> \n",
          __FUNCTION__, respLen, MODEM_respBuff);
  
   /*
    * Response:
    * <mode>,<long>,<lat>,<alt>,<UTC time>,<TTFF>,<num>,<speed>,<course>
    *
    * UTC Time format: yyyymmddHHMMSS
    *
    * Lat/Long:
    * eg. 4533.35 is 45 degrees and 33.35 minutes. 
    * ".35" of a minute is exactly 21 seconds.
    *
    * num:
    * Satellites in view for fix
    *
    * speed:
    * Speed over ground in knots
    */
  
   { 
      int fieldCnt;
      int mode, longH, longL, latH, latL, speed1, speed2, ttff, satCnt;
      int course1, course2, altH, altL;
      char timeBuff1[20], timeBuff2[8];
  
      fieldCnt = sscanf(MODEM_respBuff, "+CGPSINF: %d,%d.%d,%d.%d,%d.%d,%14s.%3s,%d,%d,%d.%d,%d.%d", 
                        &mode,
                        &latH, &latL,
                        &longH, &longL,
                        &altH, &altL,
                        timeBuff1, timeBuff2, &ttff, &satCnt,
                        &speed1, &speed2,
                        &course1, &course2);

      if (fieldCnt != 15)
          sts = MODEM_STS_INVALID_RESPONSE;
      else
      {
          /*
           * Lat/Long:
           * eg. 4533.35 is 45 degrees and 33.35 minutes. 
           * ".35" of a minute is exactly 21 seconds.
           */

          cntxt_p->latInDeg = __convCoordtoDeg(latH, latL);
          cntxt_p->longInDeg = __convCoordtoDeg(longH, longL);

          strcpy(cntxt_p->utcTimeStr1, timeBuff1);
          strcpy(cntxt_p->utcTimeStr2, timeBuff2);
      }
   }

   return sts;
}


/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t SIM808_doGPSPwrCntrlOpn(MODEM_cntxt_s *cntxt_p, char opn)
{
   MODEM_sts_t sts;
   int respLen = 0;
   char cmdBuff[20];

   if (opn != 0 && opn != 1)
       return MODEM_STS_INVALID_PARAM;

   printf("<%s> Entry ... \n", __FUNCTION__);

   sprintf(cmdBuff, "AT+CGPSPWR=%d\r\n", opn);
   printf("<%s> Sending Cmd %s \n", __FUNCTION__,  cmdBuff);
   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s> MODEM_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }
   
   printf("<%s> MODEM_sendCmd() done \n", __FUNCTION__);

   sts = MODEM_getResp(cntxt_p, MODEM_respBuff, 
                       sizeof(MODEM_respBuff), &respLen);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s> MODEM_getResp() failed !! \n", __FUNCTION__);
       return sts;
   }

   printf("<%s> MODEM_getResp() passed \n", __FUNCTION__);

   MODEM_respBuff[respLen] = '\0';

   printf("<%s>Received response - len<%d> / Resp <%s> \n",
          __FUNCTION__, respLen, MODEM_respBuff);

   if (strstr(MODEM_respBuff, "OK") == NULL)
   {
       printf("<%s> expected %s, received %s \n", 
              __FUNCTION__, "OK", MODEM_respBuff);
       sts = MODEM_STS_INVALID_RESPONSE;
   }
   else
       cntxt_p->gpsPwrOn = (opn ? 1 : 0);

   return sts;
}


/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t SIM808_getGPSPwrCntrlPinSts(MODEM_cntxt_s *cntxt_p)
{
   MODEM_sts_t sts;
   int respLen = 0, pwrState, fieldCnt;
   char cmdBuff[20];

   printf("<%s> Entry ... \n", __FUNCTION__);

   sprintf(cmdBuff, "AT+CGPSPWR?\r\n");
   printf("<%s> Sending Cmd %s \n", __FUNCTION__,  cmdBuff);
   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s> MODEM_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }
   
   printf("<%s> MODEM_sendCmd() done \n", __FUNCTION__);

   sts = MODEM_getResp(cntxt_p, MODEM_respBuff, 
                       sizeof(MODEM_respBuff), &respLen);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s> MODEM_getResp() failed !! \n", __FUNCTION__);
       return sts;
   }

   printf("<%s> MODEM_getResp() passed \n", __FUNCTION__);

   MODEM_respBuff[respLen] = '\0';

   printf("<%s>Received response - len<%d> / Resp <%s> \n",
          __FUNCTION__, respLen, MODEM_respBuff);

   fieldCnt = sscanf(MODEM_respBuff, "+CGPSPWR:%d", &pwrState);
   if ((fieldCnt != 1) || (pwrState < 0) || (pwrState > 1))
       sts = MODEM_STS_RESPONSE_FMT_BAD;
   else 
       cntxt_p->gpsPwrOn = pwrState; 

   return sts; 
}


/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t SIM808_getBattInfo(MODEM_cntxt_s *cntxt_p)
{
   MODEM_sts_t sts;
   int respLen = 0;
   char cmdBuff[20];

   printf("<%s> Entry ... \n", __FUNCTION__);

   sprintf(cmdBuff, "AT+CBC\r\n");
   printf("<%s> Sending Cmd %s \n", __FUNCTION__,  cmdBuff);
   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s> MODEM_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }
   
   printf("<%s> MODEM_sendCmd() done \n", __FUNCTION__);

   sts = MODEM_getResp(cntxt_p, MODEM_respBuff, 
                       sizeof(MODEM_respBuff), &respLen);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s> MODEM_getResp() failed !! \n", __FUNCTION__);
       return sts;
   }

   printf("<%s> MODEM_getResp() passed \n", __FUNCTION__);

   MODEM_respBuff[respLen] = '\0';

   // Example:
   // AT+CBC
   // +CBC: 0,100,4202
   //
   // AT+CBC
   // +CBC: 0,98,4186

   printf("<%s>Received response - len<%d> / Resp <%s> \n",
          __FUNCTION__, respLen, MODEM_respBuff);

   {
      int ign, level, battV;
      int fieldCnt = sscanf(MODEM_respBuff, "+CBC: %d,%d,%d", 
                            &ign, &level, &battV);
      if (fieldCnt != 3)
          sts = MODEM_STS_RESPONSE_FMT_BAD;
      else
      {
          cntxt_p->battChargeLevel = level;
          cntxt_p->battV = battV;
      }
   }

   return sts;
}

#endif
