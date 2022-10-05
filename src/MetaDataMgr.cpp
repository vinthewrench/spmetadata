//
//  MetaDataMgr.cpp
//  spmetadata
//
//  Created by Vincent Moscaritolo on 10/4/22.
//

#include "MetaDataMgr.hpp"
#include <iostream>
#include <filesystem> // C++17
#include <arpa/inet.h>
 
typedef void * (*THREADFUNCPTR)(void *);

typedef struct {
	uint32_t type;
	uint32_t code;
} filter_table_t;

static filter_table_t filter_table[] = {
	{'core', 'asal'}, // daap.songalbum
	{'core', 'asar'},	// daap.songartist
	{'core', 'minm'}, // dmap.itemname
	{'core', 'caps'} // play status  ( 01/ 02 )
};

static bool sInFilterTable(uint32_t type, uint32_t code){
 
	for( int i = 0; i <  sizeof(filter_table)/ sizeof(filter_table_t); i++){
		if(filter_table[i].code == code && filter_table[i].type == type)
			return true;
	};
	
	return false;
}

 


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
				  
				  uint32_t type, code;
				  uint32_t length;
				  
				  int ret = sscanf(line.c_str(),"<item><type>%8x</type><code>%8x</code><length>%u</length>",&type,&code,&length);
				  if (ret==3) {
					  if (length>0) {
						  
						  if(std::getline(_ifs, line) ){
							  if(line == "<data encoding=\"base64\">") {
								  
								  if(std::getline(_ifs, line) ){
									  
									  // filter out for only the packets I want..
									  if(sInFilterTable( type, code)){
	 
										  auto input_length = line.find("</data>");
										  if(input_length != std::string::npos){
											  
											  string payload = line.substr(0,input_length);
											 
											  char typestring[5];
											  *(uint32_t*)typestring = htonl(type);
											  typestring[4]=0;
											  char codestring[5];
											  *(uint32_t*)codestring = htonl(code);
											  codestring[4]=0;

											  printf("%s,%s,%zu,%s\n",typestring,codestring, input_length, payload.c_str());
											  
										  }
										  
									  }
								  }
							  }
						  }
								  
					  }
				  }
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
 
