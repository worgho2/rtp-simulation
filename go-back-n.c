#include <stdio.h>
#include <stdlib.h>

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

#define RTT                 50.0
#define SENDER_WINDOW_SIZE  8
#define SENDER_BUFFER_SIZE  50

/**
 * 
 */
struct remetente {
    float rtt;
    int pkt_window_size;
    int next_seqnum;
    int pkt_buffer_current_size;
    struct pkt pkt_buffer[SENDER_BUFFER_SIZE + 10];
} A;

/**
 * @expected_seqnum: Número de sequência do pacote esperado
 */
struct receptor {
    int expected_seqnum;
} B;

/**
 * Definição das funções que o código vai usar para evitar warning na compilação
 */
void starttimer(int AorB, float increment);
void stoptimer(int AorB);
void tolayer3(int AorB, struct pkt packet);
void tolayer5(int AorB, char datasent[20]);

/**
 * Atualiza checksum de um pacote somando seqnum, acknum e payload
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
 * Envia ACK para o meio
 */
void send_ACK(int AorB, int seqnum) {
    struct pkt packet;
    packet.acknum = seqnum;
    update_pkt_checksum(&packet);

    printf("[send_ACK] Enviando (acknum: %d)\n", seqnum);
    tolayer3(AorB, packet);
}

/**
 * Envia NAK para o meio
 */
void send_NAK(int AorB, int seqnum) {
    struct pkt packet;
    packet.acknum = - seqnum;
    update_pkt_checksum(&packet);

    printf("[send_NAK] Enviando (acknum: %d)\n", packet.acknum);
    tolayer3(AorB, packet);
}

/**
 * Desliza a o buffer para a esquerda, quando receber um ACK
 * 
 * 1. Descobre qual é a posição do pacote que recebeu ACK
 * 2. Desliza o buffer para esquerda até o pacote que recebeu ACK
 * 3. Preenche com packets vazios as posições finais, indicando posição livre
 */
void slide(int acknum) {

}

void slide_window(int acknum) {
    int acked_index = -1;

    for (int i = 0; i < SENDER_BUFFER_SIZE; i++) {
        if (A.pkt_buffer[i].seqnum == acknum) {
            acked_index = i;
            break;
        }
    }

    if (acked_index == -1) {
        printf("o ack é de um pacote que não está mais no buffer");
        return;
    }

    for (int i = acked_index + 1; i < SENDER_BUFFER_SIZE; i++) {
        A.pkt_buffer[i - acked_index + 1] = A.pkt_buffer[i];
    }

    for (int i = SENDER_BUFFER_SIZE - 1; i >= SENDER_BUFFER_SIZE - (acked_index + 1); i--) {
        struct pkt packet;
        packet.seqnum = 0;
        A.pkt_buffer[i] = packet;
    }

    A.pkt_buffer_current_size -= acked_index + 1;
}

/**
 * Envia a janela para o meio
 * 
 * 1. Calcula quais pacotes serão enviados, sem exceder a janela
 * 2. Envia os pacotes 1 a 1
 */
void send_window() {
    printf("[send_window] Buffer (current size: %d, capacity: %d)\n", A.pkt_buffer_current_size, SENDER_BUFFER_SIZE);
    for (int i = 0; i < SENDER_BUFFER_SIZE; i++) {
        printf(" %d |", A.pkt_buffer[i].seqnum);
    }
    printf("\n");

    printf("[send_window] Window (size: %d)\n", A.pkt_window_size);
    for (int i = 0; i < A.pkt_window_size; i++) {
        printf(" %d |", A.pkt_buffer[i].seqnum);
    }
    printf("\n");

    int number_of_packets_to_send = A.pkt_buffer_current_size > A.pkt_window_size ? A.pkt_window_size : A.pkt_buffer_current_size;

    for (int i = 0; i < number_of_packets_to_send; i++) {
        printf("[send_window] Sending (pkt: %d, payload: %s)\n", A.pkt_buffer[i].seqnum, A.pkt_buffer[i].payload);

        tolayer3(0, A.pkt_buffer[i]);
    }
}

/**
 * Adiciona um novo pacote no buffer
 * 
 * 1. Adiciona o pacote na posição correta
 * 2. Incrementa a variável de tamanho do buffer
 */
void add_pkt_to_buffer(struct pkt packet) {
    printf("[add_pkt_to_buffer] Antes (buffer size: %d)\n", A.pkt_buffer_current_size);

    for (int i = 0; i < SENDER_BUFFER_SIZE; i++) {
        printf(" %d |", A.pkt_buffer[i].seqnum);
    }

    printf("\n");

    A.pkt_buffer[A.pkt_buffer_current_size] = packet;
    A.pkt_buffer_current_size++;

    printf("[add_pkt_to_buffer] Depois (buffer size: %d)\n", A.pkt_buffer_current_size);

    for (int i = 0; i < SENDER_BUFFER_SIZE; i++) {
        printf(" %d |", A.pkt_buffer[i].seqnum);
    }

    printf("\n");
}

/***********************************************/
/*                  REMETENTE                  */
/***********************************************/

/**
 * Inicializa remetente
 */
void A_init() {
    A.rtt = RTT;
    A.pkt_window_size = SENDER_WINDOW_SIZE;
    A.next_seqnum = 1;
    A.pkt_buffer_current_size = 0;

    for(int i = 0; i < SENDER_BUFFER_SIZE; i++) {
        struct pkt packet;
        packet.seqnum = 0;
        A.pkt_buffer[i] = packet;
    }
}

/**
 * Chamado quando o timer do remetente é estourado
 * 
 */
void A_timerinterrupt() {

}

/**
 * Recebe mensagem da camada aplicação
 * 
 * 1. Se o buffer já estiver cheio, descarta o pacote
 * 2. Adiciona o pacote no buffer
 * 3. Envia a janela para o meio
 */
void A_output(struct msg message) {
    if (A.pkt_buffer_current_size >= SENDER_BUFFER_SIZE) {
        printf("[A_output] Buffer cheio, descartando pacote\n");
        return;
    }

    printf("[A_output] Mensagem recebida, adicionando pacote ao buffer (pkt: %d, payload: %s)\n", A.next_seqnum, message.data);

    struct pkt packet;
    packet.seqnum = A.next_seqnum;
    
    for (int i = 0; i < 20; i++) {
        packet.payload[i] = message.data[i];
    }

    update_pkt_checksum(&packet);

    A.next_seqnum++;

    add_pkt_to_buffer(packet);

    send_window();
    // send_window(A.pkt_buffer[0].seqnum);
}

/**
 * Recebe pacote do meio
 * 
 * 1. Se o pacote está corrompido, descarta pacote
 * 2. Se o pacote é um NAK, reenvia janela a partir no pacote não recebido
 * 3. Desliza janela de envio.
 */
void A_input(struct pkt packet) {
    if (!is_valid_checksum(packet.checksum, &packet)) {
        printf("[A_input] Pacote recebido corrompido, descartando\n");
        return;
    }

    if (packet.acknum < 0) {
        int first_window_pkt_seqnum = abs(packet.acknum);

        printf("[A_input] NAK recebido, reenviando janela (pkt: %d)\n", first_window_pkt_seqnum);

        // send_window(first_window_pkt_seqnum);
        send_window();
        //reiniciar timer
        return;
    }

    printf("[A_input] ACK recebido, deslizando janela (pkt: %d)\n", packet.acknum);

    slide_window(packet.acknum);
    //ajustar timer
}

/***********************************************/
/*                   RECEPTOR                  */
/***********************************************/

/**
 * Inicializa receptor
 */
void B_init() {
    B.expected_seqnum = 1;
}

/**
 * Chamado quando o timer do receptor é estourado
 * (Não utilizado)
 */
void B_timerinterrupt() {
    printf("[B_timerinterrupt] Não implementado\n");
}

/**
 * Recebe mensagem da camada aplicação
 * (Não utilizado)
 */
void B_output(struct msg message) {
    printf("[B_output] Não implementado\n");
}

/**
 * Recebe pacote do meio
 * 
 * 1. Se o pacote está corrompido, manda NAK
 * 2. Se o pacote não é o esperado, manda NAK
 * 3. Manda mensagem para a camada de aplicação
 * 4. Manda ACK para o meio
 */
void B_input(struct pkt packet) {
    if(!is_valid_checksum(packet.checksum, &packet)) {
        printf("[B_input] Pacote recebido corrompido, mandando NAK (expected_seqnum: %d)\n", B.expected_seqnum);
        send_NAK(1, B.expected_seqnum);
        return;
    }

    if (packet.seqnum != B.expected_seqnum) {
        printf("[B_input] Pacote recebido fora de ordem, mandando NAK (pkt: %d, expected_seqnum: %d)\n", packet.seqnum, B.expected_seqnum);
        send_NAK(1, B.expected_seqnum);
        return;
    }

    printf("[B_input] Pacote recebido com sucesso (pkt: %d, payload: %s)\n", packet.seqnum, packet.payload);

    tolayer5(1, packet.payload);
    send_ACK(1, packet.seqnum);
    B.expected_seqnum++;

    printf("[B_input] Aguardando próximo pacote (pkt: %d)\n", B.expected_seqnum);
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