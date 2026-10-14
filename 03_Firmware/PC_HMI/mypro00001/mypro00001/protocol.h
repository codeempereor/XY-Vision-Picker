#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QtGlobal>

#define CAN_ID_POS  0x181
#define CAN_ID_CMD  0x281
#define CAN_ID_ERR  0x381

#define CMD_HOME    0x01
#define CMD_MANUAL  0x02
#define CMD_AUTO    0x03
#define CMD_STOP    0x04
#define CMD_MOVE    0x06

#define ST_IDLE     0
#define ST_HOME     1
#define ST_MANUAL   2
#define ST_AUTO     3
#define ST_GRIP     4
#define ST_DONE     5
#define ST_ERROR    6

#endif
