#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctime>

#define MAX_PACKET_SIZE 1024
#define MAX_DIGITS 35

#define mysend(x,y) sendto(sock, x, strlen(x), 0, \
            Neighbour::Get(y)->interface, sizeof (struct sockaddr));

extern int sock;
#include "main.h"
extern Config_t config;
#include "packets.h"
#include "lsa.h"

extern Neighbour **nibr_list;

extern pthread_mutex_t print_lock;
extern pthread_mutex_t costs_lock;
extern pthread_mutex_t actcosts_lock;

extern int **costs;

int next_seq_no=0;
int *lsnos;

void *SendLSA(void *unnecessary){
    LSAPacket *pkt = new LSAPacket;
    int bytes_sent;
    
    strcpy(pkt->lsastring, "LSA");
    pkt->srcid = config.id;
    pkt->no_entries = config.nibrs_no;
    pkt->sender = config.id;
    
    while(1){
        pkt->seqno = next_seq_no;
        next_seq_no++;
        
        /*printf("Graph from sender..\n");
        PrintGraph();*/
                
        for(int i=0; i<config.nibrs_no; i++){
            pkt->nibrs[i] = nibr_list[i]->node_id;
            pthread_mutex_lock(&actcosts_lock);
            pkt->costs[i] = nibr_list[i]->act_cost;
            pthread_mutex_unlock(&actcosts_lock);
        }

        for(int i=0; i<config.nibrs_no; i++){
            bytes_sent = sendto(sock, pkt, sizeof(LSAPacket), 0, nibr_list[i]->interface, sizeof (struct sockaddr));
        }
        
        if(bytes_sent == -1){
            fprintf(stderr, "Did not send LSA\n");
        }
        
        /*pthread_mutex_lock(&print_lock);
        printf("Broadcasting LSA \n");    fflush(stdout);
        pthread_mutex_unlock(&print_lock);*/
        sleep(config.lsai);
    }
}

void RecvLSA(LSAPacket *pkt){
    static bool firstime=true;
    int nid;
    
    if(firstime == true){
        lsnos = (int*) malloc( sizeof(int)*config.routers_no);
        for(int i=0; i<config.routers_no; i++){
            lsnos[i] = -1;
        }
        firstime = false;
    }
    
    if(lsnos[pkt->srcid]==pkt->seqno) return;
    else lsnos[pkt->srcid] = pkt->seqno;
    
    pthread_mutex_lock(&costs_lock);
    for(int i=0; i<pkt->no_entries; i++){
        nid = pkt->nibrs[i];
        costs[pkt->srcid][nid] = pkt->costs[i];
    }
    /*printf("Graph from receiver..\n");
    PrintGraph();*/
    pthread_mutex_unlock(&costs_lock);
       
    FrwdLSA(pkt, pkt->sender);
}

void FrwdLSA(LSAPacket *pkt, int sendr){
    pkt->sender = config.id;
    for( int i=0; i<config.nibrs_no; i++){
        if(nibr_list[i]->node_id==sendr)    continue;
        sendto(sock, pkt, sizeof(LSAPacket), 0, nibr_list[i]->interface, sizeof (struct sockaddr));
    }
}
