#include <sys/types.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 16
#define QNAME "/my_queue"
#define PRIORITY 1
char recv_data[BUFSIZE];

int main(){
	mqd_t qd;
	struct mq_attr q_attr, old_q_attr;
	int prio;
	char buf[BUFSIZE];


	q_attr.mq_maxmsg = 10; /* max messagenumber in queue */
	q_attr.mq_msgsize = BUFSIZE;

	if((qd = mq_open(QNAME,O_RDWR | O_NONBLOCK,0600,NULL))==-1){
		perror("mq_open failed");
		exit(1);
	}
	q_attr.mq_flags = 0;
	if(mq_setattr(qd,&q_attr,NULL)){
		perror ("mq_setattr failed");
		exit(1);
	}

	if(mq_getattr(qd,&old_q_attr)){
		perror("mq_getattr failed");
		exit(1);
	}
	if (!(old_q_attr.mq_flags & O_NONBLOCK))
		printf("O_NONBLOCK not set\n");
	if (mq_receive(qd, recv_data, BUFSIZE, &prio) == -1) {
		perror ("mq_send failed");
		exit (1);
	}
	printf("received from message queue : %s, prio: %d\n",recv_data,prio);

	if(mq_close(qd) == -1){
		perror("mq_close failed");
		exit(1);
	}
	if (mq_unlink(QNAME) == -1) {
	perror ("mq_unlink failed");
	exit (1);
	}
}


