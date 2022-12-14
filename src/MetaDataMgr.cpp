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
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <algorithm>

#include <errno.h> // Error integer and strerror() function

#include "dbuf.hpp"

#define DEBUG_HEX 0

#if DEBUG_HEX

// for debugging
static void dumpHex(uint8_t* buffer, size_t length, int offset)
{
	char hexDigit[] = "0123456789ABCDEF";
	size_t			i;
	size_t						lineStart;
	size_t						lineLength;
	short					c;
	const unsigned char	  *bufferPtr = buffer;
	
	char                    lineBuf[1024];
	char                    *p;
	 
#define kLineSize	8
	for (lineStart = 0, p = lineBuf; lineStart < length; lineStart += lineLength,  p = lineBuf )
	{
		 lineLength = kLineSize;
		 if (lineStart + lineLength > length)
			  lineLength = length - lineStart;
		 
		p += sprintf(p, "%6lu: ", lineStart+offset);
		 for (i = 0; i < lineLength; i++){
			  *p++ = hexDigit[ bufferPtr[lineStart+i] >>4];
			  *p++ = hexDigit[ bufferPtr[lineStart+i] &0xF];
			  if((lineStart+i) &0x01)  *p++ = ' ';  ;
		 }
		 for (; i < kLineSize; i++)
			  p += sprintf(p, "   ");
		 
		 p += sprintf(p,"  ");
		 for (i = 0; i < lineLength; i++) {
			  c = bufferPtr[lineStart + i] & 0xFF;
			  if (c > ' ' && c < '~')
					*p++ = c ;
			  else {
					*p++ = '.';
			  }
		 }
		 *p++ = 0;
		 
  
		printf("%s\n",lineBuf);
	}
#undef kLineSize
}

#else
#define dumpHex(_arg1_ ,_arg2_ ,_arg3_ )
#endif
typedef void * (*THREADFUNCPTR)(void *);

typedef struct {
	uint32_t type;
	uint32_t code;
} filter_table_t;

static filter_table_t filter_table[] = {
	{'core', 'asal'}, // daap.songalbum
	{'core', 'asar'},	// daap.songartist
	{'core', 'minm'}, // dmap.itemname
//	{'core', 'asgn'}, //  daap.songgenre
//	{'core', 'ascp'}, //  daap.daap.songcomposer
//	{'core', 'asdk'}, //  daap.daap.songdatakind
 	{'core', 'caps'}, // play status  ( 01/ 02 )
	
	{'ssnc', 'mden'}, //  Metadata stream processing end
	{'ssnc', 'mdst'}, //  Metadata stream processing start
	 
	{'ssnc', 'aend'},	// airplay session begin
	{'ssnc', 'abeg'},	// airplay session end
	
	{'ssnc', 'pbeg'},	// play stream begin.
	{'ssnc', 'pend'}, // play stream end.

};

static bool sInFilterTable(uint32_t type, uint32_t code){
 
	for( int i = 0; i <  sizeof(filter_table)/ sizeof(filter_table_t); i++){
		if(filter_table[i].code == code && filter_table[i].type == type)
			return true;
	};
	
	return false;
}

MetaDataMgr *MetaDataMgr::sharedInstance = NULL;


MetaDataMgr * MetaDataMgr::shared() {
	if(!sharedInstance){
		sharedInstance = new MetaDataMgr;
	}
	return sharedInstance;
}
 

static void sigHandler (int signum) {
	
 	// ignore hangup
	if(signum == SIGHUP)
		return;
	
	auto mgr = MetaDataMgr::shared();
	mgr->stop();
	exit(0);
}


MetaDataMgr::MetaDataMgr() {
	
	signal(SIGKILL, sigHandler);
	signal(SIGHUP, sigHandler);
	signal(SIGQUIT, sigHandler);
	
	signal(SIGTERM, sigHandler);
	signal(SIGINT, sigHandler);

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


bool MetaDataMgr::begin(const char* metapath, const char* portpath, speed_t speed){
	int error = 0;
	
	return begin(metapath,portpath, speed, error);
}



bool MetaDataMgr::begin(const char* metapath, const char* portpath, speed_t speed,  int &error){
 
	if(isConnected())
		return true;
	 
	_metaDataFilePath = string(metapath);
	
	_isSetup  = openOutput(portpath, speed, error);
 
//	printf("Start MetaData reader  %s\n", _isSetup?"Success":"fail");
	
	
	return _isSetup;
  }

void MetaDataMgr::stop(){
	
	if(_isSetup && _fd > -1){
	
#if defined(__APPLE__)
#else
		// Restore previous TTY settings
		tcsetattr(_fd, TCSANOW, &_tty_opts_backup);
		
		ioctl(_fd, TIOCNXCL);
 
		close(_fd);
		_fd = -1;
#endif
	}
	
	_isSetup = false;
 
}

 

bool MetaDataMgr::openOutput(const char* path, speed_t speed, int &error){
 
#if defined(__APPLE__)
	_fd  = 1;
	return true;

 
#else
 	struct termios options;
	
	int fd ;
		
	if((fd = ::open( path, O_RDWR | O_NOCTTY)) <0) {
		fprintf (stderr, "FAIL open %s %s\n", path, strerror(errno));
		error = errno;
		return false;
	}
	
	// Block non-root users from using this port
	ioctl(_fd, TIOCEXCL);

	fcntl(fd, F_SETFL, 0);      // Clear the file status flags
 
	// Back up current TTY settings
	if( tcgetattr(fd, &_tty_opts_backup)<0) {
		fprintf (stderr, "FAIL tcgetattr %s %s\n", path, strerror(errno));
		error = errno;
		return false;
	}
	
	cfmakeraw(&options);
	options.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
	options.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
	options.c_cflag &= ~CSIZE; // Clear all bits that set the data size
	options.c_cflag |= CS8; // 8 bits per byte (most common)
	// options.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control 	options.c_cflag |=  CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
	options.c_cflag |=  CRTSCTS; // DCTS flow control of output
	options.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
	
	options.c_lflag &= ~ICANON;
	options.c_lflag &= ~ECHO; // Disable echo
	options.c_lflag &= ~ECHOE; // Disable erasure
	options.c_lflag &= ~ECHONL; // Disable new-line echo
	options.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
	options.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
	options.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes
	
	options.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
	options.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
	
//	options.c_ispeed=speed;
//	options.c_ospeed=speed;
 
  	if (cfsetspeed (&options, speed) < 0){
		fprintf (stderr, "FAIL cfsetospeed %d  %s\n", speed, strerror(errno));
		error = errno;
		return false;
	}
////
//// 	if (cfsetispeed (&options, speed) < 0){
////		fprintf (stderr, "FAIL cfsetispeed %d  %s\n", speed, strerror(errno));
////		error = errno;
////		return false;
////	}
//
//	cfsetospeed (&options, speed);
//	cfsetispeed (&options, speed);
	
	if (tcsetattr(fd, TCSANOW, &options) < 0){
		fprintf (stderr, "FAIL cfsetispeed %d  %s\n", speed, strerror(errno));
		error = errno;
		return false;
	}

//	printf("openOutput %s at speed %u \n", path, speed);

	_fd = fd;
	return true;
#endif
	
}

bool MetaDataMgr::writePacket(const uint8_t * data, size_t len ){
	
	bool success = false;

//	printf("send %2zu %.*s",  len, (int)len, data);

	
	dumpHex((uint8_t*) data, len,0);

#if  defined(__APPLE__)
	success = true;
//	printf("\n");
#else
  	success = (::write(_fd, data , len) == len);
	if(!success)
		fprintf (stderr, "\tFAIL write %d  %s\n", len, strerror(errno));

#endif
	
 
	return success;
}


bool  MetaDataMgr::isConnected() {
 
	return _ifs.is_open();
};

// MARK: -  MetaDataReader thread


inline  std::string trimCNTRL(std::string source) {
	  source.erase(std::find_if(source.rbegin(), source.rend(), [](char c) {
			return !std::iscntrl(static_cast<unsigned char>(c));
	  }).base(), source.end());
	  return source;
 }

inline size_t find_nth(const  std::string& haystack, size_t pos, const  std::string& needle, size_t nth)
{
	 size_t found_pos = haystack.find(needle, pos);
	 if(0 == nth ||  std::string::npos == found_pos)  return found_pos;
	 return find_nth(haystack, found_pos+1, needle, nth-1);
}


 
void MetaDataMgr::MetaDataReader(){
	
	dbuf outBuffer;
	
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
					sleep(1);
					continue;
				}
			}
			
			string line;
			
			while ( std::getline(_ifs, line) ) {
				
				uint32_t type, code;
				uint32_t length;
				
				int ret = sscanf(line.c_str(),"<item><type>%8x</type><code>%8x</code><length>%u</length>",&type,&code,&length);
				if (ret==3) {
					
					bool shouldProcessPacket  = sInFilterTable( type, code);
					char typestring[5] = {0};
					char codestring[5] = {0};
					string payload = "";
					
					*(uint32_t*)typestring = htonl(type);
					*(uint32_t*)codestring = htonl(code);
					
					if(length && std::getline(_ifs, line) ){
						if(line == "<data encoding=\"base64\">") {
							
							if(std::getline(_ifs, line) ){
								
								if(shouldProcessPacket){
									//								printf("processed %s %s \n",typestring, codestring);
									
									auto input_length = line.find("</data>");
									if(input_length != std::string::npos){
										
										payload = line.substr(0,input_length);
										payload = trimCNTRL(payload);
 									}
								}
 							}
						}
 					}
					
					// filter out only the packets I want
					if(shouldProcessPacket){
						outBuffer.reset();
						char header[16];
						sprintf( header, "$%s,%s,",typestring,codestring);
						outBuffer.append_data(header, strlen(header));
						outBuffer.append_data( (void*) payload.c_str(), payload.size());
						
						uint16_t checksum = outBuffer.calculateChecksum();
						sprintf( header, ",%hu\n",checksum);
						outBuffer.append_data(header, strlen(header));
						writePacket(outBuffer.data(), outBuffer.size());
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
 
