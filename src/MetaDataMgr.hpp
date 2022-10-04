//
//  MetaDataMgr.hpp
//  spmetadata
//
//  Created by Vincent Moscaritolo on 10/4/22.
//

#pragma once
 
#include <mutex>
#include <utility>      // std::pair, std::make_pair
#include <string>       // std::string
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <pthread.h>
#include <utility>      // std::pair, std::make_pair
#include <fstream>


using namespace std;

class MetaDataMgr {
	
public:

	MetaDataMgr();
	~MetaDataMgr();
	
	bool begin(const char* path = "/tmp/shairport-sync-metadata");
	bool begin(const char* path, int &error);

	void stop();
	bool isConnected() ;

private:
	
	bool 				_isSetup = false;

	
	void MetaDataReader();		// C++ version of thread
	// C wrappers for MetaDataReader;
	static void* MetaDataReaderThread(void *context);
	static void MetaDataReaderThreadCleanup(void *context);
	bool 			_isRunning = false;

	pthread_t				_TID;
 
	string 					_metaDataFilePath;
	std::ifstream			_ifs;

};
