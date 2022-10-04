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

 

int main(int argc, const char * argv[]) {
 
	MetaDataMgr _mgr;
	
	printf("Start MetaData reader\n");
	
	
	try {
		int error = 0;

		
		if(!_mgr.begin("/tmp/shairport-sync-metadata", error))
			throw Exception("failed to setup MetaData Mgr.  error: %d", error);

		
		while(true){

			sleep(100);
		}
		_mgr.stop();

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
