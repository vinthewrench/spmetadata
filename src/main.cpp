//
//  main.cpp
//  spmetadata
//
//  Created by Vincent Moscaritolo on 10/4/22.
//

#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <exception>
#include <stdexcept>

#include "CommonDefs.hpp"
#include "MetaDataMgr.hpp"


#if defined(__APPLE__)
const char* path_metadata  = "shairport-sync-metadata";
const char* path_port  = "stdout";

#else
const char* path_metadata  = "/tmp/shairport-sync-metadata";
const char* path_port  = "/dev/serial0";
#endif


int main(int argc, const char * argv[]) {
 
	MetaDataMgr* mgr 	= MetaDataMgr::shared();

	 

	try {
		int error = 0;

		
		if(!mgr->begin(path_metadata, path_port, B38400, error))
			throw Exception("failed to setup MetaData Mgr\n", error);
 
		while(mgr->isSetup()){

			sleep(100);
		}
		mgr->stop();

	}
	catch ( const Exception& e)  {
		
		// display error on fail..
		
		printf("\tError %d %s\n\n", e.getErrorNumber(), e.what());
	}
	catch (std::invalid_argument& e)
	{
		// display error on fail..
		
		printf("EXCEPTION: %s ",e.what() );
	}
	

	return 0;
}
