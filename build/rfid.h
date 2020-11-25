/*
 * rfid.h
 *
 *  Created on: Jan 11, 2018
 *      Author: winstonlee
 */

#ifndef RFID_H_
#define RFID_H_


#include <hiredis.h>
#include <async.h>
//#include <adapters/libevent.h>


#define LINUX_DEVICE 	"/dev/ttyRFID"
#define BAUD_RATE 		115200
#define VTIME			0
#define VMIN			1


//define variable


//define function
int 			open_serial				( char *dev_name, int baud, int vtime, int vmin );
void 			close_serial			( int fd );
int				rfid_version_check		( void );

void 			*redis_Verify		( void *arg );
//static 			void onMessage			(redisAsyncContext *c, void *reply, void *privdata);
redisContext* 	redisInitialize			( void ) ;

#endif /* RFID_H_ */
