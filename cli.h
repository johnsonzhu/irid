#ifndef _CLI_H
#define _CLI_H

typedef struct cmd_ret_s cmd_ret_t;

struct cmd_ret_s
{
	char data[2048];
	int length;	
};

void cli(int argc,char **argv);
void usage(int argc,char **argv);

#endif
