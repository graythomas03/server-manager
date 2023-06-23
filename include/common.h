#ifndef _COMMON_H
#define _COMMON_H

enum RequestType
{
    REQUEST_SYNC = 0,
    REQUEST_ARCHIVE,
    REQUEST_EMAIL
};

struct RequestPacket
{
    enum RequestType type;
};

#endif