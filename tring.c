/* gcc -pedantic -Wall tring.c -o tring */

/* global includes */

#include <stdio.h>
#include <mqueue.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <sys/msg.h>
#include <errno.h>
#include <time.h>
#include <signal.h>


/* local includes */
#include "struct.h"

/* prototypes */
/* shm */
int	create_shm(void);
int	free_shm(int);
int	place_msg(int, struct s_msg);
int	get_msg(int, struct s_msg *);
int	del_msg(int);

/* mq */
int	q_create(char *);
int	q_connect(char *);
int	q_disconnect(int);
int	q_destroy(int);
int	q_write(int, struct s_jeton);
int	q_read(int, struct s_jeton *);
int 	getrand_msg(struct s_msg *, const char *);

/* ... */
void	sig_kill(int);
void 	install_kill(void);
int	station(char *, char *, int, int, int);

/* used by handler for clean */
int q_my, q_peer;

/* defines */
#define PERMS 0666


/* main function
 */
int main (int argc, char **argv)
{
	int 	sid = create_shm();
	int 	i, pid;
	char 	name[9] = "STATION1";
	char	peer[9] = "STATION2";
	int	pids[4];
	int 	p;
	
	q_my = -1;
	q_peer = -1;
	
	install_kill();
	
	if (argc != 2 || (p = atoi(argv[1])) <= 0)
	{
		printf("[syntax] %s <p>\n\tp: probability of loosing token,\n\t1=100%% and 100=1%%\n", argv[0]);
		return -1;
	}

	for (i=0;i<4;i++)
	{
		pid = fork();
		if (pid) /* dad' */
		{
			name[7]++;
			peer[7]++;
			if (peer[7] == '5') peer[7] = '1';
			pids[i] = pid;
			continue;
		}
		else /* luke */
		{
			exit(station(name, peer, (name[7] == '1')?1:0, sid, p));
		}
		
	}

	for (i=0;i<4;i++)
		waitpid(pids[i], NULL, 0);
	
	printf("[main] all childs exited, deleting SHM...\n");
	free_shm(sid);
	return 0;
}

/* install SIGTERM
 * handler for cleaning up
 * all things..
 */
void install_kill(void)
{
	struct sigaction sig;
	sig.sa_handler = sig_kill;
	sigaction(SIGTERM, &sig, NULL);
	return;
}

/* create shared memory used
 * for message being transmitted.
 */
int	create_shm(void)
{
	return shmget(IPC_PRIVATE,sizeof(struct s_msg),PERMS);
}


/* destroy shared memory
 */
int	free_shm(int id)
{
	return shmctl(id, IPC_RMID,(struct shmid_ds *)0);
}

/* place message on shared memory
 */
int	place_msg(int sid, struct s_msg msg)
{
	char *addr;
	
	addr = (char *)shmat(sid, NULL, PERMS);
	
	if (addr == (char*)-1) 
		return -1;
	
	memcpy(addr, &msg, sizeof(struct s_msg));
	
	return shmdt(addr);
}

/* get message on shared memory
 */
int	get_msg(int sid, struct s_msg *msg)
{
	char *addr;
	
	addr = (char *)shmat(sid,(char *)0,PERMS);
	
	if (addr == (char*)-1) 
		return -1;
	
	memcpy(msg, addr, sizeof(struct s_msg));
	
	return shmdt(addr);
}

/* delete message on the shared memory
 */
int	del_msg(int sid)
{
	char *addr;
	
	addr = (char *)shmat(sid,(char *)0,PERMS);
	
	if (addr == (char*)-1) 
		return -1;
	
	memset(addr, 0, sizeof(struct s_msg));
	
	return shmdt(addr);
}



/* connect to message queue 'n'.
 */
int	q_connect(char *n)
{
	int msgflg = 0;
	
	key_t key = atoi(n+7);
	
	return msgget(key, msgflg);

	return -1;
}


/* create a message queue named
 * 'n'.
 */
int	q_create(char *n)
{
	int msgflg = IPC_CREAT | 0666;
	
	key_t key = atoi(n+7);
	
	return msgget(key, msgflg);
}


/* disconnect from message queue
 * id number 'id'.
 */
int	q_disconnect(int id)
{
	/* unused */
	return -1;
}

/* disconnect and destroy message
 * queue id number 'id'.
 */
int	q_destroy(int id)
{
	msgctl(id, IPC_RMID, NULL);
	return -1;
}

/* write message on message queue
 * with descriptor number 'id'.
 */
int	q_write(int id, struct s_jeton msg)
{
	struct msgbuf sbuf;
	sbuf.mtype = 1;
	memcpy(sbuf.mtext, &msg, sizeof(struct s_jeton));
	sbuf.mtext[sizeof(struct s_jeton)] = 0;
	return msgsnd(id, &sbuf, sizeof(struct s_jeton) + 1, IPC_NOWAIT);
}

/* read message on message queue
 * with descriptor number 'id'.
 */
int	q_read(int id, struct s_jeton *msg)
{
	struct msgbuf rbuf;
	int ret;

	if ((ret = msgrcv(id, &rbuf, 1024, 1, IPC_NOWAIT)) == -1) 
	{
		if (errno == ENOMSG) 
			return 0; 
		else
			return -1;
	}

	memcpy(msg, rbuf.mtext, sizeof(struct s_jeton));
	return ret;
}

/*
 * generate a random message with a random
 * destination but different from source
 * of course..
 */
int 	getrand_msg(struct s_msg *msg, const char *nom)
{
	int n = atoi(nom+7);
	static int last = -1;
	if (last == -1) last = time(NULL);
	do
	{
		srandom(last);
		last = random();

	} while (((last % 4) + 1) == n);
	
	msg->data = last;
	strncpy(msg->src, nom, strlen(nom)+1);
	sprintf(msg->dst, "STATION%d", (last % 4) + 1);
	return last;
}


/* Handler for SIGKILL signal,
 * destroy message queue and exit.
 */
void sig_kill(int s)
{
	if (q_peer != -1 && q_my != -1)
	{
		printf("Exiting one station\n");
		q_disconnect(q_peer);
		q_destroy(q_my);
		exit(0);
	}
	return;
}


/* launch station's process, named
 * with 'nom', who'll connect to message
 * queue named 'peer'. if station is master,
 * it will emit the first token.
 */
int	station(char *nom, char *peer, int is_master, int sid, int p)
{
	int i, last_jeton = 0, ret;
	struct s_jeton last, jeton;
	struct s_msg message;
	int	count = 0;
	
	install_kill();
	
	last.id = 0;

	printf("[%s] starting with pid %d...\n", nom, getpid());
	printf("[%s] master=%d, sid=%d\n", nom, is_master, sid);

	/* create message queue */
	if ((q_my = q_create(nom)) == -1)
	{
		printf("[%s] cannot create message queue\n", nom);
		return -1;
	}
	
	for (i=0;i<3;i++)
	{
		printf("[%s] connecting to %s peer...(%d try)\n", nom, peer, i+1);
		q_peer = q_connect(peer);
		if (q_peer > 0) break;
		else sleep(1);
	}
	if (q_peer == -1)
	{
		printf("[%s] cannot connect, exiting ...\n", nom);
	}
	printf("[%s] connected to peer %s(%d)...\n", nom, peer, q_peer);
	
	while(42)
	{
		sleep(1); /* one second by action */
		if ((ret = q_read(q_my, &jeton)) > 0) /* we have a jeton */
		{
			if (jeton.id < last.id) /* old jeton, don't propage it... */
			{
				continue;
			}
			else if (jeton.id > last.id) /* we have a new jeton \o/ \\o o// don't use it now */
			{
				last_jeton = 0;
				last = jeton;
				printf("[%s] I got a new token, don't use it now\n", nom);
				q_write(q_peer, jeton);
				continue;
			}
			count++;
			last_jeton = 0;
			if (count == p) 
			{
				count = 0;
				last = jeton;
				printf("[%s] probability of %d reached, loosing this token...\n", nom, p);
				continue;
			}
			printf("[%s] I got a token ;p\n", nom);

			/* now we can deal with shm, we have the ring dude, erm no, the token... ! \o/ */
			if (get_msg(sid, &message) == -1) /* error reading... */
			{
				printf("[%s] We have an error while reading shm !!\n", nom);
			}
			else
			{
				if (!strcmp(message.dst, nom)) /* it's for me !! */
				{
					printf("[%s] Received message for me: %d\n", nom, message.data);
					del_msg(sid);
				}
				else if (!strcmp(message.src, nom))
				{
					printf("[%s] OoOoPs, it seems that we have a loop in here...\n", nom);
			/*		del_msg(sid); */
				}
				else if (!strcmp(message.dst, ""))
				{
					/* we should send a message here... */
					getrand_msg(&message, nom);
					place_msg(sid, message);
					printf("[%s] I've send a message to %s with data %d\n", nom, message.dst, message.data);
				}
				q_write(q_peer, jeton);
			}
			
		}
		else if (ret == 0) /* nothing here */
		{
			last_jeton++;
			if (last_jeton >= 5 && is_master) /* generate new token */
			{
				last_jeton = 0;
				jeton.id = last.id+1;
				last.id++;
				if (q_write(q_peer, jeton) == -1)
					printf("[%s] Error sending new token..\n", nom);
				else
					printf("[%s] New token generated and launched in the wild...\n", nom);
			}
		}
		else
		{
			printf("[%s] Error while reading trough message queue...\n", nom);
		}
		printf("[%s] End of turn... sleeping 1 sec..\n", nom);	
	}
}


