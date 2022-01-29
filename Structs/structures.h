#pragma once
#include <stdint.h>
struct msg
{
    char data [20];

};
struct pkt {
int seqnum;
int acknum;
int checksum;
char payload[20];
};
// struct  Sender
// {
//     /* data */
// };
// struct Reciever
// {
//     /* data */
// };
enum States{WAITFORACK};

