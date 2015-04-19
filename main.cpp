/* 
 * File:   main.cpp
 * Author: kaushik
 *
 * Created on 18 April, 2015, 7:02 PM
 */

#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <limits.h>
#include <ctime>

#include "main.h"
Config_t config;
#include "packets.h"
#include "hello.h"
#include "lsa.h"
#include "ospf.h"

using namespace std;

#define MAX_PACKET_SIZE 1024

void Initialize();
void ParseCmd(int argc, char **argv);
void ParseFile();
struct sockaddr* SetInterface(int nid);
void PrintNibrInfo();
void Recv();
void TestOSPF();


Neighbour **nibr_list;
int sock;
int **costs;
int time_initial;

pthread_t hello_tr;
pthread_t lsa_tr;
pthread_t ospf_tr;

pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t costs_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t actcosts_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t timer_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * 
 */
int main(int argc, char** argv) {
    time_initial = time(NULL);
    srand( time_initial);
    ParseCmd(argc, argv);
    Initialize();
    ParseFile();
    
    //PrintNibrInfo();    //to debug input parsing
    
    pthread_create(&hello_tr, NULL, SendHello, NULL);
    pthread_create(&lsa_tr, NULL, SendLSA, NULL);
    //TestOSPF();
    
    pthread_create(&ospf_tr, NULL, MainOSPF, NULL);
    Recv();
    
    //while(1);
    return 0;
}

void Initialize(){
    struct sockaddr_in my_addr;
    
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(20053);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(my_addr.sin_zero), 8);

    if (bind(sock, (struct sockaddr *) &my_addr,
            sizeof (struct sockaddr)) == -1) {
        perror("Bind");
        exit(1);
    }
}

void ParseCmd(int argc, char **argv){
    int c;
    char gen[10];
    
    while ((c = getopt(argc, argv, "i:f:o:h:a:s:")) != -1) {
        switch(c) {
            case 'i': config.id = atoi(optarg); break;
            case 'f': strcpy(config.infile, optarg); break;
            case 'o': strcpy(config.outfile, optarg); break;
            case 'h': config.hi = atoi(optarg); break;
            case 'a': config.lsai = atoi(optarg); break;
            case 's': config.spfi = atoi(optarg); break;
        }
    }
    
    strcat( config.outfile, "-");
    sprintf(gen, "%d.txt", config.id );
    strcat( config.outfile, gen);
}

void ParseFile(){
    FILE *ifp;
    ifp = fopen(config.infile, "r");
    fscanf(ifp, "%d", &config.routers_no);
    fscanf(ifp, "%d", &config.edges_no);
    
    costs = (int**) malloc( sizeof(int*) * config.routers_no);
    for(int i=0; i<config.routers_no; i++){
        costs[i] = (int*) malloc( sizeof(int) * config.routers_no);
        for( int j=0; j<config.routers_no; j++)
            costs[i][j] = INT_BIG;
    }
    
    nibr_list = (Neighbour**) malloc( sizeof(Neighbour*) * config.routers_no);
    int nid1;
    int nid2;
    config.nibrs_no = 0;
    
    for(int i=0; i<config.edges_no; i++){
        
        fscanf(ifp,"%d", &nid1);
        if (feof(ifp))  break;
        fscanf(ifp,"%d", &nid2);
        
        if( nid1==config.id){
            nibr_list[config.nibrs_no] = (Neighbour*) malloc( sizeof(Neighbour));
            nibr_list[config.nibrs_no]->node_id = nid2;
            nibr_list[config.nibrs_no]->interface = SetInterface(nid2);
            nibr_list[config.nibrs_no]->act_cost = INT_BIG;
            config.nibrs_no++;
            
        }
        
        else if( nid2==config.id){
            nibr_list[config.nibrs_no] = (Neighbour*) malloc( sizeof(Neighbour));
            nibr_list[config.nibrs_no]->node_id = nid1;
            nibr_list[config.nibrs_no]->interface = SetInterface(nid1);
            nibr_list[config.nibrs_no]->act_cost = INT_BIG;
            config.nibrs_no++;
        }
    }
    
    fclose(ifp);        
}

struct sockaddr* SetInterface(int nid){
    struct sockaddr_in *new_addr = new sockaddr_in;
    struct hostent *host;
    char hostname_str[10];
    char gen[3];
    sprintf(gen, "%d", nid);
    strcpy( hostname_str, "node-");
    strcat( hostname_str, gen);
    
    host = (struct hostent *) /*gethostbyname("localhost");*/ gethostbyname(hostname_str);
    new_addr->sin_family = AF_INET;
    new_addr->sin_port = htons(20053);
    new_addr->sin_addr = *((struct in_addr *) host->h_addr);
    bzero(&(new_addr->sin_zero), 8);
    
    return (sockaddr*) new_addr;
}

Neighbour* Neighbour::Get(int id){
    for(int i=0; i<config.nibrs_no; i++){
        if(id==nibr_list[i]->node_id)
            return nibr_list[i];
    }
    return NULL;
}

void Recv(){
    char pkt[MAX_PACKET_SIZE];
    strcpy(pkt, "");
    int src_node;
    struct sockaddr_in *src_addr = new sockaddr_in;
    socklen_t sock_size;
    int recv_size;
    
    while(1){
        recv_size = recvfrom(sock, pkt, MAX_PACKET_SIZE, 0, (struct sockaddr*) src_addr, &sock_size);
        pkt[recv_size] = '\0';
        
        /*src_node = ntohs(src_addr->sin_port)-20000;
        pthread_mutex_lock(&print_lock);
        printf("\nNode no. %d said %s\n", src_node, pkt);
        pthread_mutex_unlock(&print_lock);*/
        
        if (strncmp(pkt, "HELLOREPLY", 10) == 0){
            RecvHelloReply(pkt);
        }
        else if (strncmp(pkt, "HELLO", 5) == 0){
            RecvHello(pkt);
        }
        else {
            RecvLSA((LSAPacket*) pkt);
        }
        
        strcpy(pkt, "");
    }
}

//----Debug functions----//
void PrintNibrInfo(){
    printf("NodeID\tMin\tMax\n");
    for(int i=0; i<config.nibrs_no; i++){
        printf("%d\n", nibr_list[i]->node_id);
    }
}

void PrintGraph(){
    for(int i=0; i<config.routers_no; i++){
        for(int j=0; j<config.routers_no; j++){
            printf("%d\t", costs[i][j]);
        }
        printf("\n");
    }
}

void TestOSPF(){
    char filename[100];
    
    printf("Enter OSPF test file name: ");
    scanf("%s", filename);
    FILE *fp;
    fp = fopen( filename, "r");
    
    for (int i=0; i<config.routers_no; i++){
        for(int j=0; j<config.routers_no; j++){
            fscanf(fp, "%d", &costs[i][j]);
        }
    }
    
    fclose(fp);
}