#include <iostream>
#include "editor.hpp"
#include "util/log.hpp"
#include "docreg.hpp"
#include "memsys.hpp"
#include "mque.hpp"

const char* appName = "liveparse";

int main (int argc, char *argv[])
{
	
	// all initialization
	//try {
		
		log::initialize();
		
		MQue::initialize();

		MemSys::initialize();
		
		DocReg::initialize();
		
		
		
		//} catch (const std::exception e) {
		//std::cout << e.what() << std::endl;
		//return 1;
		//}


	//// completion of initialization



	//// completion of normal operation


	//// completetion of finalization
	
	
	return 0;
}
