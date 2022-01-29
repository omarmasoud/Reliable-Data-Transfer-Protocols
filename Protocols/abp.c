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

//Set of all possible States in the finite state machine for both Sender and reciever entity
enum FsmStates
{
    WAITING_FOR_ACKNOWLEDGEMENT,
    WAITING_FOR_LAYER5_CALL,
    WAITING_FOR_0_SEQ,
    WAITING_FOR_1_SEQ
};

// A Structure that contains Sender Entity's required data such as its state in the finite state machine
//, round trip time after which timer interrupt occurs,sender bit sequence
struct SenderRecieverEntity
{
    enum FsmStates SenderFsmState;
    int RoundTripTime;
    bool Senderbitsequence;
    struct pkt lastpacket;
    enum FsmStates RecieverFsmState;
    bool recieverbitsequence;
};

void starttimer(int AorB, float increment);
void stoptimer(int AorB);
void tolayer3(int AorB, struct pkt packet);
void tolayer5(int AorB, char datasent[20]);

int CalculateChecksum(struct pkt *packet);
void BufferData(char *data1creator, char *data2reciever);
void Ack(int AorB, int ack);

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
struct SenderRecieverEntity A;
struct SenderRecieverEntity B;

/* called from layer 5, passed the data to be sent to other side */

void A_output(struct msg message)
{
    switch (A.SenderFsmState)
    {
    case WAITING_FOR_ACKNOWLEDGEMENT:
        printf("packet with message (%s) is dropped \n as A is still waiting for acknowledgement\n", message.data);
        break;
    case WAITING_FOR_LAYER5_CALL:
    {
        //making a new packet
        struct pkt newPacket;
        newPacket.seqnum = A.Senderbitsequence;
        //setting package acknowledgement number for synchronizing with reciever
        newPacket.acknum = A.Senderbitsequence;
        //buffering data for the new packet
        BufferData(message.data, newPacket.payload);
        // adding checksum to the packet
        newPacket.checksum = CalculateChecksum(&newPacket);
        //sending packet to layer3
        tolayer3(Avalue, newPacket);
        printf("packet sent from A with sequence number of %d and data of %s\n", newPacket.seqnum, newPacket.payload);
        // Changing A's State
        A.SenderFsmState = WAITING_FOR_ACKNOWLEDGEMENT;
        // setting last packet in A to the formed packet in cas lost
        A.lastpacket = newPacket;
        //starting timer for the stop and wait process
        starttimer(Avalue, A.RoundTripTime);
    }
    default:
        break;
    }
}
int CalculateChecksum(struct pkt *packet)
{
    //to calculate the checksum we first sum all the packet headers and payload values
    int checksum = packet->acknum + packet->seqnum;
    for (int i = 0; i < 20; i++)
    {
        checksum += packet->payload[i];
    }

    // then we take the complemnt of the summation and assign it to the packet's checksum value
    return checksum;
}

//* void function that buffers data into the packet by identifying its size and copying it elementwise
void BufferData(char *data1creator, char *data2reciever)
{
   // int datasize = (int)(sizeof(data1creator) / sizeof(char)); //gets the size of the buffer to loop on
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
    // checking the sender's state in entity A
    switch (A.SenderFsmState)
    {
        //case not waiting for acknowledgement
    case WAITING_FOR_ACKNOWLEDGEMENT:
        if (packet.checksum == CalculateChecksum(&packet))
        {
            if (packet.acknum == A.Senderbitsequence)
            {
                printf("entity A recieved Correct packet Acknowledgement please proceed\n");
                // sincec checksum and acknowledgement number are correct so we stop the timer and send the packet to layer 5
                stoptimer(Avalue);
                //also since this packet is an ack packet we just consume it and don't pass it to layer 5
                A.SenderFsmState = WAITING_FOR_LAYER5_CALL;
                //flipping A's Senderbitsequence
                A.Senderbitsequence != A.Senderbitsequence;
            }
            else
            {
                printf("entity A recieved unexpected ack which is not equal to %d , correct and resend\n", A.Senderbitsequence);
               // tolayer3(Avalue, A.lastpacket);
                return;
            }
        }
        else
        {
            printf("Recieved packet At A with payload %s is corrupted , resend\n", packet.payload);
           // tolayer3(Avalue, A.lastpacket);
            return;
        }
        break;

    default:
        // case when A Sender is not waiting for acknowledgement and not waiting for Ack messages
        printf("A entity is not waiting for acknowledgement so cant take this input with data %s and will drop it \n ", packet.payload);
        break;
    }
}

/* called when A's timer goes off */
void A_timerinterrupt(void)
{
    switch (A.SenderFsmState)
    {
    case WAITING_FOR_ACKNOWLEDGEMENT:
        printf("A Entity's timer timed out, resending last packet of payload %s \n", A.lastpacket.payload);
        tolayer3(Avalue, A.lastpacket); 
        starttimer(Avalue, A.RoundTripTime);
        break;

    default:
        printf("A Entity's timer timed out while not waiting for acknowledgement hence no data on the network and ignore\n");
        break;
    }
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(void)
{
    //initializing A sending entity
    A.RoundTripTime = 100,
    A.Senderbitsequence = 0,
    A.SenderFsmState = WAITING_FOR_LAYER5_CALL;
}
// void function that sends acknowledgement or negative acknowledgement for A or B ( the sender )
void Ack(int AorB, int ack)
{
    //make a packet with no payload just acknowledgement field
    struct pkt ackpacket;
    ackpacket.acknum = ack;
    //then add checksum to it to check corruption of the Ack packet
    ackpacket.checksum = CalculateChecksum(&ackpacket);
    //send ack packet to the network layer destined to the sender side specified in the parameters
    tolayer3(AorB, ackpacket);
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    if (packet.checksum != CalculateChecksum(&packet))
    {
        printf("Recieved packet At B with payload %s is corrupted , sending negative acknowledgment please resend \n", packet.payload);
        //send -ve acknowledgement
        Ack(Bvalue, !B.recieverbitsequence);
        return;
    }
    //if checksum is okay and data not corrupted

    if (packet.seqnum == B.recieverbitsequence)
    {
        printf("Recieved Packet At B with payload %s , packet is not corrupted, sending positive acknowledgement to A Entity\n ", packet.payload);
        //positively acknowledging A sender
        Ack(Bvalue, B.recieverbitsequence);
        //send data for application layer for later use
        tolayer5(Bvalue, packet.payload);
        B.recieverbitsequence != B.recieverbitsequence;
        return;
    }
    else
    {
        printf("Recieved Packet At B is not corrupted but with different Ack num hence dropped with negative acknowledgement\n");
        Ack(Bvalue, !B.recieverbitsequence);
        return;
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
    B.recieverbitsequence = 0;
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
    if (TRACE>=2)
    {
        printf("          TOLAYER5: data received: ");
        for (i = 0; i < 20; i++)
            printf("%c", datasent[i]);
        printf("\n");
    }
}