/* 
 * File:   hello.h
 * Author: kaushik
 *
 * Created on 19 April, 2015, 7:13 PM
 */

#ifndef HELLO_H
#define	HELLO_H

void *SendHello(void*);
int RecvHello(char*);
void SendHelloReply(int nid);
int RecvHelloReply(char*);

#endif	/* HELLO_H */

