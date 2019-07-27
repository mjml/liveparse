#include "editor.hpp"
#include "log.hpp"
#include "docreg.hpp"
#include "memsys.hpp"
#include "mque.hpp"


int main (int argc, char *argv[])
{
	
	// all initialization
	try {
		
		log::initialize();
		
		MQue::initialize();

		MemSys::initialize();
		
		DocReg::initialize();
		
		
		
	} catch (const std::exception e) {
		
		return 1;
	}


	//// completion of initialization



	//// completion of normal operation


	//// completetion of finalization
	
	
	return 0;
}
