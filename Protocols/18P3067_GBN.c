#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose
   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0 /* change to 1 if you're doing extra credit */
                        /* and write a routine called B_output */

// macros to be used when calling tolayer5,tolayer3,starttimer,stoptimer,Ack functions
//which requires a AorB parameter instead of hardcoding numbers
#define Avalue 0
#define Bvalue 1
//A macro for buffersize for a fifo that will hold packets that are not yet acked
#define BufferSize 64
// A macro for Sender estimated RTT
#define RTT 17

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */

//datastructure of a message which contains fixed sized payload of 20 charachters / 20 bytes
struct msg
{
    char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt
{
    int seqnum;
    int acknum;
    int checksum;
    char payload[20];
};

// A Structure that contains Sender Entity's required data such as its state in the finite state machine
//, round trip time after which timer interrupt occurs,sender bit sequence
struct SenderEntity
{
    int BasePktNum;                         //the starting packet index
    int NextPktNum;                         // next packet to send from' index
    int WindowSize;                         // the N parameter in Go back N which corresponds to how many packets are sent over network
    struct pkt BufferedPackets[BufferSize]; //circular array / fifo for packets that are not yet acked
    int BufferNextNum;                      // integer index for next buffer index to be placed at
    int RoundTripTime;                      // estimated round trip time of a packet and its ack/nack
};
struct RecieverEntity
{
    int ExpectedSeqNum;
};

void starttimer(int AorB, float increment);
void stoptimer(int AorB);
void tolayer3(int AorB, struct pkt packet);
void tolayer5(int AorB, char datasent[20]);

int CalculateChecksum(struct pkt *packet);
void BufferData(char *data1creator, char *data2reciever);
void SendPacketsInWindow(void);

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
struct SenderEntity A;
struct RecieverEntity B;

/* called from layer 5, passed the data to be sent to other side */

void A_output(struct msg message)
{
    // if buffer is not full of packets
    if (A.BufferNextNum - A.BasePktNum > BufferSize)
    {
        printf("[output] A Sender Entity's Buffer is full message with data %s is being dropped ", message.data);
    }
    printf("[output] A Sender's Entity is buffering message with data %s then sending the data at window", message.data);
    //getting address of next place in buffer to add the packet to it
    struct pkt *packetinbuffer = &A.BufferedPackets[A.BufferNextNum % BufferSize];
    // add sequence number to the packet
    packetinbuffer->seqnum = A.BufferNextNum;
    //move message data to packet payload
    BufferData(message.data, packetinbuffer->payload);
    //adding checksum for packet
    packetinbuffer->checksum = CalculateChecksum(packetinbuffer);
    //incrementing buffer index
    A.BufferNextNum++;
    //send N packets in A's window
    SendPacketsInWindow();
}
void SendPacketsInWindow(void)
{
    while (A.NextPktNum < A.BufferNextNum                 /* as long as buffer is not yet empty */
           && A.NextPktNum < A.BasePktNum + A.WindowSize) /*and as long as the next sequence number is withn the range of the current window*/
    {
        //here since it is a circular array/fifo we get the index of packet by getting the ramiander of the sequence number from the buffer size
        struct pkt *packettobesent = &A.BufferedPackets[A.NextPktNum % BufferSize];
        //now send the packet over network layer (layer 3)
        printf("[output] A Sender's Entity is Sending packet of Sequence number %d over network (layer 3)", packettobesent->seqnum);
        tolayer3(Avalue, *packettobesent);
        // if we are now having both same base and next this means we are sending the first Packet so we will start timer on it
        if (A.BasePktNum == A.NextPktNum)
        {
            starttimer(Avalue, A.RoundTripTime);
        }
        //increment the next packet to be sent sequence number
        A.NextPktNum++;
    }
}

//* function that takes the packet and returns the checksum for it and return it to
//either  add it to packet  or to validate being not corrupted
int CalculateChecksum(struct pkt *packet)
{
    //to calculate the checksum we first sum all the packet headers and payload values
    int checksum = packet->acknum + packet->seqnum;
    for (int i = 0; i < 20; i++)
    {
        checksum += packet->payload[i];
    }

    return checksum;
}

//* void function that buffers data into the packet by identifying its size and copying it elementwise
void BufferData(char *data1creator, char *data2reciever)
{
    for (int i = 0; i < 20; i++)
    {
        data2reciever[i] = data1creator[i];
    }
}

void B_output(struct msg message) /* need be completed only for extra credit */
{
    /*do nothing */
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    if (packet.checksum != CalculateChecksum(&packet))
    {
        printf("[input] A sender Entity Recieved Corrupted Packet of acknowledgement number %d with payload %s and hence being dropped, please resend \n", packet.acknum, packet.payload);
        return;
    }
    else if (packet.acknum < A.BasePktNum)
    {
        printf("[input] A sender Entity Recieved Packet of acknowledgement number %d< %d As a NACK .Dropped \n", packet.acknum, A.BasePktNum);
        return;
    }
    else
    {
        printf("[input] A sender Entity Recieved Correct ACK(%d)=Base(%d) with payload %s ,Incrementing A's Base number\n", packet.acknum, A.BasePktNum, packet.payload);
        //incrementing A's base for last acknowledged packet from B
        A.BasePktNum = packet.acknum + 1;
    }
    //condidition if A's base is the same as next which means that there are no packets in window yet
    if (A.BasePktNum == A.NextPktNum)
    {
        printf("[input] A Sender Entity has no packets to send in window , stopping A's timer");
        stoptimer(Avalue);
        //callig sendpackets in window to check if there are any and open the timer itself
        SendPacketsInWindow();
    }
    // if there are yet packets in the window we open their timers to wait for their acks/nacks
    else
    {
        printf("[input] A Sender entity starting timer for packet of sequence number %d \n", A.BasePktNum);
        starttimer(Avalue, A.RoundTripTime);
    }
}

/* called when A's timer goes off */
void A_timerinterrupt(void)
{
    //checking if there is no packet in window
    if (A.BasePktNum == A.NextPktNum)
    {
        printf("[output] A Sender Entity timer interrupt, nothing left to send\n");
        return;
    }
    else
    {
        printf("[output] A Sender Entity timer interrupt, Resending Un acked packets from sequence number %d to %d \n", A.BasePktNum, A.NextPktNum);

        //looping over packets that are still in window, not yet acked
        for (int i = A.BasePktNum; i < A.NextPktNum; i++)
        {
            //here since it is a circular array/fifo we get the index of packet by getting the ramiander of the sequence number from the buffer size
            struct pkt *packettoberesent = &A.BufferedPackets[i % BufferSize];
            printf("[output] A: Resend packet of sequence number %d, and payload %s \n", packettoberesent->seqnum, packettoberesent->payload);
            tolayer3(Avalue, *packettoberesent);
        }
        // restarting timer after resending
        printf("[output] A Sender Entity Restarted timer after timer interrupt\n");
        starttimer(Avalue, A.RoundTripTime);
    }
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(void)
{
    //initializing A sending entity
    A.BasePktNum = 1;
    A.NextPktNum = 1;
    A.RoundTripTime = RTT;
    A.BufferNextNum = 1;
    A.WindowSize = 8;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    if (packet.checksum != CalculateChecksum(&packet))
    {
        printf("[input] B Reciever Entity Recieved packet of payload %s with corrupted data ,sending NACK please resend \n", packet.payload);
        //adding a wrong ACK num so sender would resend packet
        struct pkt NACKPKT = {.acknum = B.ExpectedSeqNum - 1};
        //adding checksum
        NACKPKT.checksum = CalculateChecksum(&NACKPKT);
        //sending NACK over layer 3
        tolayer3(Bvalue, NACKPKT);
        return;
    }
    else if (packet.seqnum != B.ExpectedSeqNum)
    {
        printf("[input] B Reciever Entity Recieved packet of payload %s with Sequence number=%d  not as expected(%d) (out of order)\n ,sending NACK please resend correct order \n", packet.payload, packet.seqnum, B.ExpectedSeqNum);
        //adding a wrong ACK num so sender would resend packet
        struct pkt NACKPKT = {.acknum = B.ExpectedSeqNum - 1};
        //adding checksum
        NACKPKT.checksum = CalculateChecksum(&NACKPKT);
        //sending NACK over layer 3
        tolayer3(Bvalue, NACKPKT);
        return;
    }
    else
    {
        printf("[input] B Reciever Entity Recieved packet of payload %s with Sequence number=%d  which is as expected(%d),sending ACK please proceed \n", packet.payload, packet.seqnum, B.ExpectedSeqNum);
        //adding correct ACK num to acknowledge Sender
        struct pkt ACKPKT = {.acknum = B.ExpectedSeqNum};
        //adding checksum
        ACKPKT.checksum = CalculateChecksum(&ACKPKT);
        //sending ACK over layer 3
        tolayer3(Bvalue, ACKPKT);
        // incrementing B's Expected Sequence number
        B.ExpectedSeqNum++;
    }
}

/* called when B's timer goes off */
void B_timerinterrupt(void)
{
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init(void)
{

    //initializing B Reciever Entity
    B.ExpectedSeqNum = 1;
}

/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
    - emulates the tranmission and delivery (possibly with bit-level corruption
        and packet loss) of packets across the layer 3/4 interface
    - handles the starting/stopping of a timer, and generates timer
        interrupts (resulting in calling students timer handler).
    - generates message to be sent (passed from later 5 to 4)
THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event
{
    float evtime;       /* event time */
    int evtype;         /* event type code */
    int eventity;       /* entity where event occurs */
    struct pkt *pktptr; /* ptr to packet (if any) assoc w/ this event */
    struct event *prev;
    struct event *next;
};
struct event *evlist = NULL; /* the event list */

/* possible events: */
#define TIMER_INTERRUPT 0
#define FROM_LAYER5 1
#define FROM_LAYER3 2

#define OFF 0
#define ON 1
#define A 0
#define B 1

int TRACE = 1;   /* for my debugging */
int nsim = 0;    /* number of messages from 5 to 4 so far */
int nsimmax = 0; /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob;    /* probability that a packet is dropped  */
float corruptprob; /* probability that one bit is packet is flipped */
float lambda;      /* arrival rate of messages from layer 5 */
int ntolayer3;     /* number sent into layer 3 */
int nlost;         /* number lost in media */
int ncorrupt;      /* number corrupted by media*/

void init(int argc, char **argv);
void generate_next_arrival(void);
void insertevent(struct event *p);

int main(int argc, char **argv)
{
    struct event *eventptr;
    struct msg msg2give;
    struct pkt pkt2give;

    int i, j;
    char c;

    init(argc, argv);
    A_init();
    B_init();

    while (1)
    {
        eventptr = evlist; /* get next event to simulate */
        if (eventptr == NULL)
            goto terminate;
        evlist = evlist->next; /* remove this event from event list */
        if (evlist != NULL)
            evlist->prev = NULL;
        if (TRACE >= 2)
        {
            printf("\nEVENT time: %f,", eventptr->evtime);
            printf("  type: %d", eventptr->evtype);
            if (eventptr->evtype == 0)
                printf(", timerinterrupt  ");
            else if (eventptr->evtype == 1)
                printf(", fromlayer5 ");
            else
                printf(", fromlayer3 ");
            printf(" entity: %d\n", eventptr->eventity);
        }
        time = eventptr->evtime; /* update time to next event time */
        if (eventptr->evtype == FROM_LAYER5)
        {
            if (nsim < nsimmax)
            {
                if (nsim + 1 < nsimmax)
                    generate_next_arrival(); /* set up future arrival */
                /* fill in msg to give with string of same letter */
                j = nsim % 26;
                for (i = 0; i < 20; i++)
                    msg2give.data[i] = 97 + j;
                msg2give.data[19] = 0;
                if (TRACE > 2)
                {
                    printf("          MAINLOOP: data given to student: ");
                    for (i = 0; i < 20; i++)
                        printf("%c", msg2give.data[i]);
                    printf("\n");
                }
                nsim++;
                if (eventptr->eventity == A)
                    A_output(msg2give);
                else
                    B_output(msg2give);
            }
        }
        else if (eventptr->evtype == FROM_LAYER3)
        {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i = 0; i < 20; i++)
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
            if (eventptr->eventity == A) /* deliver packet by calling */
                A_input(pkt2give);       /* appropriate entity */
            else
                B_input(pkt2give);
            free(eventptr->pktptr); /* free the memory for packet */
        }
        else if (eventptr->evtype == TIMER_INTERRUPT)
        {
            if (eventptr->eventity == A)
                A_timerinterrupt();
            else
                B_timerinterrupt();
        }
        else
        {
            printf("INTERNAL PANIC: unknown event type \n");
        }
        free(eventptr);
    }

terminate:
    printf(
        " Simulator terminated at time %f\n after sending %d msgs from layer5\n",
        time, nsim);
    system("pause");
}

void init(int argc, char **argv) /* initialize the simulator */
{
    int i;
    float sum, avg;
    float jimsrand();

    printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
    printf("Enter the number of messages to simulate: ");
    scanf("%d", &nsimmax);
    printf("Enter  packet loss probability [enter 0.0 for no loss]:");
    scanf("%f", &lossprob);
    printf("Enter packet corruption probability [0.0 for no corruption]:");
    scanf("%f", &corruptprob);
    printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
    scanf("%f", &lambda);
    printf("Enter TRACE:");
    scanf("%d", &TRACE);

    srand(9999); /* init random number generator */
    sum = 0.0;   /* test random number generator for students */
    for (i = 0; i < 1000; i++)
        sum = sum + jimsrand(); /* jimsrand() should be uniform in [0,1] */
    avg = sum / 1000.0;
    if (avg < 0.25 || avg > 0.75)
    {
        printf("It is likely that random number generation on your machine\n");
        printf("is different from what this emulator expects.  Please take\n");
        printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
        exit(0);
    }

    ntolayer3 = 0;
    nlost = 0;
    ncorrupt = 0;

    time = 0.0;              /* initialize time to 0.0 */
    generate_next_arrival(); /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand(void)
{
    double mmm = RAND_MAX;
    float x;          /* individual students may need to change mmm */
    x = rand() / mmm; /* x should be uniform in [0,1] */
    return (x);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

void generate_next_arrival(void)
{
    double x, log(), ceil();
    struct event *evptr;
    float ttime;
    int tempint;

    if (TRACE > 2)
        printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

    x = lambda * jimsrand() * 2; /* x is uniform on [0,2*lambda] */
                                 /* having mean of lambda        */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = time + x;
    evptr->evtype = FROM_LAYER5;
    if (BIDIRECTIONAL && (jimsrand() > 0.5))
        evptr->eventity = B;
    else
        evptr->eventity = A;
    insertevent(evptr);
}

void insertevent(struct event *p)
{
    struct event *q, *qold;

    if (TRACE > 2)
    {
        printf("            INSERTEVENT: time is %lf\n", time);
        printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
    }
    q = evlist; /* q points to header of list in which p struct inserted */
    if (q == NULL)
    { /* list is empty */
        evlist = p;
        p->next = NULL;
        p->prev = NULL;
    }
    else
    {
        for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
            qold = q;
        if (q == NULL)
        { /* end of list */
            qold->next = p;
            p->prev = qold;
            p->next = NULL;
        }
        else if (q == evlist)
        { /* front of list */
            p->next = evlist;
            p->prev = NULL;
            p->next->prev = p;
            evlist = p;
        }
        else
        { /* middle of list */
            p->next = q;
            p->prev = q->prev;
            q->prev->next = p;
            q->prev = p;
        }
    }
}

void printevlist(void)
{
    struct event *q;
    int i;
    printf("--------------\nEvent List Follows:\n");
    for (q = evlist; q != NULL; q = q->next)
    {
        printf("Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype,
               q->eventity);
    }
    printf("--------------\n");
}

/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(int AorB /* A or B is trying to stop timer */)
{
    struct event *q, *qold;

    if (TRACE > 2)
        printf("          STOP TIMER: stopping timer at %f\n", time);
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB))
        {
            /* remove this event */
            if (q->next == NULL && q->prev == NULL)
                evlist = NULL;        /* remove first and only event on list */
            else if (q->next == NULL) /* end of list - there is one in front */
                q->prev->next = NULL;
            else if (q == evlist)
            { /* front of list - there must be event after */
                q->next->prev = NULL;
                evlist = q->next;
            }
            else
            { /* middle of list */
                q->next->prev = q->prev;
                q->prev->next = q->next;
            }
            free(q);
            return;
        }
    printf("Warning: unable to cancel your timer. It wasn't running.\n");
}

void starttimer(int AorB /* A or B is trying to stop timer */, float increment)
{
    struct event *q;
    struct event *evptr;

    if (TRACE > 2)
        printf("          START TIMER: starting timer at %f\n", time);
    /* be nice: check to see if timer is already started, if so, then  warn */
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB))
        {
            printf("Warning: attempt to start a timer that is already started\n");
            return;
        }

    /* create future event for when timer goes off */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = time + increment;
    evptr->evtype = TIMER_INTERRUPT;
    evptr->eventity = AorB;
    insertevent(evptr);
}

/************************** TOLAYER3 ***************/
void tolayer3(int AorB /* A or B is trying to stop timer */, struct pkt packet)
{
    struct pkt *mypktptr;
    struct event *evptr, *q;
    float lastime, x;
    int i;

    ntolayer3++;

    /* simulate losses: */
    if (jimsrand() < lossprob)
    {
        nlost++;
        if (TRACE > 0)
            printf("          TOLAYER3: packet being lost\n");
        return;
    }

    /* make a copy of the packet student just gave me since he/she may decide */
    /* to do something with the packet after we return back to him/her */
    mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
    mypktptr->seqnum = packet.seqnum;
    mypktptr->acknum = packet.acknum;
    mypktptr->checksum = packet.checksum;
    for (i = 0; i < 20; i++)
        mypktptr->payload[i] = packet.payload[i];
    if (TRACE > 2)
    {
        printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
               mypktptr->acknum, mypktptr->checksum);
        for (i = 0; i < 20; i++)
            printf("%c", mypktptr->payload[i]);
        printf("\n");
    }

    /* create future event for arrival of packet at the other side */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtype = FROM_LAYER3;      /* packet will pop out from layer3 */
    evptr->eventity = (AorB + 1) % 2; /* event occurs at other entity */
    evptr->pktptr = mypktptr;         /* save ptr to my copy of packet */
                                      /* finally, compute the arrival time of packet at the other end.
                                         medium can not reorder, so make sure packet arrives between 1 and 10
                                         time units after the latest arrival time of packets
                                         currently in the medium on their way to the destination */
    lastime = time;
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == FROM_LAYER3 && q->eventity == evptr->eventity))
            lastime = q->evtime;
    evptr->evtime = lastime + 1 + 9 * jimsrand();

    /* simulate corruption: */
    if (jimsrand() < corruptprob)
    {
        ncorrupt++;
        if ((x = jimsrand()) < .75)
            mypktptr->payload[0] = 'Z'; /* corrupt payload */
        else if (x < .875)
            mypktptr->seqnum = 999999;
        else
            mypktptr->acknum = 999999;
        if (TRACE > 0)
            printf("          TOLAYER3: packet being corrupted\n");
    }

    if (TRACE > 2)
        printf("          TOLAYER3: scheduling arrival on other side\n");
    insertevent(evptr);
}

void tolayer5(int AorB, char datasent[20])
{
    int i;
    if (TRACE >= 2)
    {
        printf("          TOLAYER5: data received: ");
        for (i = 0; i < 20; i++)
            printf("%c", datasent[i]);
        printf("\n");
    }
}