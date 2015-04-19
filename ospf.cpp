#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <ctime>

using namespace std;

#include "ospf.h"
#include "main.h"

extern Neighbour** nibr_list;
extern Config_t config;
extern int **costs;
extern pthread_mutex_t costs_lock;
extern int time_initial;
extern pthread_mutex_t print_lock;

class RoutingTableRow{
public:
    int dest;
    vector<int> path;
    int cost;
    bool visited;
};

class RoutingTable{
public:
    int curr_time;
    int nid;
    
    RoutingTableRow *rows;
    
    RoutingTable(){
        nid = config.id;
        rows = (RoutingTableRow*) malloc(sizeof(RoutingTableRow)*config.routers_no);
        for( int i=0; i<config.routers_no; i++){
            rows[i].dest = i;
            rows[i].path.push_back(config.id);
            
            if(i==config.id)    rows[i].cost = 0;
            else                rows[i].cost = INT_BIG;
            /*else{
                pthread_mutex_lock(&costs_lock);
                rows[i].cost = costs[nid][i];
                pthread_mutex_unlock(&costs_lock);
            }*/
            rows[i].visited = false;
        }
    }
    
    int GetMinUnvisited(){
        int min = INT_BIG;
        int min_node = 0;
        int count = 0;
        
        for(int i=0; i<config.routers_no; i++){
            if(rows[i].cost < min && rows[i].cost!=0 && rows[i].visited == false){
                min = rows[i].cost;
                min_node = i;
                count++;
            }
        }
        
        if (count == 0) return -1;
        else    return min_node;
    }
    
    void Print(int);
    void PrintScreen(int);
    void Clear();
};

vector<int> GetNibrs(int nid);

void *MainOSPF(void*){
    RoutingTable *tabl;
    int curr_node;
    bool done = false;
    int new_dist;
    vector<int> nibr_vect;
    int time_now;
    tabl = new RoutingTable();
    
    while(1){
        
        //PrintGraphFile();
        sleep(config.spfi);
        done = false;
        
        curr_node = config.id;
        
        //pthread_mutex_lock(&costs_lock);
        while(!done){
            nibr_vect = GetNibrs(curr_node);
            
            pthread_mutex_lock(&costs_lock);
            for(vector<int>::iterator it = nibr_vect.begin(); it != nibr_vect.end(); ++it){
                
                new_dist = tabl->rows[curr_node].cost + costs[curr_node][*it];
                //pthread_mutex_unlock(&costs_lock);
                
                if(tabl->rows[*it].cost > new_dist){
                    tabl->rows[*it].cost = new_dist;
                    tabl->rows[*it].path.clear();
                    tabl->rows[*it].path = tabl->rows[curr_node].path;
                    tabl->rows[*it].path.push_back(*it);
                }
            }
            pthread_mutex_unlock(&costs_lock);
            
            tabl->rows[curr_node].visited = true;
            curr_node = tabl->GetMinUnvisited();
            if( curr_node==-1)  done = true;
        }
        //pthread_mutex_unlock(&costs_lock);
        
        time_now = time(NULL);
        tabl->Print(time_now-time_initial);
        tabl->PrintScreen(time_now-time_initial);
        tabl->Clear();
        
    }
}

vector<int> GetNibrs(int nid){
    vector<int> res;
    
    for(int i=0; i<config.routers_no; i++){
        pthread_mutex_lock(&costs_lock);
        if( costs[nid][i]!=INT_BIG){
            res.push_back(i);
        }
        pthread_mutex_unlock(&costs_lock);
    }
    
    return res;
}

void RoutingTable::Print(int time_tp){
    FILE *fp;
    fp = fopen(config.outfile, "a");
    
    fprintf(fp, "\nRouting Table for node no. %d at time %d\n", config.id, time_tp);
    fprintf(fp, "Destination\tPath\tCost\n");
    
    for(int i=0; i<config.routers_no; i++){
        fprintf(fp, "%d\t", rows[i].dest);
        for( vector<int>::iterator it = rows[i].path.begin(); it != rows[i].path.end(); ++it)
            fprintf(fp, "%d-", *it);
        fprintf(fp, "\t");
        fprintf(fp, "%d\n", rows[i].cost);
    }
    
    fclose(fp);
}

void RoutingTable::PrintScreen(int time_tp){
    
    pthread_mutex_lock(&print_lock);
    printf( "\nRouting Table for node no. %d at time %d\n", config.id, time_tp);
    printf( "Destination\tPath\tCost\n");
    
    for(int i=0; i<config.routers_no; i++){
        printf( "%d\t", rows[i].dest);
        for( vector<int>::iterator it = rows[i].path.begin(); it != rows[i].path.end(); ++it)
            printf( "%d-", *it);
        printf( "\t");
        printf( "%d\n", rows[i].cost);
    }
    pthread_mutex_unlock(&print_lock);
}

void RoutingTable::Clear(){
    for( int i=0; i<config.routers_no; i++){
            rows[i].dest = i;
            rows[i].path.clear();
            rows[i].path.push_back(config.id);
            
            if(i==config.id)    rows[i].cost = 0;
            else                rows[i].cost = INT_BIG;
            /*else{
                pthread_mutex_lock(&costs_lock);
                rows[i].cost = costs[nid][i];
                pthread_mutex_unlock(&costs_lock);
            }*/
            rows[i].visited = false;
    }
}
