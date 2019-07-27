#pragma once

#include <sys/types.h>
#include <sys/socket.h>


namespace MQue
{

typedef uint32_t proc_t;

struct Message
{
	proc_t  from_pid;
	uint8_t   code;
	uint32_t  size;
	uint8_t   data[];
};

void initialize ();

void finalize ();

void send (proc_t to, const Message& msg);

void poll ();

void dispatch (const Message& msg);
	
};
