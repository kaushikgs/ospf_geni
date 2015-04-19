/* 
 * File:   lsa.h
 * Author: kaushik
 *
 * Created on 19 April, 2015, 7:15 PM
 */

#ifndef LSA_H
#define	LSA_H

void *SendLSA(void*);
void RecvLSA(LSAPacket*);
void FrwdLSA(LSAPacket*, int);

class LastSeqBuf{
public:
    int *nids;
    int *seq_no;
};

#endif	/* LSA_H */

