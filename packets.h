/* 
 * File:   packets.h
 * Author: kaushik
 *
 * Created on 19 April, 2015, 7:16 PM
 */

#ifndef PACKETS_H
#define	PACKETS_H

#define MAX_NEIGHBOURS 100

struct LSAPacket{
public:
    char lsastring[3];
    int srcid;
    int seqno;
    int no_entries;
    int nibrs[MAX_NEIGHBOURS];
    int costs[MAX_NEIGHBOURS];
    int sender;
};

#endif	/* PACKETS_H */

