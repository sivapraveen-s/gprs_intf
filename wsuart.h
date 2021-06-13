#ifndef __WSUART_H__
#define __WSUART_H__

#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#define UART_MSG_TYPE_ACK   0x0

// Bit definitions of the flags field in the message header
#define UART_HDR_ACK_BM  (1 << 7)
#define UART_PYLD_ACK_BM  (~(1 << 7))

#define UART_ACK_STS_OK_BM               (1 << 0)
#define UART_ACK_STS_OOM_BM              (1 << 1)
#define UART_ACK_STS_FRAME_TOO_LONG_BM   (1 << 2)
#define UART_ACK_STS_INV_CRC             (1 << 3)
#define UART_ACK_STS_RELAY_IN_PROGRESS   (1 << 4)
#define UART_ACK_STS_HDR_BYTES_MISSING   (1 << 5)
#define UART_ACK_STS_PYLD_BYTES_MISSING  (1 << 6)




#define UART_FRAME_HDR_MSG_TYPE_FIELD_LEN   2
#define UART_FRAME_HDR_FLAGS_FIELD_LEN      1
#define UART_FRAME_HDR_SEQ_NR_FIELD_LEN     1
#define UART_FRAME_HDR_PYLD_LEN_FIELD_LEN   2
#define UART_FRAME_HDR_CRC_FIELD_LEN        2

#define UART_FRAME_HDR_HDR_CRC_FIELD_LEN   UART_FRAME_HDR_CRC_FIELD_LEN
#define UART_FRAME_HDR_PYLD_CRC_FIELD_LEN  UART_FRAME_HDR_CRC_FIELD_LEN

#define UART_FRAME_HDR_MSG_TYPE_FIELD_OFF   0

#define UART_FRAME_HDR_FLAGS_FIELD_OFF \
        UART_FRAME_HDR_MSG_TYPE_FIELD_LEN

#define UART_FRAME_HDR_SEQ_NR_FIELD_OFF \
        (UART_FRAME_HDR_FLAGS_FIELD_OFF + UART_FRAME_HDR_FLAGS_FIELD_LEN)

#define UART_FRAME_HDR_PYLD_LEN_FIELD_OFF \
        (UART_FRAME_HDR_SEQ_NR_FIELD_OFF + UART_FRAME_HDR_SEQ_NR_FIELD_LEN)

#define UART_FRAME_HDR_HDR_CRC_FIELD_OFF  \
        (UART_FRAME_HDR_PYLD_LEN_FIELD_OFF + UART_FRAME_HDR_PYLD_LEN_FIELD_LEN)

#define UART_FRAME_HDR_PYLD_CRC_FIELD_OFF \
        (UART_FRAME_HDR_HDR_CRC_FIELD_OFF + UART_FRAME_HDR_HDR_CRC_FIELD_LEN)

#define UART_FRAME_MAX_PYLD_LEN  128

#define UART_FRAME_HDR_LEN  (UART_FRAME_HDR_MSG_TYPE_FIELD_LEN \
                             + UART_FRAME_HDR_FLAGS_FIELD_LEN \
                             + UART_FRAME_HDR_SEQ_NR_FIELD_LEN \
                             + UART_FRAME_HDR_PYLD_LEN_FIELD_LEN \
                             + UART_FRAME_HDR_HDR_CRC_FIELD_LEN \
                             + UART_FRAME_HDR_PYLD_CRC_FIELD_LEN)


/*
 * Rcvd message format -
 * Msg type (2 bytes)
 * Flags (1 byte)
 * Seq Nr (1 byte)
 * Pyld len (2 bytes)
 * Hdr crc (2 bytes)
 * Pyld crc (2 bytes)
 * Pyld (N bytes)
 */

#define UART_MSG_HDR_MSG_TYPE_FIELD_LEN  2
#define UART_MSG_HDR_FLAGS_FIELD_LEN     1
#define UART_MSG_HDR_SEQ_NR_FIELD_LEN    1
#define UART_MSG_HDR_PYLD_LEN_FIELD_LEN  2
#define UART_MSG_HDR_HDR_CRC_FIELD_LEN   2
#define UART_MSG_HDR_PYLD_CRC_FIELD_LEN  2


#define UART_MSG_HDR_MSG_TYPE_FIELD_OFF  0
#define UART_MSG_HDR_FLAGS_FIELD_OFF     2
#define UART_MSG_HDR_SEQ_NR_FIELD_OFF    3
#define UART_MSG_HDR_PYLD_LEN_FIELD_OFF  4
#define UART_MSG_HDR_HDR_CRC_FIELD_OFF   6
#define UART_MSG_HDR_PYLD_CRC_FIELD_OFF  8

typedef struct
{
  int baudRate; 
  int serialFd;
  struct termios dcb;
} UART_cntxt_s;

#define flush(uartCntxt_p)  tcflush((uartCntxt_p)->commDevFd, TCIFLUSH)

extern int UART_readPort(UART_cntxt_s *c_p, char *buff_p, int len);
extern int UART_writePort(UART_cntxt_s *c_p, char *buff_p, int cnt);
extern int UART_cfgPort(UART_cntxt_s *p, const char *, const int);
extern int UART_flushPortRx(UART_cntxt_s *p);

#endif
