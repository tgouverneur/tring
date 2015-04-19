#ifndef STRUCT_H
#define STRUCT_H


#define MSGMNB 1024

/* jeton , this structure size
 * should be less than MSGMNB */
struct s_jeton
{
	int id;
};

/* message */
struct s_msg
{
	/* header */
	char 	src[9];
	char	dst[9];

	/* data */
	int	data;
};

/* structure used for message queuing */
struct msgbuf 
{
	long mtype;     /* message type, must be > 0 */
	char mtext[1024];  /* message data */
};


#endif
