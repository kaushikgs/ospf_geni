#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctime>

extern int sock;
#include "main.h"
extern Config_t config;
#include "packets.h"
#include "hello.h"

extern Neighbour **nibr_list;

extern pthread_mutex_t timer_lock;
extern pthread_mutex_t print_lock;
extern pthread_mutex_t costs_lock;
extern pthread_mutex_t actcosts_lock;
extern int **costs;

void *SendHello(void* unnecessary){
    char *pkt;
    size_t pkt_size = 5*sizeof(char)+sizeof(int);
    pkt = (char*) malloc(pkt_size);
    strncpy(pkt, "HELLO", 5);
    memcpy((void*) (pkt+5*sizeof(char)), (void*) &config.id, sizeof(int));
    
    while(1){
        for(int i=0; i<config.nibrs_no; i++){
            sendto(sock, (char *) pkt, pkt_size, 0,
            nibr_list[i]->interface, sizeof (struct sockaddr));
            pthread_mutex_lock(&timer_lock);
            nibr_list[i]->start = std::clock();
            nibr_list[i]->reply_pending = true;
            pthread_mutex_unlock(&timer_lock);
        }
        sleep(config.hi);
    }
}

int RecvHello(char *pkt){
    int nid;
    
    memcpy((void*) &nid, (void*) (pkt+5*sizeof(char)), sizeof(int));
    SendHelloReply(nid);
    
    /*pthread_mutex_lock(&print_lock);
    printf("Received hello packet from %d\n", nid);
    pthread_mutex_unlock(&print_lock);*/
}

void SendHelloReply(int nid){
    size_t pkt_size = 10*sizeof(char)+2*sizeof(int);
    char *pkt = (char*) malloc( pkt_size);
    
    strncpy(pkt, "HELLOREPLY", 10);
    memcpy((void*)(pkt+10*sizeof(char)), (void*) &config.id, sizeof(int));
    memcpy((void*)(pkt+10*sizeof(char)+sizeof(int)), (void*) &nid, sizeof(int));
    
    sendto(sock, (char *) pkt, pkt_size, 0,
            Neighbour::Get(nid)->interface, sizeof (struct sockaddr));
    
}

int RecvHelloReply(char *pkt){
    int recvr_id = *(pkt+10*sizeof(char));
    int cost_ret;
    double duration;
    
    pthread_mutex_lock(&timer_lock);
    duration = (std::clock() - Neighbour::Get(recvr_id)->start) / (double) CLOCKS_PER_SEC;
    Neighbour::Get(recvr_id)->reply_pending = false;
    pthread_mutex_unlock(&timer_lock);
    cost_ret = 1000000 * (double) duration; //to get microseconds
    
    /*pthread_mutex_lock(&print_lock);
    printf("Received hello reply from %d with cost %d\n", recvr_id, cost_ret);
    pthread_mutex_unlock(&print_lock);*/
    
    pthread_mutex_lock(&costs_lock);
    costs[config.id][recvr_id] = cost_ret;
    pthread_mutex_unlock(&costs_lock);
    
    pthread_mutex_lock(&actcosts_lock);
    Neighbour::Get(recvr_id)->act_cost = cost_ret;
    pthread_mutex_unlock(&actcosts_lock);
}
