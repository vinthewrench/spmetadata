//
//  dbuf.c
//  spmetadata
//
//  Created by Vincent Moscaritolo on 10/4/22.
//

#include "dbuf.hpp"
#include <string.h>
#include <stdlib.h>

// MARK: -  Dbuf

#define ALLOC_QUANTUM 16


dbuf::dbuf(){
	_used = 0;
	_pos = 0;
	_alloc = ALLOC_QUANTUM;
	_data =  (uint8_t*) malloc(ALLOC_QUANTUM);
 }

dbuf::~dbuf(){
	if(_data)
		free(_data);
	_data = NULL;
	_used = 0;
	_pos = 0;
	_alloc = 0;
 }

bool dbuf::append_data(void* data, size_t len){
	
	if(len + _pos  >  _alloc){
		size_t newSize = len + _used + ALLOC_QUANTUM;
		
		if( (_data = (uint8_t*) realloc(_data,newSize)) == NULL)
			return false;
		
		_alloc = newSize;
	}
	
	 if(_pos < _used) {
		 memmove((void*) (_data + _pos + len) ,
					(_data + _pos),
					_used -_pos);
	 }
	 
	memcpy((void*) (_data + _pos), data, len);
	 
	 _pos += len;
	 _used += len;
 //	if(_pos > _used)
 //		_used = _pos;
	 
	return true;
}


bool dbuf::reserve(size_t len){
	
	if(len + _pos  >  _alloc){
		size_t newSize = len + _used + ALLOC_QUANTUM;
		
		if( (_data = (uint8_t*) realloc(_data,newSize)) == NULL)
			return false;
		
		_alloc = newSize;
	}

	if(_pos < _used) {
		memmove((void*) (_data + _pos + len) ,
				  (_data + _pos),
				  _used -_pos);
	}
	 
	return true;
}


uint16_t dbuf::calculateChecksum(){
 
	// The checksum is calculated over the message, starting and including the $,
	//but excluding, the checksum fields
	// The checksum algorithm used is the 8-bit Fletcher algorithm, which is used in the
	// TCP standard RFC 1145 .  https://www.ietf.org/rfc/rfc1145.txt
	uint8_t 	CK_A = 0;
	uint8_t 	CK_B = 0;
	for(size_t i = 0; i < _used;  i++){
		
		 CK_A += _data[i];
		 CK_B += CK_A;
	}
	
	return( (CK_A << 8 ) | CK_B);
}
