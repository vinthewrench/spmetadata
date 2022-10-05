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
#include <termios.h>


using namespace std;

class MetaDataMgr {
	
public:
 
	static MetaDataMgr *shared();

	MetaDataMgr();
	~MetaDataMgr();
	
	bool begin(const char* metapath = "/tmp/shairport-sync-metadata",
				const char* portpath = "/dev/ttyAMA0",
				  speed_t speed =  B115200);
	
	bool begin(const char* metapath, const char* portpath, speed_t speed, int &error);

	void stop();
	bool isConnected() ;
	bool isSetup() {return _isSetup;};

private:
	
	
	static MetaDataMgr *sharedInstance;

	bool 				_isSetup = false;

	bool openOutput(const char* portpath, speed_t speed, int &error);
	bool writePacket(const uint8_t * data, size_t len );
 
	void MetaDataReader();		// C++ version of thread
	// C wrappers for MetaDataReader;
	static void* MetaDataReaderThread(void *context);
	static void MetaDataReaderThreadCleanup(void *context);
	bool 			_isRunning = false;

	pthread_t				_TID;
 
	string 					_metaDataFilePath;
	std::ifstream			_ifs;

	
	int	 	_fd;
 	struct termios _tty_opts_backup;
};
