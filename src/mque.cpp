#include <string>
#include <cassert>
#include "mque.hpp"

namespace MQue
{

int mqsockfd = 0;


void initialize ()
{
	
	assert(mqsockfd == 0);
	mqsockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
	assert(mqsockfd != -1);

	
	
}


void finalize ()
{

}


void send (proc_t to, const Message& msg)
{
	
}


void poll ()
{

}


void dispatch (const Message& msg)
{
	
	
}



}
