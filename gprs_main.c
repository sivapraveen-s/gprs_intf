#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <pthread.h>
#include <wsuart.h>
#include <modem.h>
#if defined(MODEM_PART_NR_GL868)
#include <gl868.h>
#elif defined(MODEM_PART_NR_SIM808)
#include <sim808.h>
#else
#error Modem Not Specified/Supported !!
#endif
#include <modem_api.h>

int verbose = 0;

const int MODEM_baudRate = 
#if defined(MODEM_PART_NR_GL868) || defined(MODEM_PART_NR_SIM808)
   B38400
#elif defined(MODEM_PART_NR_SIM868)
   B115200
#else
#error Modem Not Specified/Supported !!
#endif
   ;

char SERVER_ipAddr[16] = "";
int SERVER_tcpPort= -1;

MODEM_cntxt_s  MODEM_cntxt;

int GW_cidPortNr = 45000;
int GW_cidSockFd = -1;

int GSM_operatorIdx = -1;

const GSM_operatorInfo_s GSM_operatorList[ ] =
{
   {"vodafone in", "40486", "www"},
   {"hutch", "40486", "www"},
   {"airtel", "40445", "airtelgprs.com"},
   {"bsnl mobile", "40471", "bsnlnet"},
   {"cellOne", "40471", "bsnlnet"},
   {"idea", "40444", ""},
   {"Bharat Karnataka", "40471", "bsnlnet"},
   {"t-mobile", "310260", "fast.t-mobile.com"},
   {"", "", ""},
};

const char *OPER_list[ ] =
{
 "vodaphone in",    // Vodaphone IN
 "Airtel",          // AirTel
 "hutch",           // HUTCH
 "bsnl mobile",     // BSNL MOBILE
 "spice",           // SPICE
 "Ting"             // Ting USA
};

#define MODEM_RX_BUFF_LEN  2048

char MODEM_rxBuff[MODEM_RX_BUFF_LEN];

/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
void print_usage()
{
    printf("Usage: ./gprs.exe <serial-device> <Operator_name_in_lower_case> <Cloud-IP> <Cloud-TCP-Port> <-d | -f> \n");
    printf("On Cywgin, the serial port device is named /dev/ttySX if the COM port is COMY where X = Y - 1 \n");
    printf("Example: ./gprs.exe /dev/ttyS20 vodafone 52.220.99.9 30000 -f\n");
}


#ifdef __CYGWIN__
    

/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
int UART_cfgPort(UART_cntxt_s *uartCntxt_p, 
                 const char *serDevName_p, 
                 const int baudRate)
{
    struct termios newtio;
    struct termios oldtio;
    struct termios latesttio;
    int rc;

    uartCntxt_p->serialFd = open(serDevName_p, O_RDWR | O_NOCTTY );
    if (uartCntxt_p->serialFd < 0)
    {
        printf("Failed to open serial device <%s> - errno<%d> !!\n",
               serDevName_p, errno);  
        return -1;
    }

    // printf("Opened serial device <%s> \n", serDevName_p); 
#if 0 
    rc = tcgetattr(uartCntxt_p->serialFd, &oldtio); /* save current port settings */
    if (rc < 0)
    {
        printf("\n tcgetattr() failed !! - rc<%d>, errno<%d> \n", rc, errno);
        return -1;
    }
#endif
    bzero(&newtio, sizeof(newtio));

    rc = cfsetspeed(&newtio, baudRate);
    if (rc < 0)
    {
        printf("\n cfsetspeed() failed !! - rc<%d>, errno<%d> \n", rc, errno);
        return -1;
    }

    newtio.c_cflag = CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // if ((rc = fcntl(uartCntxt_p->serialFd, F_SETOWN, getpid())) < 0)
    // {
    //     printf("\n fcntl failed !! - rc<%d>, errno<%d> \n", rc, errno);
    //     return -1;
    // }

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 10 chars received */

    rc = tcsetattr(uartCntxt_p->serialFd, TCSANOW, &newtio);
    if (rc < 0)
    {
        printf("\n tcsetattr() failed !! - rc<%d> / errno<%d> \n", rc, errno);
        return -1;
    }

    rc = tcflush(uartCntxt_p->serialFd, TCIFLUSH);
    if (rc < 0)
    {
        printf("\n tcflush() failed !! - rc<%d> \n", rc);
        return -1;
    }
    
    tcgetattr(uartCntxt_p->serialFd, &latesttio); 
    if (rc < 0)
    {
        printf("\n tcgetattr() failed !! - rc<%d> \n", rc);
        return -1;
    }

    // printf("\nispeed<%d> / ospeed<%d> \n", latesttio.c_ispeed, latesttio.c_ospeed);
    // printf("\niflag<0x%x>/oflag<0x%x>/cflag<0x%x> \n", latesttio.c_iflag, latesttio.c_oflag, latesttio.c_cflag);

    return 1;
}

#else 
/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
int UART_cfgPort(UART_cntxt_s *uartCntxt_p, 
                 const char *serDevName_p, 
                 const int baudRate)
{
   int rc;

   memset(uartCntxt_p, 0, sizeof(UART_cntxt_s));

   uartCntxt_p->serialFd = open((char *)serDevName_p, O_RDWR | O_NOCTTY | O_NDELAY);
   if (uartCntxt_p->serialFd < 0)
   {
       printf("\n<%s> open(%s) failed !! - errno<%d> \n",
              __FUNCTION__, serDevName_p, errno);
       return -1;
   }
   
   // Zero out port status flags
   if (fcntl(uartCntxt_p->serialFd, F_SETFL, 0) != 0x0)
   {
       return -1;
   }

   bzero(&(uartCntxt_p->dcb), sizeof(uartCntxt_p->dcb));

   // uartCntxt_p->dcb.c_cflag |= uartCntxt_p->baudRate;  // Set baud rate first time
   uartCntxt_p->dcb.c_cflag |= baudRate;  // Set baud rate first time
   uartCntxt_p->dcb.c_cflag |= CLOCAL;  // local - don't change owner of port
   uartCntxt_p->dcb.c_cflag |= CREAD;  // enable receiver

   // Set to 8N1
   uartCntxt_p->dcb.c_cflag &= ~PARENB;  // no parity bit
   uartCntxt_p->dcb.c_cflag &= ~CSTOPB;  // 1 stop bit
   uartCntxt_p->dcb.c_cflag &= ~CSIZE;  // mask character size bits
   uartCntxt_p->dcb.c_cflag |= CS8;  // 8 data bits

   // Set output mode to 0
   uartCntxt_p->dcb.c_oflag = 0;
 
   uartCntxt_p->dcb.c_lflag &= ~ICANON;  // disable canonical mode
   uartCntxt_p->dcb.c_lflag &= ~ECHO;  // disable echoing of input characters
   uartCntxt_p->dcb.c_lflag &= ~ECHOE;
 
   // Set baud rate
   uartCntxt_p->baudRate = baudRate;  
   cfsetispeed(&uartCntxt_p->dcb, uartCntxt_p->baudRate);
   cfsetospeed(&uartCntxt_p->dcb, uartCntxt_p->baudRate);

   uartCntxt_p->dcb.c_cc[VTIME] = 0;  // timeout = 0.1 sec
   uartCntxt_p->dcb.c_cc[VMIN] = 1;
 
   if ((tcsetattr(uartCntxt_p->serialFd, TCSANOW, &(uartCntxt_p->dcb))) != 0)
   {
       printf("\ntcsetattr(%s) failed !! - errno<%d> \n",
              serDevName_p, errno);
       close(uartCntxt_p->serialFd);
       return -1;
   }

   // flush received data
   tcflush(uartCntxt_p->serialFd, TCIFLUSH);
   tcflush(uartCntxt_p->serialFd, TCOFLUSH);

   return 1;
}
#endif


/*
 ********************************************************************
 *
 *
 *
 ********************************************************************
 */
int MODEM_writeToSerialPort(MODEM_cntxt_s *cntxt_p, 
                            unsigned char *buff_p, 
                            unsigned int cnt,
                            int flushFlag)
{
   int rc, bytesLeft = cnt, bytesWritten = 0;

   if (verbose) 
       printf("<%s> cnt<%d> \n", __FUNCTION__, cnt);

   rc = UART_flushPortRx(&(cntxt_p->UART_cntxt));
   if (rc < 0)
       return rc;

   while (bytesLeft > 0)
   {
      rc = write(cntxt_p->UART_cntxt.serialFd, buff_p + bytesWritten, bytesLeft);
     
      if (verbose) 
          printf("<%s> rc<%d> bw<%d> \n", __FUNCTION__, rc, bytesWritten);
      
      if (rc <= 0)
          return -1;
      else
      {
          bytesLeft -= rc;
          bytesWritten += rc;
      }
   }

   if (verbose) 
       printf("\n<%s> rc<%d> bw<%d> \n", __FUNCTION__, rc, bytesWritten);

   return 1;
}


/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
int UART_readPort(UART_cntxt_s *cntxt_p, char *buff_p, int len)
{
   int rdLen, readLeft = len, totRead = 0;

   while (readLeft > 0)
   {
      rdLen = read(cntxt_p->serialFd, buff_p + totRead, readLeft);
     
      if (verbose) 
          printf("\n<%s> rdLen<%d> \n", __FUNCTION__, rdLen);

      if (rdLen > 0)
      {
          totRead += rdLen;
          readLeft -= rdLen;
      }
      else
      {
          printf("\n<%s> read() failed  - %d !! \n", __FUNCTION__, rdLen);
          return rdLen;
      }
   }

   return totRead;
}


/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
int UART_flushPortRx(UART_cntxt_s *cntxt_p)
{
   int rc = 0, done = 0;
   
   if (verbose) 
       printf("<%s> Entry \n", __FUNCTION__);

   do
   {
      fd_set set;
      struct timeval timeout;

      FD_ZERO(&set); /* clear the set */
      FD_SET(cntxt_p->serialFd, &set); /* add our file descriptor to the set */

      timeout.tv_sec = 0;
      timeout.tv_usec = 100000;  // 100 milli-secs

      rc = select(cntxt_p->serialFd + 1, &set, NULL, NULL, &timeout);
      if (rc == -1)
      {
          printf("\n<%s> select() failed - errno<%d> !! \n", __FUNCTION__, errno);
          done = 1;
      }
      else
      {
          char buff;

          // if rc is 0, select has timed out !!
          if (rc > 0)
          {
              rc = UART_readPort(cntxt_p, &buff, 1);
              if (rc < 0)
                  done = 1;
              if (verbose)
                  printf("<%s> fbyte<0x%x>\n", __FUNCTION__, buff);
          }
          else
          {
              if (verbose) 
                 printf("<%s> select() timedout - uart flushed \n", __FUNCTION__);
              rc = 0;
              done = 1;
          }
       }
   } while (done == 0);

   return rc;
}


/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
int UART_readPortTmo(UART_cntxt_s *cntxt_p, 
                     unsigned char *buff_p, 
                     unsigned int len, 
                     int timeOutInMilliSecs)
{
   int rc;
   fd_set set;
   struct timeval timeout;

   if (verbose)
       printf("<%s> len<%d>/tmo_msecs<%d> \n", __FUNCTION__, len, timeOutInMilliSecs);

   FD_ZERO(&set); /* clear the set */
   FD_SET(cntxt_p->serialFd, &set); /* add our file descriptor to the set */

   timeout.tv_sec = (timeOutInMilliSecs / 1000);
   timeout.tv_usec = (timeOutInMilliSecs % 1000) * 1000;

   rc = select(cntxt_p->serialFd + 1, &set, NULL, NULL, &timeout);
   if (rc == -1)
   {
       printf("\n<%s> select() failed - errno<%d> !! \n", __FUNCTION__, errno);
   }
   else
   {
       // if rc is 0, select has timed out !!
       if (rc > 0)
       {
          if (verbose)
              printf("<%s> serial port has data to read ... \n", __FUNCTION__);
          rc = UART_readPort(cntxt_p, buff_p, len);
          return rc;
       }
       else
          printf("<%s> serial port select timed out  ... \n", __FUNCTION__);
   }

   return rc;
}


/*
 ********************************************************************
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t MODEM_waitForExactResp(MODEM_cntxt_s *cntxt_p, char *rxBuff_p,
                                   char *expResp_p, int expRespLen, int tmoSecs)
{
   int rc, respRcvd = 0, rdCnt = 0, prefixDelimRcvd = 0;
   MODEM_sts_t sts = MODEM_STS_SUCCESS;
       
   printf("<%s> Entry \n", __FUNCTION__);

   rc = UART_readPortTmo(&(cntxt_p->UART_cntxt),
                         rxBuff_p, expRespLen, tmoSecs*1000);
   printf("<%s> 1 \n", __FUNCTION__);
   if (rc <= 0)
   {
       printf("<%s> UART_readPortTmo() failed !! \n", __FUNCTION__);
       return MODEM_STS_UART_READ_FLR;
   }

   rxBuff_p[expRespLen] = '\0';

   printf("<%s> 2 0x%d, 0x%d, 0x%d \n", __FUNCTION__, rxBuff_p[0], rxBuff_p[1], rxBuff_p[2]);
   if (memcmp(rxBuff_p, expResp_p, expRespLen) != 0)
   {
       printf("<%s> expected exp-resp<%s> / rcvd<%s>  !! \n",
              __FUNCTION__, expResp_p, rxBuff_p);
       sts = MODEM_STS_INVALID_RESPONSE;
   }
   printf("<%s> 3 \n", __FUNCTION__);

   return sts;
}



/*
 ********************************************************************
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t MODEM_getResp(MODEM_cntxt_s *cntxt_p, char *buff_p, 
                          const int buffLen, int *respLen_p)
{
   int rc, respRcvd = 0, rdCnt = 0, prefixDelimRcvd = 0;
   MODEM_sts_t sts = MODEM_STS_SUCCESS;

   do
   {
       rc = UART_readPortTmo(&(cntxt_p->UART_cntxt),
                             buff_p + rdCnt, 1, 10000);
       if (rc <= 0)
           break;

       if (verbose)
           printf("<%s> rdCnt<%d> / rc<%d> / Byte<0x%x>/<%c> \n", 
                  __FUNCTION__, rdCnt, rc, buff_p[rdCnt], buff_p[rdCnt]);

       rdCnt ++;

       if (prefixDelimRcvd == 0)
       {
           if (rdCnt == 2)
           {
               // First two bytes received
               if (buff_p[rdCnt - 1] == '\n'
                   && (buff_p[rdCnt - 2] == '\r'))
               {
                   prefixDelimRcvd = 1;
                   if (verbose)
                       printf("<%s> prefixDelim rcvd !! \n", __FUNCTION__);
                   rdCnt = 0;
               }
               else
               {
                   rc =  -1;
                   break;
               }
           }
       }
       else
       {
           if (rdCnt >= 2)
           {
               if (buff_p[rdCnt - 1] == '\n'
                   && (buff_p[rdCnt - 2] == '\r'))
               {
                   respRcvd = 1;
                   rdCnt -= 2;
                   if (verbose)
                       printf("<%s> Response Received  ... \n", __FUNCTION__);
                   break;
               }
           }
       }
   } while (1);

   if (rc < 0)
       sts = MODEM_STS_UART_READ_FLR;
   else
   {
       if (respRcvd == 0)
       {
           if (rdCnt == 0)
               sts = MODEM_STS_NO_RESPONSE;
           else
           {
               sts = MODEM_STS_INCOMPLETE_RESPONSE;            
           }
       }
   }

   if (sts == MODEM_STS_SUCCESS)
       *respLen_p = rdCnt;

   return sts;
}


/*
 ********************************************************************
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t MODEM_sendCmd(MODEM_cntxt_s *cntxt_p, char *cmdStr)
{
   MODEM_sts_t sts = MODEM_STS_SUCCESS;
   int rc, msgLen = strlen(cmdStr);

   rc = MODEM_writeToSerialPort(cntxt_p, cmdStr, msgLen, 1);
   if (rc != 1)
   {
       rc = MODEM_STS_UART_WRITE_FLR;
   }

   return sts;
}
   

/*
 ********************************************************************
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t MODEM_cfg(MODEM_cntxt_s *cntxt_p, const char *serPortName_p)
{
  MODEM_sts_t sts = MODEM_STS_SUCCESS;

  memset(cntxt_p, 0, sizeof(MODEM_cntxt));

#ifdef MODEM_PART_NR_SIM808
  cntxt_p->gpsEna = 1;
  cntxt_p->battMonEna = 1;
#endif

  printf("<%s> setPortName<%s>\n", __FUNCTION__, serPortName_p);

  if (UART_cfgPort(&(cntxt_p->UART_cntxt), serPortName_p, MODEM_baudRate) < 0)
  {
      printf("<%s> UART_cfgPort(%s) failed !! \n", __FUNCTION__, serPortName_p);
      return MODEM_STS_UART_CFG_FLR;
  }

  printf("<%s> UART_cfgPort(%s) done \n", __FUNCTION__, serPortName_p);

  MODEM_disableEcho(cntxt_p);

  sts = MODEM_verifyMfrAndPartNr(cntxt_p);
  if (sts != MODEM_STS_SUCCESS)
      return sts;
 
  printf("<%s> MODEM mfr is %s and part-nr is %s\n",
         __FUNCTION__, MODEM_cntxt.mfr, MODEM_cntxt.partNr);

  sts = MODEM_checkSIMPresence(cntxt_p);
  if (sts != MODEM_STS_SUCCESS)
  {
      if (sts ==  MODEM_STS_SIM_NOT_PRESENT)
          printf("<%s> SIM not present !! \n", __FUNCTION__);
      else
          printf("<%s> Invalid response !! \n", __FUNCTION__);
      return sts;
  }
  else
  {
      printf("<%s> SIM present \n", __FUNCTION__);
  }

   MODEM_cntxt.dataTxTimeoutSecs = 1;   // 1 second
   MODEM_cntxt.connEstTmoSecs = 60;  // 60 seconds
   MODEM_cntxt.dataExchangeTmoSecs = 20;  // 90 seconds

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
MODEM_sts_t GSM_nwkRegProc(MODEM_cntxt_s *cntxt_p)
{
  MODEM_sts_t sts = MODEM_STS_SUCCESS;
  int regTimeElapsed = 0;


#if 0
  printf("<%s> Requesting Modem to de-register ... \n", __FUNCTION__);
  // Ask modem to de-register
  sts = MODEM_reqGSMRegDeregOpn(cntxt_p, MODEM_REQ_GSM_DEREG_OPN,  NULL);
  if (sts != MODEM_STS_SUCCESS)
  {
       // If modem is not registered, called function will return OPN_FAILED
       return sts;
  }
#endif

  regTimeElapsed = 0;
  do
  {
     sts = MODEM_getGSMRegSts(cntxt_p);
     if (sts != MODEM_STS_SUCCESS)
         break;

     printf("<%s> Modem GSM Reg Sts<%d> \n", __FUNCTION__, cntxt_p->gsmRegStatus);

     if ((cntxt_p->gsmRegStatus == MODEM_GSM_REG_STS_DONE_HOME)
         || (cntxt_p->gsmRegStatus == MODEM_GSM_REG_STS_DONE_ROAMING))
     {
         // Modem is registered .. now check operator is as specified
         sts = MODEM_getOperator(cntxt_p);
         if (sts == MODEM_STS_SUCCESS)
         {
             int strLen = strlen(cntxt_p->operatorStr), idx;

             printf("<%s> Operator String <%s> / specified operator<%s/%s> \n", 
                    __FUNCTION__, cntxt_p->operatorStr, GSM_operatorList[GSM_operatorIdx].operator,
                    GSM_operatorList[GSM_operatorIdx].nwkCode);

             // Convert received string to lowercase
             for (idx=0; idx<strLen; idx++)
             {
                  char c  = cntxt_p->operatorStr[idx];
                  if (c >= 'A' && c <= 'Z')
                      cntxt_p->operatorStr[idx] = 'a' + (c - 'A');
             }
            
             // strstr(haystack, needle) 
             if ((strstr(cntxt_p->operatorStr, GSM_operatorList[GSM_operatorIdx].operator) != NULL)
                 || (strstr(cntxt_p->operatorStr, GSM_operatorList[GSM_operatorIdx].nwkCode) != NULL))
             {
                 // Registered through specified operator ...
                 printf("<%s> Modem registered through specified operator ... \n", __FUNCTION__);
                 return MODEM_STS_SUCCESS;
             }
         }
         else
             return sts;
             
         printf("<%s> Modem not registered through specified operator !!\n", __FUNCTION__);
         printf("<%s> Requesting Modem to de-register !! \n", __FUNCTION__);
         // Ask modem to de-register
         sts = MODEM_reqGSMRegDeregOpn(cntxt_p, MODEM_REQ_GSM_DEREG_OPN,  NULL);
         if (sts != MODEM_STS_SUCCESS)
         {
             // If modem is not registered, called function will return OPN_FAILED
             return sts;
         }
         else
             return sts; 
     }

     if (cntxt_p->gsmRegStatus != MODEM_GSM_REG_STS_NOT_DONE_NOT_SEARCHING)
     {
         if (regTimeElapsed > 10)
         {
             sts = MODEM_STS_GSM_REG_NOT_DONE;
             break;
         }
         regTimeElapsed ++;
         sleep(2);
     }
     else
         break;
  } while(1);
  
  if (sts != MODEM_STS_SUCCESS)
       return sts;

  sleep(3);

  do
  {
#if 0
     printf("<%s> Getting list of network operators in the vicinity ... \n", __FUNCTION__);

     sts = MODEM_getNwkOpsInVicinity(cntxt_p);
     if (sts != MODEM_STS_SUCCESS)
          return sts;
#endif

     printf("<%s> Registering to %s with nwk-code %s and apn %s \n",
            __FUNCTION__, GSM_operatorList[GSM_operatorIdx].operator,
            GSM_operatorList[GSM_operatorIdx].nwkCode,
            GSM_operatorList[GSM_operatorIdx].apn);

     // Ask modem to register only to specified network 
     MODEM_cntxt.operatorSelMode = MODEM_OPERATOR_SEL_MODE_MANUAL;
     sts = MODEM_reqGSMRegDeregOpn(cntxt_p,  
                                MODEM_REQ_GSM_REG_OPN, 
                                GSM_operatorList[GSM_operatorIdx].nwkCode);

     if (sts != MODEM_STS_SUCCESS)
          return sts;

     regTimeElapsed = 0;
     do
     {
        sts = MODEM_getGSMRegSts(cntxt_p);
        if (sts != MODEM_STS_SUCCESS)
            break;

        printf("<%s> Modem GSM Reg Sts<%d> \n", __FUNCTION__, cntxt_p->gsmRegStatus);

        if (cntxt_p->gsmRegStatus != MODEM_GSM_REG_STS_DONE_HOME
            && cntxt_p->gsmRegStatus != MODEM_GSM_REG_STS_DONE_ROAMING)
        {
            if (regTimeElapsed > 10)
            {
                sts = MODEM_STS_GSM_REG_NOT_DONE;
                break;
            }
            regTimeElapsed ++;
            sleep(2);
       }
       else
            break;
     } while(1);

     if (sts == MODEM_STS_SUCCESS)
         break;
  } while(1);

  if (sts != MODEM_STS_SUCCESS)
      return sts;

  MODEM_getOperator(cntxt_p);

#ifdef MODEM_PART_NR_GL868
  GL868_delAllPDPCntxt(cntxt_p);
 
  GL868_deactivatePDPCntxt(cntxt_p);
#endif

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
int GW_readTCPPort(int sockFd, unsigned char *buff_p, unsigned int len)
{
   int rdLen, readLeft = len, totRead = 0;

   printf("<%s> sockFd<%d> /  %d \n", __FUNCTION__, sockFd, len);

   while (readLeft > 0)
   {
      /*
       * The recv() call is normally used only on a connected socket (see connect(2)).
       */

      rdLen = recv(sockFd, buff_p + totRead, 1, 
      totRead == 0 ? MSG_WAITALL : MSG_DONTWAIT);

      if (0)
          printf("<%s> rdLen<%d> totRead<%d> %c \n", __FUNCTION__, rdLen, totRead, buff_p[totRead]);

      if (rdLen > 0)
      {
          totRead += rdLen;
          readLeft -= rdLen;
      }
      else
      {
          if (errno == EAGAIN)
          {
              // printf("<%s> nothing to read() - %d \n", __FUNCTION__, rdLen); 
              return totRead;
          }   
          printf("<%s> read() failed  - %d !! \n", __FUNCTION__, rdLen);
          return rdLen;
      }
   }

   return totRead;
}


/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
int GPRS_IF_connectToCID(void)
{
    int rc = 1;
    struct sockaddr_in cidAddr;

    GW_cidSockFd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (GW_cidSockFd < 0)
    {
        printf("socket() failed !! - errno<%d> \n",  errno);
        return -1;
    }

    memset(&cidAddr, 0, sizeof(cidAddr));
    cidAddr.sin_family      = AF_INET;
    cidAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // CID IP 
    cidAddr.sin_port        = htons(GW_cidPortNr);  // CID port 

    rc = connect(GW_cidSockFd, (struct sockaddr *)&cidAddr, sizeof(cidAddr));
    if (rc < 0)
    {
       printf("connect(%d) failed !! - errno<%d> \n", GW_cidSockFd, errno);
       return -1;
    }

    return 1;
}


/*
 ********************************************************************
 *
 *
 *
 *
 ********************************************************************
 */
int readTCPSock(unsigned char *buff_p, unsigned int len)
{
   int rdLen, readLeft = len, totRead = 0;
   
   while (readLeft > 0)
   {
      /*
       * The recv() call is normally used only on a connected socket (see connect(2)).
       */ 
      rdLen = recv(GW_cidSockFd, buff_p + totRead, readLeft, 0);
      
      if (1)
          printf("\n<%s> rdLen<%d> \n", __FUNCTION__, rdLen);

      if (rdLen > 0)
      {
          totRead += rdLen;
          readLeft -= rdLen;
      }
      else
      {
          printf("\n<%s> read() failed  - %d !! \n", __FUNCTION__, rdLen);
          return rdLen;
      }
   }

   return totRead;
}


/*
 ********************************************************************
 *
 *
 *
 ********************************************************************
 */
unsigned char *GW_procLPWMNMsg(int *msgTotLen_p)
{
   unsigned char hdrBuff[UART_FRAME_HDR_PYLD_CRC_FIELD_OFF];
   unsigned char *msg_p = hdrBuff;
   int totReadLen = 0, done = 0;
   int readLen = UART_FRAME_HDR_PYLD_CRC_FIELD_OFF, off = 0;
   unsigned char *buff_p = hdrBuff;

   memset(hdrBuff, 0, sizeof(hdrBuff));

   do
   {
      int rc = readTCPSock(msg_p + off, readLen);
      if (rc != readLen)
      {
          if (msg_p != hdrBuff)
          {
              free(msg_p);
              msg_p = NULL;
          }
          printf("\nreadPort() failed !! ................. \n");
          break;
      }

      totReadLen += readLen;
      off += readLen;

      if (1)
          printf("\n rc<%d> \n", rc);


      switch (totReadLen)
      {
          case UART_MSG_HDR_PYLD_CRC_FIELD_OFF: 
               {
                   int idx, pyldLen, currMsgType;

                   // Get the message length

                   pyldLen = hdrBuff[UART_MSG_HDR_PYLD_LEN_FIELD_OFF];
                   pyldLen = (pyldLen << 8) |  hdrBuff[UART_MSG_HDR_PYLD_LEN_FIELD_OFF + 1];

                   currMsgType = hdrBuff[UART_MSG_HDR_MSG_TYPE_FIELD_OFF];
                   currMsgType = (currMsgType << 8) | hdrBuff[UART_MSG_HDR_MSG_TYPE_FIELD_OFF + 1];
                   printf("\nMessage Type<%d> / Length<%d>", currMsgType, pyldLen);                   

                   readLen = pyldLen + UART_MSG_HDR_PYLD_CRC_FIELD_LEN;

                   *msgTotLen_p = UART_FRAME_HDR_LEN + pyldLen;
                   msg_p = (unsigned char *)malloc(UART_FRAME_HDR_LEN + pyldLen);
                   if (msg_p == NULL)
                   {
                       printf("<%s> malloc(%d) failed !! \n", UART_FRAME_HDR_LEN + pyldLen);
                       assert(1);
                   }

                   memcpy(msg_p, hdrBuff, UART_FRAME_HDR_LEN);
               }
               break;

          default:
               done = 1;
               break;
      }
   } while (done == 0);

   return msg_p;
}


/*
 ********************************************************************
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t MODEM_procRcvdMsg(MODEM_cntxt_s *cntxt_p, char *rxBuff_p, int rxLen)
{
   MODEM_sts_t sts = MODEM_STS_SUCCESS;

   rxBuff_p[rxLen] = '\0';

   printf("<%s>Received response - len<%d> / Resp <%s> \n",
          __FUNCTION__, rxLen, rxBuff_p);

   if (strstr(rxBuff_p, "NO CARRIER") != NULL)
   {
       printf("<%s> TCP connection closed !! \n", __FUNCTION__);
       sts = MODEM_STS_TCP_CONN_DOWN;
   }

   return sts;
}


char MODEM_txBuff[256];

char GPS_outBuff[128];
char Batt_outBuff[128];

/*
 ********************************************************************
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t GPRS_dataLoop(void)
{
   MODEM_sts_t sts;
   static int dataVal = 12345;
   static unsigned int msgTxCnt = 0;
   MODEM_cntxt_s  *cntxt_p = &MODEM_cntxt;
   
   printf("<%s> Entry .... \n", __FUNCTION__);

   MODEM_txBuff[0] = '\0';
   GPS_outBuff[0] = '\0';
   Batt_outBuff[0] = '\0';

   printf("<%s> gps<%d> battMon<%d> \n",
          __FUNCTION__, cntxt_p->gpsEna, cntxt_p->battMonEna);

#ifdef GPS_SUPPORT_ENA
   if (cntxt_p->gpsEna)
   {
       printf("<%s> GPS enabled \n", __FUNCTION__);

       if (cntxt_p->gpsPwrOn == 0)
       {
           printf("<%s> Enabling GPS Power !! \n", __FUNCTION__);
           sts = MODEM_doGPSPwrCntrlOpn(cntxt_p, 1);
           if (sts == MODEM_STS_SUCCESS)
               cntxt_p->gpsPwrOn = 1;
           sleep(1);
       }

       if (cntxt_p->gpsPwrOn)
       {
           printf("<%s> GPS Powered Up !! \n", __FUNCTION__);

           sts = MODEM_getGPSOpInfo(cntxt_p);
           if (sts == MODEM_STS_SUCCESS)
           {
               sprintf(GPS_outBuff, "GPS<Lat:%lf Long:%lf T:%s.%s>",
                       cntxt_p->latInDeg, 
                       cntxt_p->longInDeg, 
                       cntxt_p->utcTimeStr1,
                       cntxt_p->utcTimeStr2);
           }
           else
               sprintf(GPS_outBuff, "GPS<No Data !!>");
       }
       else
           sprintf(GPS_outBuff, "GPS<Failed to switch on !!>");
   }
   else
   {
       printf("<%s> GPS not enabled \n", __FUNCTION__);
       sprintf(GPS_outBuff, "GPS<Not Equipped !!>");
   }
#endif

#ifdef BATTV_MON_ENA
   if (cntxt_p->battMonEna)
   {
       printf("<%s> Batt Mon Supported \n", __FUNCTION__);
       sts = MODEM_getBattInfo(cntxt_p);
       if (sts == MODEM_STS_SUCCESS)
       {
           float battV = cntxt_p->battV;
           battV /= 1000;
           sprintf(Batt_outBuff, "Batt<L:%d V:%.3fV>", cntxt_p->battChargeLevel, battV);
       }
       else
           sprintf(Batt_outBuff, "Batt<Failed !!> ");
   }
   else
   {
       printf("<%s> Batt Mon Not supported !! \n", __FUNCTION__);
       sprintf(Batt_outBuff, "Batt<No Info !!> ");
   }
#endif

   msgTxCnt ++;

   sprintf(MODEM_txBuff, "Hello-from-India [%4u] ", msgTxCnt);

   if (GPS_outBuff[0] != '\0')
       sprintf(MODEM_txBuff + strlen(MODEM_txBuff), " %s ", GPS_outBuff);

   if (Batt_outBuff[0] != '\0')
       sprintf(MODEM_txBuff + strlen(MODEM_txBuff), " %s ", Batt_outBuff);

   sts = MODEM_sendTCPData(cntxt_p, MODEM_txBuff, strlen(MODEM_txBuff));

   return sts;
}


/*
 ********************************************************************
 *
 *
 *
 ********************************************************************
 */
MODEM_sts_t GPRS_dataLoop1(void)
{
   MODEM_sts_t sts;
   int rc, maxFd;
   fd_set set;
   struct timeval timeout;

   maxFd = GW_cidSockFd > MODEM_cntxt.UART_cntxt.serialFd ? \
           GW_cidSockFd :  MODEM_cntxt.UART_cntxt.serialFd;

   printf("<%s> %d/%d \n", __FUNCTION__, GW_cidSockFd,  MODEM_cntxt.UART_cntxt.serialFd);

   do
   {
      int rc;
     
      FD_ZERO(&set); /* clear the set */
      FD_SET(GW_cidSockFd, &set); /* add our file descriptor to the set */
      FD_SET(MODEM_cntxt.UART_cntxt.serialFd, &set); /* add our file descriptor to the set */
     
      rc = select(maxFd + 1, &set, NULL, NULL, NULL);

      /*
       * Select returns the total number of bits set in readfds, writefds and errorfds, or 
       * zero if the timeout expired, and -1 on error.
       */

      if (rc < 0)
      {
          printf("<%s> select() failed !! - errno<%d> \n",  __FUNCTION__, errno);
          sts = MODEM_STS_SELECT_FLR;
          break;
      }
      else
      {
          if (rc == 0)
          {
              // Should not hit since timeout param is NULL !!
              printf("<%s> select() returned 0 !! \n",  __FUNCTION__);
              assert(500);
          }
          else
          {
              printf("<%s> select() returned %d \n", __FUNCTION__, rc);

              if (FD_ISSET(GW_cidSockFd, &set))
              {
                  int totMsgLen = 0;
                  unsigned char *msg_p = GW_procLPWMNMsg(&totMsgLen);
                  if (msg_p != NULL)
                  {
                      if (verbose)
                      {
                          int idx;
                          for (idx = 0; idx<totMsgLen; idx++)
                          {
                               if (idx % 8 == 0)
                                   printf("\n <%s> : ", __FUNCTION__);
                               printf(" 0x%02x ", msg_p[idx]);
                          }
                          printf("\n");
                      }

#ifdef MODEM_PART_NR_GL868
                      msg_p = GL868_removeSockSuspendEscSeq(msg_p, &totMsgLen);
                      if (msg_p == NULL)
                      {
                          assert(200);
                      }
#endif
                      
                      rc = MODEM_writeToSerialPort(&MODEM_cntxt, msg_p, totMsgLen, 0);
                      free(msg_p);

                      if (rc < 0)
                      {
                          assert(300);  // UART read/write system call failure !!
                      }
                  }
                  else
                  {
                      assert(300);  // UART read system call failure !!
                  }
              }

              if (FD_ISSET(MODEM_cntxt.UART_cntxt.serialFd, &set))
              {
                  int respLen;
                  sts = MODEM_getResp(&MODEM_cntxt, MODEM_rxBuff, 
                                      sizeof(MODEM_rxBuff), &respLen);
                  if (sts != MODEM_STS_SUCCESS)
                  {
                      assert(400);  // UART read system call failure !!
                  }

                  sts = MODEM_procRcvdMsg(&MODEM_cntxt, MODEM_rxBuff, respLen);
                  if (sts == MODEM_STS_TCP_CONN_DOWN)
                      break;
              }
          }
      }

   } while (1);

   return sts;
}

char XMODEM_respBuff[128];


/*
 ********************************************************************
 *
 *
 *
 ********************************************************************
 */
int main(int argc,  char **argv)
{
   if (argc < 6)
   {
       print_usage();
       return 1;
   }

   if (argv[5][0] == '-' && (argv[5][1] == 'f' || argv[5][1] == 'd'))
   {
       if (argv[5][1] == 'd')
       {
          if (daemon(1, 1) < 0)
          {
              return 10;
          }           
       }
   }
   else
   {
       print_usage();
       return 1;
   }

#if 0
   if (GPRS_IF_connectToCID() < 0)
       return 11;
#endif

   {
      int idx = 0;
      do
      {
         if (GSM_operatorList[idx].operator[0] == '\0')
             break;

         if (strstr(GSM_operatorList[idx].operator, argv[2]) != NULL)
         {        
             GSM_operatorIdx = idx;
             break;
         }        

         idx ++;
      } while (1); 

      printf("<%s> Operator <%s> %ssupported \n",
             __FUNCTION__, argv[2], GSM_operatorIdx < 0 ? "not " : "");

      if (GSM_operatorIdx < 0)
          return 20;
   }

   strncpy(SERVER_ipAddr, argv[3], sizeof(SERVER_ipAddr) - 1); 

   {
      int idx, fldCnt, ip[IP_V4_ADDR_LEN], badIP = 1;
      
      fldCnt = sscanf(SERVER_ipAddr, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);

      if (fldCnt == 4)
      {
          for (idx=0; idx<IP_V4_ADDR_LEN; idx++)
          {
              if (ip[idx] < 0 || ip[idx] > 255)
                  break;
          }

          if (idx == IP_V4_ADDR_LEN)
              badIP = 0;
      }

      if (badIP)
      {
          printf("Please specify valid server IPv4 address !! \n");
          return 12;
      }
   }

   SERVER_tcpPort = atoi(argv[4]);
   if (SERVER_tcpPort < 0 || SERVER_tcpPort > 65535)
   {
       printf("Please specify valid server TCP port (0 to 65535) !! \n");
       return 13;
   }

   if (MODEM_cfg(&MODEM_cntxt, (const char *)argv[1]) != MODEM_STS_SUCCESS)
       return 14;

   do
   {
      printf("<%s> Main loop Entry ...\n", __FUNCTION__);
      MODEM_sts_t sts = GSM_nwkRegProc(&MODEM_cntxt);
      printf("<%s> Main loop - GSM_nwkRegProc() rc<%d> \n", 
             __FUNCTION__,  sts);

      if (sts != MODEM_STS_SUCCESS)  // == MODEM_STS_GSM_REG_NOT_DONE)
      {
          sleep(1);
          continue;
      }
   
      if (MODEM_getIPAddr(&MODEM_cntxt, 
                          GSM_operatorList[GSM_operatorIdx].apn) != MODEM_STS_SUCCESS)
      {
          if (verbose)
              printf("<%s> Failed to get IP address !! - Sleeping for 5 seconds  .. \n", __FUNCTION__);
          sleep(5);
          continue;
      }


      do
      {
          if (MODEM_openTCPConn(&MODEM_cntxt, SERVER_ipAddr, SERVER_tcpPort) != MODEM_STS_SUCCESS)
          {
              // See if modem is still registered 
        
              sts = MODEM_getGSMRegSts(&MODEM_cntxt);
              if (sts != MODEM_STS_SUCCESS)
                  break;

              printf("<%s> Modem GSM Reg Sts<%d> \n", __FUNCTION__, MODEM_cntxt.gsmRegStatus);

              if (MODEM_cntxt.gsmRegStatus != MODEM_GSM_REG_STS_DONE_HOME
                  && MODEM_cntxt.gsmRegStatus != MODEM_GSM_REG_STS_DONE_ROAMING)
              {
                  printf("<%s> Modem has de-registered while opening TCP connection !! \n", __FUNCTION__);
                  sts = MODEM_STS_GSM_REG_NOT_DONE;
                  break;
              }

              printf("<%s> Failed to open TCP connection - Sleeping for 5 seconds  .. \n", __FUNCTION__);
              sleep(5);
              continue;
          }
          else
          {
              printf("<%s> TCP connection opened successfully .... \n", __FUNCTION__);
       
              GPRS_dataLoop();
   
              // MODEM_closeTCPConn(&MODEM_cntxt);

              {
                 int idy;
                 for (idy=0; idy<5; idy++)
                 {
                    printf("<%s>waking up in <%d> secs .. \n", __FUNCTION__, 5 - idy);
                    sleep(1);
                 }
              }
          }
      } while (1);
   } while (1);
 
#if 0  
   if (MODEM_suspendTCPConn(&MODEM_cntxt) != MODEM_STS_SUCCESS)
       return 5;
#endif

   return 0;
}
