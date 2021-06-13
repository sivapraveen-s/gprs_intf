#ifdef MODEM_PART_NR_GL868

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include <ip.h>
#include <modem.h>
#include <gl868.h>

extern int verbose;

const char *GL868_SUCCESS_RESP_STR = "OK";
const char *GL868_FLR_RESP_STR = "ERROR";

char GL868_sockSuspendEscSeq[ ] = "+++";

char *GL868_cmdList[ ]  = 
{
   "",    
   "AT",
   "ATE0",   /* 0 - Disable echo, 1 - Enable echo */
   "ATIx",   /* Identification Information */
   "AT&K=0",  /* Disable hw flow control */
   "AT+CSQ?",
   "AT+CREG?\r\n",
   "AT+COPS?\r\n",   // 7

   /*
    * AT+CGDCONT?
    * Read command returns the current settings for each defined context in the format:
    * +CGDCONT: <cid>,<PDP_type>,<APN>,<PDP_addr>,<d_comp>, <h_comp>
    */
   "AT+CGDCONT?",    // 8

   /*
    * AT+CGDCONT=x
    * a special form of the Set command, +CGDCONT=<cid>, causes the values for context 
    * number <cid> to become undefined.
    */
   "AT+CGDCONT=",    // 9
   
   "AT+CGDCONT=",    // 10

   "AT#CGPADDR=",    // 11
   "AT#SGACT=",      // 12
   "AT#SD=",         // 13

   /*
    * TA returns a list of quadruplets, each representing an operator present in
    * the network. Any of the formats may be unavailable and should then be an
    * empty field. The list of operators shall be in order: home network,
    * networks referenced in SIM, and other networks.
    */
   "AT+COPS=?\r\n"   // 14
};

char MODEM_respBuff[1024];


/*
 ********************************************************************
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t GL868_sendCmd(MODEM_cntxt_s *cntxt_p, int cmdId)
{
   printf("<%s> Sending Cmd %s \n", __FUNCTION__,  GL868_cmdList[cmdId]);
   return MODEM_sendCmd(cntxt_p, GL868_cmdList[cmdId]);
}
   

/*
 * Prior to establishing a GPRS session, a device / subscriber must 
 * first register with the * network. 
 *
 * GPRS is enabled by a network overlay on top of a standard GSM 
 * network. A modem registers separately with the GSM network  (i.e. 
 * registers with the MSC) and the GPRS network (i.e.  registers with 
 * the SGSN, also known as GPRS Attach). Although it is possible to 
 * register only on the GPRS network (i.e. perform GPRS Attach without 
 * GSM registration), most modems are configured to register for GSM 
 * and GPRS. Some operators require that a device perform both GSM 
 * registration and GPRS Attach.  Do Not set the device to GPRS-only 
 * service. In other words, DO NOT issue command AT+CGCLASS = CG or 
 * command AT+CGCLASS = CC. 
 */



/*
 * Network Registration - Checking Status
 * AT+CREG command checks GSM registration status. Without GSM registration, 
 * you should not attempt to attach to GPRS or perform any dialing function. 
 * By default, a modem will attempt to automatically register with a network 
 * without your having to invoke any action.  
 * 
 * Command: AT+CREG?
 * 
 * Read command reports the <mode> and <stat> parameter values in the format:
 * 
 * +CREG: <mode>,<stat>[,<Lac>,<Ci>]
 * 
 * Note: <Lac> and <Ci> are reported only if <mode>=2 and the mobile is 
 *       registered on some network cell.
 *
 * <mode> can be:
 *  0 - disable network registration unsolicited result code (factory default)
 *  1 - enable network registration unsolicited result code
 *  2 - enable network registration unsolicited result code with network Cell
 *     identification data
 *
 * <stat> can be: 
 *  0 - not registered, ME is not currently searching a new operator to reg to
 *  1 - Registered, home network
 *  2 - not registered, but ME is currently searching a new operator to reg to
 *  3 - Registration denied
 *  4 - Unknown
 *  5 - Registered, non-home network
 *
 * Prior to performing GPRS attach, it is important to ensure that GSM registration 
 * has been successful. You can check this in the following ways:
 * Command: AT+CREG?    (returns the current GSM registration status)
 * Command: AT+CREG = 1 (enables network registration unsolicited result code - 
 *                       provides registration status only)
 * Command: AT+CREG = 2 (enables network registration unsolicited result code - 
 *                       provides registration status only and service location area 
 *                       and cell)
 * Note that AT+CREG=1 or AT+CREG=2, if selected, should be used at the start of the 
 * modem manager script to enable the +CREG unsolicited response code. 
 */




/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t GL868_deactivatePDPCntxt(MODEM_cntxt_s *cntxt_p)
{
   MODEM_sts_t sts;
   int respLen;
   char cmdBuff[20];

   /*
    * The command syntax is:
    * #SGACT= <Cntx Id>,<Status>, [<Username>],[<Password>]
    * Where:
    * 1 Cntx Id is the context that we want to activate/deactivate.
    * 2 Status is the context status (0 means deactivation, 1 activation).
    */

   sprintf(cmdBuff, "AT#SGACT=%d,%d\r\n", GL868_PDP_CNTXT_ACTIVE_CNTXT_ID, 0);
   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>GL868_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }

   printf("\n sleeping for 5 seconds ... \n");
   sleep(5);

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

   if (strcmp(MODEM_respBuff, GL868_SUCCESS_RESP_STR) != 0)
   {
       printf("<%s> expected %s, received %s \n", 
              __FUNCTION__, GL868_SUCCESS_RESP_STR, MODEM_respBuff);
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
MODEM_sts_t GL868_activatePDPCntxt(MODEM_cntxt_s *cntxt_p)
{
   MODEM_sts_t sts;
   int fieldCnt, ipv4[IP_V4_ADDR_LEN], respLen;
   char cmdBuff[20];

   /*
    * 3.2.1.3. Request the context to be activated
    * This command allows activation of one of the contexts defined with AT command
    * +CGDCONT. With multisocket it is possible to activate simultaneously two contexts of
    * the five that have been set. We can write username and password directly from command
    * line (if required). At least one Connection Id must be associated to the context we want
    * to activate; otherwise an error will be appear.
    * The command syntax is:
    * #SGACT= <Cntx Id>,<Status>, [<Username>],[<Password>]
    * Where:
    * 1 Cntx Id is the context that we want to activate/deactivate.
    * 2 Status is the context status (0 means deactivation, 1 activation).
    */

   sprintf(cmdBuff, "AT#SGACT=%d,%d\r\n", GL868_PDP_CNTXT_ACTIVE_CNTXT_ID, 1);
   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>GL868_sendCmd() failed !! \n", __FUNCTION__);
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

   fieldCnt = sscanf(MODEM_respBuff, "#SGACT: %d.%d.%d.%d", 
                     &ipv4[0], &ipv4[1], &ipv4[2], &ipv4[3]);
   
   if (fieldCnt == IP_V4_ADDR_LEN)
   {
       int idx;
       for (idx=0; idx<IP_V4_ADDR_LEN; idx++)
            cntxt_p->ipv4Addr[idx] = ipv4[idx];
   }
   else
       sts = MODEM_STS_INVALID_RESPONSE;
   
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
MODEM_sts_t GL868_cfgTCPStack(MODEM_cntxt_s *cntxt_p)
{
   MODEM_sts_t sts;
   char cmdBuff[64];
   int respLen;

   /*
    * Configuring the embedded TCP/IP stack
    * The TCP/IP stack behavior must be configured by setting:
    * 1 the packet default size
    * 2 the data sending timeout
    * 3 the socket inactivity timeout 
    * AT#SCFG =  <Conn Id>, <Cntx Id>, <Pkt sz>, <Global To>, <Conn To>, <Tx To>
    * Where:
    * 1 Conn Id -the connection identifier
    * 2 Cntx Id -the context identifier
    * 3 Pkt sz - packet size to be used by the TCP/UDP/IP stack for data sending.
    *            0 - select automatically; default value (300).
    *            1..1500 - packet size in bytes
    * 4 Global To - exchange timeout (or socket inactivity timeout); if there’s no data
    *               exchange within this timeout period the connection is closed.
    *               0 - no timeout
    *               1..65535 - timeout value in seconds (default 90 s.)
    * 5 Conn To - connection timeout (default 60 sec, expressed in tenths of second)
    *             if we can’t establish a connection to the remote within this timeout period, 
    *             an error is raised.  
    *             10..1200 - timeout value in hundreds of milliseconds (default 600)
    * 6 Tx To - data sending timeout; after this period data are sent also if they’re less
    *           than max packet size.
    *           0 - no timeout
    *           1..255 - timeout value in hundreds of milliseconds (default 50)
    *           256 - set timeout value in 10 milliseconds
    *           257 - set timeout value in 20 milliseconds
    *           258 - set timeout value in 30 milliseconds
    *           259 - set timeout value in 40 milliseconds
    *           260 - set timeout value in 50 milliseconds
    *           261 - set timeout value in 60 milliseconds
    *           262 - set timeout value in 70 milliseconds
    *           263 - set timeout value in 80 milliseconds
    *           264 - set timeout value in 90 milliseconds
    *           Note: these values are automatically saved in NVM.
    *
    *           Note: if DNS resolution is required, max DNS resolution time (20 sec) has to be considered 
    *           in addition to <connTo>
    *
    * Example:
    * AT#SCFG = 4, 1, 512, 30, 300, 100
    */

   sprintf(cmdBuff, "AT#SCFG=%d,%d,%d,%d,%d,%d\r\n",
           GL868_ACTIVE_SOCKET_CONN_ID, 
           GL868_PDP_CNTXT_ACTIVE_CNTXT_ID,
           512, cntxt_p->dataExchangeTmoSecs, 
           cntxt_p->connEstTmoSecs*10, 
           cntxt_p->dataTxTimeoutSecs*10);    // Tx To set to 10*0.1 = 1 second  
                                               // (since we are not doing DNS resolution when  sending to AWS)

   printf("<%s> Sending Cmd %s \n", __FUNCTION__,  cmdBuff);
   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>GL868_sendCmd() failed !! \n", __FUNCTION__);
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
        
   if (strcmp(MODEM_respBuff, GL868_SUCCESS_RESP_STR) != 0)
   {
       printf("<%s> expected %s, received %s \n", 
              __FUNCTION__, GL868_SUCCESS_RESP_STR, MODEM_respBuff);
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
MODEM_sts_t GL868_createPDPCntxt(MODEM_cntxt_s *cntxt_p, const char *apn_p)
{
   MODEM_sts_t sts;
   char cmdBuff[32];
   int respLen;

   // This code will always create one PDP context with CID 1 for IP
   // traffic with specified APN

   sprintf(cmdBuff, "AT+CGDCONT=%d,\"IP\",\"%s\"\r\n",
           GL868_PDP_CNTXT_ACTIVE_CNTXT_ID, apn_p);

   printf("<%s> Sending Cmd %s \n", __FUNCTION__,  cmdBuff);
   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>GL868_sendCmd() failed !! \n", __FUNCTION__);
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
        
   if (strcmp(MODEM_respBuff, GL868_SUCCESS_RESP_STR) != 0)
   {
       printf("<%s> expected %s, received %s \n", 
              __FUNCTION__, GL868_SUCCESS_RESP_STR, MODEM_respBuff);
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
MODEM_sts_t GL868_delAllPDPCntxt(MODEM_cntxt_s *cntxt_p)
{
   MODEM_sts_t sts;
   int cid;

   for (cid = GL868_PDP_CNTXT_ID_MIN_VAL;
        cid <= GL868_PDP_CNTXT_ID_MAX_VAL;
        cid ++)
   {
        char cmdBuff[20];
        int respLen;

        sprintf(cmdBuff, "%s%d\r\n", 
                GL868_cmdList[GL868_DEL_PDP_CNTXT_CMD_ID], cid);

        printf("<%s> Sending Cmd %s \n", __FUNCTION__,  cmdBuff);
        sts = MODEM_sendCmd(cntxt_p, cmdBuff);
        if (sts != MODEM_STS_SUCCESS)
        {
            printf("<%s>GL868_sendCmd() failed !! \n", __FUNCTION__);
            break;
        }
   
        printf("<%s>GL868_sendCmd() done \n", __FUNCTION__);

        sts = MODEM_getResp(cntxt_p, MODEM_respBuff, 
                            sizeof(MODEM_respBuff), &respLen);
        if (sts != MODEM_STS_SUCCESS)
        {
            printf("<%s> MODEM_getResp() failed !! \n", __FUNCTION__);
            break;
        }
    
        MODEM_respBuff[respLen] = '\0';

        printf("<%s>Received response - len<%d> / Resp <%s> \n",
               __FUNCTION__, respLen, MODEM_respBuff);

        if (strcmp(MODEM_respBuff, GL868_SUCCESS_RESP_STR) != 0)
        {
            printf("<%s> expected %s, received %s \n", 
                   __FUNCTION__, GL868_SUCCESS_RESP_STR, MODEM_respBuff);
            sts = MODEM_STS_INVALID_RESPONSE;
            break;
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
MODEM_sts_t GL868_getOperator(MODEM_cntxt_s *cntxt_p)
{
   MODEM_sts_t sts;
   int mode, stat, fieldCnt, respLen = 0;

   printf("<%s> Entry ... \n", __FUNCTION__);

   sts = GL868_sendCmd(cntxt_p, GL868_GET_NWK_OPERATOR_CMD_ID);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s> GL868_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }
   
   printf("<%s> GL868_sendCmd() done \n", __FUNCTION__);

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
MODEM_sts_t GL868_getNwkOpsInVicinity(MODEM_cntxt_s *cntxt_p)
{
   MODEM_sts_t sts;
   int respLen;

   printf("<%s> Entry ... \n", __FUNCTION__);

   sts = GL868_sendCmd(cntxt_p, GL868_GET_NWK_OPS_IN_VICINITY);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s> GL868_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }
   
   printf("<%s> GL868_sendCmd() done \n", __FUNCTION__);

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
MODEM_sts_t GL868_getGSMRegSts(MODEM_cntxt_s *cntxt_p)
{
   MODEM_sts_t sts;
   int mode, stat, fieldCnt, respLen = 0;

   printf("<%s> Entry ... \n", __FUNCTION__);

   sts = GL868_sendCmd(cntxt_p, GL868_GET_GSM_REG_STS_CMD_ID);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s> GL868_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }
   
   printf("<%s> GL868_sendCmd() done \n", __FUNCTION__);

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
              case GL868_CREG_STAT_UNKNOWN:
                   {
                       cntxt_p->gsmRegStatus= MODEM_GSM_REG_STS_UNKNOWN;
                   }
                   break;

              case GL868_CREG_STAT_REG_DONE_HOME_NWK:
                   {
                       cntxt_p->gsmRegStatus = MODEM_GSM_REG_STS_DONE_HOME;
                   }
                   break;

              case GL868_CREG_STAT_REG_DONE_ROAMING:
                   {
                       cntxt_p->gsmRegStatus = MODEM_GSM_REG_STS_DONE_ROAMING;
                   }
                   break;

              case GL868_CREG_STAT_NOT_REGD_SEARCHING:
                   {
                       cntxt_p->gsmRegStatus = MODEM_GSM_REG_STS_NOT_DONE_SEARCHING;
                   }
                   break;

              case GL868_CREG_STAT_NOT_REGD_NOT_SEARCHING:
                   {
                       cntxt_p->gsmRegStatus = MODEM_GSM_REG_STS_NOT_DONE_NOT_SEARCHING;
                   }
                   break;

              case GL868_CREG_STAT_REGN_DENIED:
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
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t GL868_waitForResp(MODEM_cntxt_s *cntxt_p, char *respBuff_p, 
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
MODEM_sts_t GL868_suspendTCPConn(MODEM_cntxt_s *cntxt_p)
{
   MODEM_sts_t sts;
   int respLen;

   /*
    * After the CONNECT we can suspend the direct interface to the
    * socket connection (nb the socket stays open) using the escape 
    * sequence (+++): the module moves back to command mode and we 
    * receive the final result code OK after the suspension.
    */

   printf("<%s> Sending Cmd %s \n", __FUNCTION__,  GL868_sockSuspendEscSeq);

   sts = MODEM_sendCmd(cntxt_p, GL868_sockSuspendEscSeq);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>GL868_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }

   sts = GL868_waitForResp(cntxt_p, MODEM_respBuff, 
                           sizeof(MODEM_respBuff), &respLen, 0);
   if (sts != MODEM_STS_SUCCESS)
       return sts;

   MODEM_respBuff[respLen] = '\0';

   printf("<%s>Received response - len<%d> / Resp <%s> \n",
          __FUNCTION__, respLen, MODEM_respBuff);
   
   if (strcmp(MODEM_respBuff, GL868_SUCCESS_RESP_STR) != 0)
   {
       printf("<%s> expected %s, received %s \n", 
              __FUNCTION__, GL868_SUCCESS_RESP_STR, MODEM_respBuff);
       sts = MODEM_STS_INVALID_RESPONSE;
   }

   return sts;
}
   

extern int __verbose;



/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t GL868_sendTCPData(MODEM_cntxt_s *cntxt_p,
                              unsigned char *msg_p,
                              unsigned int msgLen) 
{
   MODEM_sts_t sts;
   char cmdBuff[64];
   int respLen;

   /*
    * AT#SSENDEXT= <connId>, <bytestosend>
    *
    * Execution command permits, while the module is in command mode, to send
    * data through a connected socket including all possible octets (from 0x00 
    * to 0xFF).  Parameters:
    *   <connId> - socket connection identifier 1..6
    *   <bytestosend > - number of bytes to be sent
    * 
    * Please refer to test command for range
    * 
    * The device responds to the command with the prompt <greater_than><space> 
    * and waits for the data to send.  When <bytestosend> bytes have been sent, 
    * operation is automatically completed.
    * If data are successfully sent, then the response is OK. If data sending fails 
    * for some reason, an error code is reported.  
    *
    * AT#SSENDEXT=1,20
    * > .............................. ; // Terminal echo of bytes sent is displayed here
    * OK
    */

   sprintf(cmdBuff, "AT#SSENDEXT=%d,%d\r\n", GL868_ACTIVE_SOCKET_CONN_ID, msgLen);
   
   printf("<%s> Sending Cmd %s \n", __FUNCTION__,  cmdBuff);
   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>GL868_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }
  
   sts = MODEM_waitForExactResp(cntxt_p, MODEM_respBuff, "\r\n>", 3, 5);
   if (sts != MODEM_STS_SUCCESS)
       return sts;

   printf("<%s> Sending Data String : %s \n", __FUNCTION__, msg_p);
   sts = MODEM_sendCmd(cntxt_p, msg_p);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>GL868_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }

   sts = GL868_waitForResp(cntxt_p, MODEM_respBuff, 
                           sizeof(MODEM_respBuff), 
                           &respLen, 10000);
   if (sts != MODEM_STS_SUCCESS)
       return sts;

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
void GL868_closeTCPConn(MODEM_cntxt_s *cntxt_p)
{
   MODEM_sts_t sts;
   char cmdBuff[64];
   int respLen;

   /*
    * This command is used to close a socket.
    * Parameter:
    *   <connId> - socket connection identifier 
    *    1..6
    */

   sprintf(cmdBuff, "AT#SH=%d\r\n", GL868_ACTIVE_SOCKET_CONN_ID);
   printf("<%s> Sending Cmd %s \n", __FUNCTION__,  cmdBuff);
   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>GL868_sendCmd() failed !! \n", __FUNCTION__);
       return; 
   }

   sts = GL868_waitForResp(cntxt_p, MODEM_respBuff, 
                           sizeof(MODEM_respBuff), 
                           &respLen, cntxt_p->connEstTmoSecs*1000);
   if (sts != MODEM_STS_SUCCESS)
       return;

   MODEM_respBuff[respLen] = '\0';

   printf("<%s>Received response - len<%d> / Resp <%s> \n",
          __FUNCTION__, respLen, MODEM_respBuff);

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
MODEM_sts_t GL868_openTCPConn(MODEM_cntxt_s *cntxt_p,
                              char *remoteIP_p,
                              int remotePort) 
{
   MODEM_sts_t sts;
   char cmdBuff[64];
   int respLen;

   /*
    * AT#SD=<connId>, <txProt>, <rPort>, <IPaddr> [,<closureType> [,<lPort> [,<connMode>]]]
    * Execution command opens a remote connection via socket.
    * Parameters:
    * <connId> - socket connection identifier 1..6
    * <txProt> - transmission protocol
    * 0 - TCP
    * 1 - UDP
    * <rPort> - remote host port to contact 1..65535
    * <IPaddr> - address of the remote host, string type. 
    *            This parameter can be either:
    *            - any valid IP address in the format: xxx.xxx.xxx.xxx
    *            - any host name to be solved with a DNS query
    * <closureType> - socket closure behaviour for TCP when remote host has closed
    *                 0 - local host closes immediately (default)
    *               255 - local host closes after an AT#SH or immediately in case of 
    *                     an abortive disconnect from remote.
    * <lPort> - UDP connections local port 1..65535
    * <connMode> - Connection mode
    *              0 - online mode connection (default)
    *              1 - command mode connection
    * Note: <closureType> parameter is valid for TCP connections only and has no
    *       effect (if used) for UDP connections.  
    * Note: <lPort> parameter is valid for UDP connections only and has no effect 
    *       (if used) for TCP connections.
    */

   // This code will always create one PDP context with CID 1 for IP
   // traffic with specified APN

#ifdef GPRS_TCP_CONN_ONLINE_MODE
   sprintf(cmdBuff, "AT#SD=%d,%d,%d,\"%s\"\r\n",
           GL868_ACTIVE_SOCKET_CONN_ID, 0, remotePort, remoteIP_p);
#else
   sprintf(cmdBuff, "AT#SD=%d,%d,%d,\"%s\",0,0,1\r\n",
           GL868_ACTIVE_SOCKET_CONN_ID, 0, remotePort, remoteIP_p);
#endif

   printf("<%s> Sending Cmd %s \n", __FUNCTION__,  cmdBuff);
   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>GL868_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }

   sts = GL868_waitForResp(cntxt_p, MODEM_respBuff, 
                           sizeof(MODEM_respBuff), 
                           &respLen, cntxt_p->connEstTmoSecs*1000);
   if (sts != MODEM_STS_SUCCESS)
       return sts;

   MODEM_respBuff[respLen] = '\0';

   printf("<%s>Received response - len<%d> / Resp <%s> \n",
          __FUNCTION__, respLen, MODEM_respBuff);

#ifdef GPRS_TCP_CONN_ONLINE_MODE
   /*
    * if we set <connMode> to online mode connection and the command is
    * successful we enter in online data mode and we see the intermediate 
    * result code CONNECT.
    */
   if (strcmp(MODEM_respBuff, "CONNECT") != 0)
   {
       printf("<%s> expected %s, received %s \n", 
              __FUNCTION__, "CONNECT", MODEM_respBuff);
       sts = MODEM_STS_INVALID_RESPONSE;
       cntxt_p->tcpSockState = MODEM_TCP_SOCK_STATE_UNKNOWN;
   }
#else
   /*
    * if we set <connMode> to command mode connection and the command is
    * successful, the socket is opened and we remain in command mode and 
    * we see the result code OK.
    */
   if (strcmp(MODEM_respBuff, "OK") != 0)
   {
       printf("<%s> expected %s, received %s \n", 
              __FUNCTION__, "OK", MODEM_respBuff);
       sts = MODEM_STS_INVALID_RESPONSE;
       cntxt_p->tcpSockState = MODEM_TCP_SOCK_STATE_UNKNOWN;
   }
#endif
   else
       cntxt_p->tcpSockState = MODEM_TCP_SOCK_STATE_OPEN;

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
int GL868_checkForIPAddr(MODEM_cntxt_s *cntxt_p)
{
   MODEM_sts_t sts;
   char cmdBuff[64];
   int respLen;
   
   sprintf(cmdBuff, "AT#CGPADDR=%d\r\n", GL868_PDP_CNTXT_ACTIVE_CNTXT_ID, 1);
   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>GL868_sendCmd() failed !! \n", __FUNCTION__);
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

   {
      int fieldCnt, cntxtId, ipv4[4];

      fieldCnt = sscanf(MODEM_respBuff, "#CGPADDR: %d,\"%d.%d.%d.%d\"", 
                        &cntxtId, 
                        &(ipv4[0]), &(ipv4[1]), &(ipv4[2]), &(ipv4[3]));
      if (fieldCnt == 5)
      {
          if (cntxtId == GL868_PDP_CNTXT_ACTIVE_CNTXT_ID)
          {
              printf("<%s> Assigned IP address is <%d.%d.%d.%d> \n",
                     __FUNCTION__, ipv4[0], ipv4[1], ipv4[2], ipv4[3]);
              sts = MODEM_STS_SUCCESS;
          }
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
MODEM_sts_t GL868_getIPAddr(MODEM_cntxt_s *cntxt_p, const char *apn_p)
{
   MODEM_sts_t sts;

   // First check if IP address is already assigned
   sts = GL868_checkForIPAddr(cntxt_p);
   if (sts == MODEM_STS_SUCCESS)
   {
       printf("<%s> IP address already assigned .... \n", __FUNCTION__);
       // RAM-HACK
       return sts;
   }
   else
       printf("<%s> IP address not assigned .... \n", __FUNCTION__);
  
   // Creating PDP context if already created wont return an error 
   sts = GL868_createPDPCntxt(cntxt_p, apn_p);
   if (sts != MODEM_STS_SUCCESS)
       return sts;

   printf("<%s> Created PDP Context ... \n", __FUNCTION__);

   sts = GL868_cfgTCPStack(cntxt_p);
   if (sts != MODEM_STS_SUCCESS)
       return sts;
   
   printf("<%s> Configured TCP/IP Stack ... \n", __FUNCTION__);
  
   // If PDP cntxt is already active, reactivating will return ERROR 
   sts = GL868_deactivatePDPCntxt(cntxt_p);
   if (sts != MODEM_STS_SUCCESS)
       return sts;
   
   printf("<%s> De-Activated PDP Context ... \n", __FUNCTION__);

   printf("<%s> Activating PDP Context ... \n", __FUNCTION__);
   sts = GL868_activatePDPCntxt(cntxt_p);
   if (sts != MODEM_STS_SUCCESS)
       return sts;
   
   printf("<%s> Activated PDP Context ... \n", __FUNCTION__);

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
MODEM_sts_t GL868_disableEcho(MODEM_cntxt_s *cntxt_p)
{
   MODEM_sts_t sts;
   int respLen;

   sts = MODEM_sendCmd(cntxt_p, "ATE0\r\n");
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>GL868_sendCmd() failed !! \n", __FUNCTION__);
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
MODEM_sts_t GL868_checkSIMPresence(MODEM_cntxt_s *cntxt_p)
{
   MODEM_sts_t sts;
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

   sts = MODEM_sendCmd(cntxt_p, "AT+CPIN?\r\n");
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>GL868_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }

   sts = GL868_waitForResp(cntxt_p, MODEM_respBuff, 
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
MODEM_sts_t GL868_cfgOperatorSelMode(MODEM_cntxt_s *cntxt_p, 
                                     MODEM_operatorSelMode_t opSelMode)
{
   MODEM_sts_t sts;
   int respLen, mode = -1;
   char cmdBuff[16];

   switch (opSelMode)
   {
      case MODEM_OPERATOR_SEL_MODE_MANUAL:
           mode = 1;
           break;

      case MODEM_OPERATOR_SEL_MODE_AUTO:
           mode = 0;
           break;

      case MODEM_OPERATOR_SEL_MODE_AUTO_ON_MANUAL_FLR:
           mode = 4;
           break;

      default:
           break;
   }

   if (mode == -1)
       return MODEM_STS_INVALID_PARAM;

   sprintf(cmdBuff, "AT+COPS=%d\r\n", mode);
           
   printf("<%s> Sending Cmd %s \n", __FUNCTION__, cmdBuff);

   sts = MODEM_sendCmd(cntxt_p, cmdBuff);
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>GL868_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }

   sts = GL868_waitForResp(cntxt_p, MODEM_respBuff, 
                           sizeof(MODEM_respBuff), &respLen, 10000);
   if (sts != MODEM_STS_SUCCESS)
       return sts;

   MODEM_respBuff[respLen] = '\0';

   printf("<%s>Received response - len<%d> / Resp <%s> \n",
          __FUNCTION__, respLen, MODEM_respBuff);
   
   if (strcmp(MODEM_respBuff, GL868_SUCCESS_RESP_STR) != 0)
   {
       printf("<%s> expected %s, received %s \n", 
              __FUNCTION__, GL868_SUCCESS_RESP_STR, MODEM_respBuff);
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
MODEM_sts_t GL868_reqGSMRegDeregOpn(MODEM_cntxt_s *cntxt_p, 
                                    MODEM_gsmRegDeregOpn_t opn,                                  
                                    const char *nwkCode_p)
{
   MODEM_sts_t sts;
   int respLen, opSelMode = -1;
   char cmdBuff[16];

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
    * Looks like Telit GL868-Dual only supports format 2.
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
       printf("<%s>GL868_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }

   /*
    * If MODEM is currently registered and it is requested to de-register,
    * it will simply return OK
    */

   sts = GL868_waitForResp(cntxt_p, MODEM_respBuff, 
                           sizeof(MODEM_respBuff), &respLen, 10000);
   if (sts != MODEM_STS_SUCCESS)
       return sts;

   MODEM_respBuff[respLen] = '\0';

   if (1) // verbose)
       printf("<%s>Received response - len<%d> / Resp <%s> \n",
              __FUNCTION__, respLen, MODEM_respBuff);

   if (strcmp(MODEM_respBuff, GL868_SUCCESS_RESP_STR) != 0)
   {
       printf("<%s> expected %s, received %s \n", 
              __FUNCTION__, GL868_SUCCESS_RESP_STR, MODEM_respBuff);

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
MODEM_sts_t GL868_verifyMfrAndPartNr(MODEM_cntxt_s *cntxt_p)
{
   MODEM_sts_t sts;
   int respLen;

   printf("<%s> Entry .... \n", __FUNCTION__);

   sts = MODEM_sendCmd(cntxt_p, "ATI3\r\n");
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>GL868_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }

   sts = GL868_waitForResp(cntxt_p, MODEM_respBuff, 
                           sizeof(MODEM_respBuff), &respLen, 10000);
   if (sts != MODEM_STS_SUCCESS)
       return sts;

   MODEM_respBuff[respLen] = '\0';

   if (verbose)
       printf("<%s>Received response - len<%d> / Resp <%s> \n",
              __FUNCTION__, respLen, MODEM_respBuff);

   if (strstr(MODEM_respBuff, "Telit") != NULL
       && strstr(MODEM_respBuff, "telit") != NULL
       && strstr(MODEM_respBuff, "TELIT") != NULL)
   {
       return MODEM_STS_INVALID_RESPONSE;
   }

   strncpy(cntxt_p->mfr, MODEM_respBuff, sizeof(cntxt_p->mfr));

   sts = MODEM_sendCmd(cntxt_p, "ATI4\r\n");
   if (sts != MODEM_STS_SUCCESS)
   {
       printf("<%s>GL868_sendCmd() failed !! \n", __FUNCTION__);
       return sts;
   }

   sts = GL868_waitForResp(cntxt_p, MODEM_respBuff, 
                           sizeof(MODEM_respBuff), &respLen, 10000);
   if (sts != MODEM_STS_SUCCESS)
       return sts;

   MODEM_respBuff[respLen] = '\0';

   if (verbose)
       printf("<%s>Received response - len<%d> / Resp <%s> \n",
              __FUNCTION__, respLen, MODEM_respBuff);

   if (strstr(MODEM_respBuff, "GL868") != NULL
       && strstr(MODEM_respBuff, "gl868") != NULL)
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
unsigned char *GL868_removeSockSuspendEscSeq(unsigned char *msg_p, int *msgLen_p)
{
   int idx, off = 0;
   char  *newMsg_p;

   /*
    * After the CONNECT we can suspend the direct interface to the
    * socket connection (nb the socket stays open) using the escape 
    * sequence (+++): the module moves back to command mode and we 
    * receive the final result code OK after the suspension.
    *
    *
    * Since we are sending binary data over the connection, we need
    * to make sure that the stream does not contain the sequence 
    * "+++". Replace every + by /+ and replace every / by //.
    */

   newMsg_p = (unsigned char *)malloc(2*(*msgLen_p));
   if (newMsg_p == NULL)
   {
       printf("<%s> malloc(%d) failed !! \n", __FUNCTION__, 2*(*msgLen_p));
       return NULL;
   }

   for (idx=0; idx<*msgLen_p; idx++)
   {
        if (msg_p[idx] == '\\')
        {
            newMsg_p[off ++] = '\\';            
        }
        else
        {
            if (msg_p[idx] == '+')
            {
                newMsg_p[off ++] = '\\';            
            }
        }
        newMsg_p[off ++] = msg_p[idx];     
   }

   *msgLen_p = off;

   free(msg_p);

   return newMsg_p;
}
#endif
