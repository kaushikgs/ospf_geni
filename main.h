/* 
 * File:   main.h
 * Author: kaushik
 *
 * Created on 19 April, 2015, 7:16 PM
 */

#ifndef MAIN_H
#define	MAIN_H

class Config_t{
public:
    int id;
    char infile[100];
    char outfile[100];
    int hi;
    int lsai;
    int spfi;
    int routers_no;
    int edges_no;
    int nibrs_no;
    
    Config_t(){
        hi = 1;
        lsai = 5;
        spfi = 20;
    }
};

class Neighbour{
public:
    int node_id;
    int act_cost;   //in milliseconds
    struct sockaddr *interface;
    
    bool reply_pending;
    std::clock_t start;
    
    static Neighbour* Get(int id);
};

void PrintGraph();
void PrintGraphFile (FILE *fp);

#define INT_BIG 100000

#endif	/* MAIN_H */

