#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BIDIRECTIONAL 0

struct msg {
    char data[20];
};

struct pkt {
    int seqnum;
    int acknum;
    int checksum;
    char payload[20];
};

/***********************************************/
/*    STUDENTS WRITE THE NEXT SEVEN ROUTINES   */
/***********************************************/

/**
 * Estrutura de controle para o remetente
 * 
 * last_packet: 
 * rtt: 
 * waiting_for_ack: 
 * seqnum: 
 */
struct a {
    struct pkt last_packet;
    float rtt;
    int waiting_for_ack;
    int seqnum;
};

/**
 * Estrutura de controle para o destinatário 
 * 
 * seqnum: 
 */
struct b {
    int seqnum;
};

/**
 * Definição das funções que o código vai usar para evitar warning na compilação
 */
void starttimer(int AorB, float increment);
void stoptimer(int AorB);
void tolayer3(int AorB, struct pkt packet);
void tolayer5(int AorB, char datasent[20]);

/**
 * Atualiza o checksum do pacote, somando o seqnum, acknum e todos as posições
 * da payload, sugerido pela descrição do trabalho
 */
void update_pkt_checksum(struct pkt *packet) {
    int checksum = packet->seqnum + packet->acknum;

    for (int i = 0; i < 20; i++) {
        checksum += packet->payload[i];
    }

    packet->checksum = checksum;
}

/**
 * Verifica se o checksum do pacote é igual ao checksum esperado
 */
int is_valid_checksum(int expected_checksum, struct pkt *packet) {
    int checksum = packet->seqnum + packet->acknum;

    for (int i = 0; i < 20; i++) {
        checksum += packet->payload[i];
    }

    return (checksum == expected_checksum);
}

/**
 * Envia um pacote para o layer 3 com um ACK
 */
void send_ACK(int AorB, int seqnum) {
    struct pkt packet;
    packet.acknum = seqnum;
    update_pkt_checksum(&packet);
    tolayer3(AorB, packet);
}

/**
 * Envia um pacote para o layer 3 com um NAK, nesse caso o acknum será
 * um valor negativo para podermos diferenciar
 */
void send_NAK(int AorB, int seqnum) {
    struct pkt packet;
    packet.acknum = 1 - seqnum;
    update_pkt_checksum(&packet);
    tolayer3(AorB, packet);
}


struct a A;
struct b B;

#define RTT 15.0


void A_output(struct msg message) {
    if (A.waiting_for_ack) {
        printf("[A_output] Remetente aguardando ACK, regeitando mensagem (data: %s)\n", message.data);
        return;
    }

    printf("[A_output] Enviando pacote (pkt: %d, payload: %s)\n", A.seqnum, message.data);

    struct pkt packet;
    packet.seqnum = A.seqnum;

    for (int i = 0; i < 20; i++) {
        packet.payload[i] = message.data[i];
    }

    update_pkt_checksum(&packet);

    A.last_packet = packet;
    A.waiting_for_ack = 1;

    tolayer3(0, packet);
    starttimer(0, A.rtt);
}

/**
 * Utilizada para implementação bidirecional, nessa caso não será utilizada
 */
void B_output(struct msg message) {
    printf("[B_output] Pulando\n");
}


void A_input(struct pkt packet) {

    if(packet.acknum != A.seqnum) {
        printf("[A_input] Descartando pacote fora de ordem (pkt recebido: %d, pkt esperado: %d)\n", packet.acknum, A.seqnum);
        return;
    }

    if (!is_valid_checksum(packet.checksum, &packet)) {
        printf("[A_input] Descartando pacote corrompido (pkt: %d)\n", packet.seqnum);
        return;
    }

    if (!A.waiting_for_ack) {
        printf("[A_input] Erro: comunicação bidirecional não implementada\n");
        return;
    }

    printf("[A_input] ACK recebido (acknum: %d)\n", packet.acknum);

    stoptimer(0);

    A.seqnum = 1 - A.seqnum;
    A.waiting_for_ack = 0;
}

/**
 * Executa quando o timer inicializado no envio de um pacote é estourado,
 * então o último pacote é re-enviado.
 */
void A_timerinterrupt() {
    if (!A.waiting_for_ack) {
        printf("[A_timerinterrupt] Erro: pacote não está esperando ack (pkt: %d)\n", A.seqnum);
        return;
    }

    printf("[A_timerinterrupt] Timeout. Reenviando (pkt: %d, payload: %s)\n", A.last_packet.seqnum, A.last_packet.payload);

    tolayer3(0, A.last_packet);
    starttimer(0, A.rtt);
}

/**
 * Inicializa o remetente
 */
void A_init() {
    A.rtt = RTT;
    A.waiting_for_ack = 0;
    A.seqnum = 0;
}

void B_input(struct pkt packet) {

    /**
     * Se o pacote recebido não é o pacote esperado, mandar um NAK
     * para o pacote esperado
     */
    if (packet.seqnum != B.seqnum) {
        printf("[B_input] Enviando NAK, pacote fora de ordem (pkt recebido: %d, pkt esperado: %d)\n", packet.seqnum, B.seqnum);
        send_NAK(1, B.seqnum);
        return;
    }

    /**
     * Se o pacote está corrompido, enviar um NAK para o pacote esperado
     */
    if (!is_valid_checksum(packet.checksum, &packet)) {
        printf("[B_input] Enviando NAK, checksum incorreto (pkt: %d)\n", packet.seqnum);
        send_NAK(1, B.seqnum);
        return;
    }

    /**
     * Pacote válido recebido, enviar um ACK, mandar para layer 5 e
     * mudar seqnum do receptor para o próximo pacote
     */
    printf("[B_input] Pacote recebido (pkt: %d, payload: %s)\n", packet.seqnum, packet.payload);
    printf("[B_input] Enviando ACK\n");

    send_ACK(1, B.seqnum);
    tolayer5(1, packet.payload);
    B.seqnum = 1 - B.seqnum;
}

/**
 * Na inplementação unidirecional o B não tem um timer, então não será utilizado
 */
void B_timerinterrupt() {
    printf("[B_timerinterrupt] Pulando\n");
}

/**
 * Inicializa o destinatário com um número de sequência 0
 */
void B_init() {
    B.seqnum = 0;
}


/***********************************************/
/*     NETWORK EMULATION CODE STARTS BELOW      */
/***********************************************/

struct event {
    float evtime;           /* event time */
    int evtype;             /* event type code */
    int eventity;           /* entity where event occurs */
    struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
    struct event *prev;
    struct event *next;
};

struct event *evlist = NULL;   /* the event list */

/* possible events: */
#define TIMER_INTERRUPT    0
#define FROM_LAYER5        1
#define FROM_LAYER3        2

#define OFF                0
#define ON                 1
#define A                  0
#define B                  1

int TRACE = 1;                  /* for my debugging */
int nsim = 0;                   /* number of messages from 5 to 4 so far */
int nsimmax = 0;                /* number of msgs to generate, then stop */
float time = (float)0.000;
float lossprob;                 /* probability that a packet is dropped  */
float corruptprob;              /* probability that one bit is packet is flipped */
float lambda;                   /* arrival rate of messages from layer 5 */
int ntolayer3;                  /* number sent into layer 3 */
int nlost;                      /* number lost in media */
int ncorrupt;                   /* number corrupted by media*/

void init();
void generate_next_arrival();
void insertevent(struct event *p);

int main() {
    struct event *eventptr;
    struct msg  msg2give;
    struct pkt  pkt2give;

    int i,j;
    char c;

    init();
    A_init();
    B_init();

    while (1) {
        /* get next event to simulate */
        eventptr = evlist;

        if (eventptr == NULL) {
            goto terminate;
        }
        
        /* remove this event from event list */
        evlist = evlist->next;  

        if (evlist != NULL) {
            evlist->prev = NULL;
        }
           
        if (TRACE >= 2) {
            printf("\nEVENT time: %f,", eventptr->evtime);
            printf("  type: %d", eventptr->evtype);

            if (eventptr->evtype == 0) {
                printf(", timerinterrupt  ");
            } else if (eventptr->evtype == 1) {
                printf(", fromlayer5 ");
            } else {
                printf(", fromlayer3 ");
            }
	     
            printf(" entity: %d\n", eventptr->eventity);
        }

        /* update time to next event time */
        time = eventptr->evtime;
	  
        if (eventptr->evtype == FROM_LAYER5 && nsim < nsimmax) {
            /* set up future arrival */
            if (nsim + 1 < nsimmax) {
                generate_next_arrival();
            }

            /* fill in msg to give with string of same letter */
            j = nsim % 26;
            for (i = 0; i < 20; i++) {
                msg2give.data[i] = 97 + j;
            }
            msg2give.data[19] = 0;
               
            if (TRACE > 2) {
                printf("          MAINLOOP: data given to student: ");

                for (i = 0; i < 20; i++) {
                    printf("%c", msg2give.data[i]);
                }
                  
               printf("\n");
	        }

            nsim++;

            if (eventptr->eventity == A) {
                A_output(msg2give);
            } else {
                B_output(msg2give);
            }
        } else if (eventptr->evtype == FROM_LAYER3) {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;

            for (i = 0; i < 20; i++) {
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
            }
            
            /* deliver packet by calling */
            if (eventptr->eventity == A) {
                A_input(pkt2give);
            } else {
                B_input(pkt2give);
            }
            
            /* free the memory for packet */
            free(eventptr->pktptr);
        } else if (eventptr->evtype == TIMER_INTERRUPT) {
            if (eventptr->eventity == A) {
                A_timerinterrupt();
            } else {
                B_timerinterrupt();
            }
        } else {
	        printf("INTERNAL PANIC: unknown event type \n");
        }

        free(eventptr);
    }

    terminate:
        printf("\nSimulator terminated at time %f after sending %d msgs from layer5\n", time, nsim);
}

/***********************************************/
/*           INITIALIZE THE SIMULATOR          */
/***********************************************/

void init() {
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

    /* init random number generator */
    srand(9999);

    /* test random number generator for students */
    sum = (float)0.0;   

    for (i = 0; i < 1000; i++) {
        /* jimsrand() should be uniform in [0,1] */
        sum = sum + jimsrand();
    }
      
    avg = sum / (float)1000.0;

    if (avg < 0.25 || avg > 0.75) {
        printf("It is likely that random number generation on your machine\n" );
        printf("is different from what this emulator expects.  Please take\n");
        printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
        exit(0);
    }

    ntolayer3 = 0;
    nlost = 0;
    ncorrupt = 0;

    /* initialize time to 0.0 */
    time = (float)0.0;

    /* initialize event list */
    generate_next_arrival();
}

/***********************************************/
/*           RANDOM GENERATOR ROUTINE          */
/***********************************************/

float jimsrand() {
    /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
    double mmm = RAND_MAX;

    /* individual students may need to change mmm */
    float x;

    /* x should be uniform in [0,1] */
    x = (float)(rand() / mmm);

    return(x);
}

/***********************************************/
/*           EVENT HANDLINE ROUTINES           */
/***********************************************/

void generate_next_arrival() {
    double x, log(), ceil();
    struct event *evptr;

    if (TRACE > 2) {
        printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");
    }

    /* x is uniform on [0,2*lambda] */
    /* having mean of lambda        */
    x = lambda * jimsrand() * 2;

    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = (float)(time + x);
    evptr->evtype = FROM_LAYER5;

    if (BIDIRECTIONAL && (jimsrand() > 0.5)) {
        evptr->eventity = B;
    } else {
        evptr->eventity = A;
    }

    insertevent(evptr);
}

void insertevent(struct event *p) {
    struct event *q, *qold;

    if (TRACE > 2) {
        printf("            INSERTEVENT: time is %lf\n", time);
        printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
    }

    /* q points to header of list in which p struct inserted */
    q = evlist;

    /* list is empty */
    if (q == NULL) {
        evlist = p;
        p->next = NULL;
        p->prev = NULL;
    } else {
        for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next) {
            qold=q;
        }

        if (q == NULL) {            /* end of list */
            qold->next = p;
            p->prev = qold;
            p->next = NULL;
        } else if (q == evlist) {   /* front of list */
            p->next = evlist;
            p->prev = NULL;
            p->next->prev = p;
            evlist = p;
        } else {                    /* middle of list */
            p->next = q;
            p->prev = q->prev;
            q->prev->next = p;
            q->prev = p;
        }
    }
}

void printevlist() {
    struct event *q;

    printf("--------------\nEvent List Follows:\n");

    for(q = evlist; q != NULL; q = q->next) {
        printf("Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype, q->eventity);
    }

    printf("--------------\n");
}

/***********************************************/
/*          STUDENT-CALLABLE ROUTINES          */
/***********************************************/

void stoptimer(int AorB) {
    struct event *q;

    if (TRACE > 2) {
        printf("          STOP TIMER: stopping timer at %f\n", time);
    }
    
    /* for (q = evlist; q != NULL && q->next != NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next) {
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB)) {
            /* remove this event */
            if (q->next == NULL && q->prev == NULL) {   /* remove first and only event on list */
                evlist = NULL;
            } else if (q->next == NULL) {               /* end of list - there is one in front */
                q->prev->next = NULL;
            } else if (q == evlist) {                   /* front of list - there must be event after */
                q->next->prev = NULL;
                evlist = q->next;
            } else {                                    /* middle of list */
                q->next->prev = q->prev;
                q->prev->next = q->next;
            }

            free(q);
            return;
        }
    }
    
    printf("Warning: unable to cancel your timer. It wasn't running.\n");
}

void starttimer(int AorB, float increment) {
    struct event *q;
    struct event *evptr;

    if (TRACE > 2) {
        printf("          START TIMER: starting timer at %f\n",time);
    }

    /* be nice: check to see if timer is already started, if so, then warn */
    /* for (q = evlist; q != NULL && q->next != NULL; q = q->next) */
    for (q = evlist; q != NULL; q = q->next) {
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB)) {
            printf("Warning: attempt to start a timer that is already started\n");
            return;
        }
    }

    /* create future event for when timer goes off */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = (float)(time + increment);
    evptr->evtype = TIMER_INTERRUPT;
    evptr->eventity = AorB;

    insertevent(evptr);
}

void tolayer3(int AorB, struct pkt packet) {
    struct pkt *mypktptr;
    struct event *evptr, *q;
    float lastime, x, jimsrand();
    int i;

    ntolayer3++;

    /* simulate losses: */
    if (jimsrand() < lossprob) {
        nlost++;

        if (TRACE > 0) {
            printf("          TOLAYER3: packet being lost\n");
        }
    
        return;
    }

    /* make a copy of the packet student just gave me since he/she may decide */
    /* to do something with the packet after we return back to him/her */
    mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
    mypktptr->seqnum = packet.seqnum;
    mypktptr->acknum = packet.acknum;
    mypktptr->checksum = packet.checksum;

    for (i = 0; i < 20; i++) {
        mypktptr->payload[i] = packet.payload[i];
    }

    if (TRACE > 2) {
        printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum, mypktptr->acknum, mypktptr->checksum);

        for (i = 0; i < 20; i++) {
            printf("%c", mypktptr->payload[i]);
        }

        printf("\n");
    }

    /* create future event for arrival of packet at the other side */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtype = FROM_LAYER3;    /* packet will pop out from layer3 */
    evptr->eventity = (AorB + 1) % 2;   /* event occurs at other entity */
    evptr->pktptr = mypktptr;   /* save ptr to my copy of packet */
    
    /* finally, compute the arrival time of packet at the other end.
    medium can not reorder, so make sure packet arrives between 1 and 10
    time units after the latest arrival time of packets
    currently in the medium on their way to the destination */

    lastime = time;

    /* for (q = evlist; q != NULL && q->next != NULL; q = q->next) */
    for (q = evlist; q != NULL; q = q->next) {
        if ((q->evtype == FROM_LAYER3 && q->eventity == evptr->eventity)) {
            lastime = q->evtime;
        }
    }

    evptr->evtime = lastime + 1 + 9 * jimsrand();

    /* simulate corruption: */
    if (jimsrand() < corruptprob) {
        ncorrupt++;

        if ((x = jimsrand()) < .75) {
            mypktptr->payload[0] = 'Z'; /* corrupt payload */
        } else if (x < .875) {
            mypktptr->seqnum = 999999;
        } else {
            mypktptr->acknum = 999999;
        }

        if (TRACE > 0) {
            printf("          TOLAYER3: packet being corrupted\n");
        }
    }

    if (TRACE > 2) {
        printf("          TOLAYER3: scheduling arrival on other side\n");
    }
     
    insertevent(evptr);
}

void tolayer5(int AorB, char datasent[20]) {
    int i;

    if (TRACE > 2) {
        printf("          TOLAYER5: data received: ");

        for (i = 0; i < 20; i++) {
            printf("%c", datasent[i]);
        }

        printf("\n");
    }
}