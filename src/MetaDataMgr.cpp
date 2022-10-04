//
//  MetaDataMgr.cpp
//  spmetadata
//
//  Created by Vincent Moscaritolo on 10/4/22.
//

#include "MetaDataMgr.hpp"
#include <iostream>
#include <filesystem> // C++17

typedef void * (*THREADFUNCPTR)(void *);


MetaDataMgr::MetaDataMgr() {
	_isSetup = false;
 	_isRunning = true;

	pthread_create(&_TID, NULL,
										  (THREADFUNCPTR) &MetaDataMgr::MetaDataReaderThread, (void*)this);
 
}

MetaDataMgr::~MetaDataMgr(){
	stop();
	 
	_isRunning = false;
  	pthread_join(_TID, NULL);
 
}


bool MetaDataMgr::begin(const char* path){
	int error = 0;
	
	return begin(path, error);
}



bool MetaDataMgr::begin(const char* path,  int &error){

	if(isConnected())
		return true;
	 
	_metaDataFilePath = string(path);
	_isSetup = true;

	return true;
  }

void MetaDataMgr::stop(){
	
	if(_isSetup) {
 		_isSetup = false;
		}
}

 
bool  MetaDataMgr::isConnected() {
 
	return _ifs.is_open();
};

// MARK: -  MetaDataReader thread

void MetaDataMgr::MetaDataReader(){
 
	
	  while(_isRunning){
		   
		  // if not setup // check back later
		  if(!_isSetup){
			  sleep(2);
			  continue;
		  }
		
		  try{
			  
			  if(!_ifs.is_open()){
				  _ifs.open(_metaDataFilePath, ios::in);
				  
				  if(!_ifs.is_open()) {
					  continue;
				  }
			  }
			  
			  string line;
			  
			  while ( std::getline(_ifs, line) ) {
		//		  uint32_t type,code,length;

				  printf("%4lu:|%s|\n",line.size(), line.c_str());
//				  int ret = sscanf(line.c_str(),"<item><type>%8x</type><code>%8x</code><length>%u</length>",&type,&code,&length);
//					  if (ret==3) {
//
//					  }
				  if(_isSetup && _isRunning)
					  break;
			  }
	 
		  }
 		  catch(std::ifstream::failure &err) {
			  printf("MetaDataReader:FAIL: %s", err.what());
 		  }
		   
		  if(!_ifs.is_open()) {
			  _ifs.close();
		  }
	  }
	  
}



void* MetaDataMgr::MetaDataReaderThread(void *context){
	MetaDataMgr* d = (MetaDataMgr*)context;

	//   the pthread_cleanup_push needs to be balanced with pthread_cleanup_pop
	pthread_cleanup_push(   &MetaDataMgr::MetaDataReaderThreadCleanup ,context);
 
	d->MetaDataReader();
	
	pthread_exit(NULL);
	
	pthread_cleanup_pop(0);
	return((void *)1);
}

 
void MetaDataMgr::MetaDataReaderThreadCleanup(void *context){
	//MetaDataMgr* d = (MetaDataMgr*)context;
 
	printf("cleanup MetaDataReader\n");
}
 
