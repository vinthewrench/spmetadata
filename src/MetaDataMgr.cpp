//
//  MetaDataMgr.cpp
//  spmetadata
//
//  Created by Vincent Moscaritolo on 10/4/22.
//

#include "MetaDataMgr.hpp"
#include <iostream>
#include <filesystem> // C++17
#include "dbuf.hpp"

typedef void * (*THREADFUNCPTR)(void *);


// From Stack Overflow, with thanks:
// http://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c
// minor mods to make independent of C99.
// more significant changes make it not malloc memory
// needs to initialise the docoding table first

static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
										  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
										  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
										  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
										  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
										  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
										  'w', 'x', 'y', 'z', '0', '1', '2', '3',
										  '4', '5', '6', '7', '8', '9', '+', '/'};
static int decoding_table[256]; // an incoming char can range over ASCII, but by mistake could be all 8 bits.

 
static void initialise_decoding_table() {
	int i;
 for (i = 0; i < 64; i++)
	  decoding_table[(unsigned char) encoding_table[i]] = i;
}

// pass in a pointer to the data, its length, a pointer to the output buffer and a pointer to an int containing its maximum length
// the actual length will be returned.
static int base64_decode(const char *data,
									  size_t input_length,
									  unsigned char *decoded_data,
									  size_t *output_length) {

	 //remember somewhere to call initialise_decoding_table();

	 if (input_length % 4 != 0) return -1;

	 size_t calculated_output_length = input_length / 4 * 3;
	 if (data[input_length - 1] == '=') calculated_output_length--;
	 if (data[input_length - 2] == '=') calculated_output_length--;
	 if (calculated_output_length> *output_length)
		return(-1);
	 *output_length = calculated_output_length;

	 int i,j;
	 for (i = 0, j = 0; i < input_length;) {

		  uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
		  uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
		  uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
		  uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

		  uint32_t triple = (sextet_a << 3 * 6)
		  + (sextet_b << 2 * 6)
		  + (sextet_c << 1 * 6)
		  + (sextet_d << 0 * 6);

		  if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
		  if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
		  if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
	 }

	 return 0;
}


MetaDataMgr::MetaDataMgr() {
	_isSetup = false;
 	_isRunning = true;

	initialise_decoding_table();
	
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

	typedef 	union
	{
		uint32_t u32;
		uint8_t bytes[4];
 	} u32_bytes_t;
	
	  while(_isRunning){
		   
		  // if not setup // check back later
		  if(!_isSetup){
			  sleep(2);
			  continue;
		  }
		
		  try{
			  
			  dbuf   buff;
			  
			  if(!_ifs.is_open()){
				  _ifs.open(_metaDataFilePath, ios::in);
				  
				  if(!_ifs.is_open()) {
					  continue;
				  }
			  }
			  
			  string line;
			  
			  while ( std::getline(_ifs, line) ) {
				  
				  u32_bytes_t type, code;
				  uint32_t length;
				  
				  printf("%4lu:|%s|\n",line.size(), line.c_str());
				  int ret = sscanf(line.c_str(),"<item><type>%8x</type><code>%8x</code><length>%u</length>",&type.u32,&code.u32,&length);
				  if (ret==3) {
					  if (length>0) {
						  
						  if(std::getline(_ifs, line) ){
							  if(line == "<data encoding=\"base64\">") {
								  
								  if(std::getline(_ifs, line) ){
									  size_t outputlength=0;
									  
									  auto b64Len = line.find("</data>");
									  if(b64Len != std::string::npos){
										  
										  buff.reset();
										  buff.reserve(length);
				
										  if (base64_decode(line.c_str(), b64Len, buff.data(),&outputlength)!=0) {
											  printf("Failed to decode it.\n");
											  continue;
										  }
										  // process actual data  (length bytes of base64)			  }
				
										  printf("%4s %4s %zu\n " , type.bytes, code.bytes, outputlength );
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
 