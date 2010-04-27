/*
$Id$
    OW -- One-Wire filesystem
    version 0.4 7/2/2003


    Written 2003 Paul H Alfille
        Fuse code based on "fusexmp" {GPL} by Miklos Szeredi, mszeredi@inf.bme.hu
        Serial code based on "xt" {GPL} by David Querbach, www.realtime.bc.ca
        in turn based on "miniterm" by Sven Goldt, goldt@math.tu.berlin.de
    GPL license
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    Other portions based on Dallas Semiconductor Public Domain Kit,
    ---------------------------------------------------------------------------
    Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
        Permission is hereby granted, free of charge, to any person obtaining a
        copy of this software and associated documentation files (the "Software"),
        to deal in the Software without restriction, including without limitation
        the rights to use, copy, modify, merge, publish, distribute, sublicense,
        and/or sell copies of the Software, and to permit persons to whom the
        Software is furnished to do so, subject to the following conditions:
        The above copyright notice and this permission notice shall be included
        in all copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
    OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.
        Except as contained in this notice, the name of Dallas Semiconductor
        shall not be used except as stated in the Dallas Semiconductor
        Branding Policy.
    ---------------------------------------------------------------------------
    Implementation:
    25-05-2003 iButtonLink device
*/


/* Cannot stand alone -- part of ow.h and only separated out for clarity */

#ifndef OW_MUTEXES_H			/* tedious wrapper */
#define OW_MUTEXES_H


#if OW_MT
#include <pthread.h>
#endif							/* OW_MT */

extern struct mutexes {
#if OW_MT
	pthread_mutex_t stat_mutex;
	pthread_mutex_t controlflags_mutex;
	pthread_mutex_t fstat_mutex;
	pthread_mutex_t simul_mutex;
	pthread_mutex_t dir_mutex;
	pthread_mutex_t libusb_mutex;
	pthread_mutex_t typedir_mutex;
	pthread_mutex_t namefind_mutex;
	pthread_mutex_t aliasfind_mutex;
	
	pthread_mutexattr_t *pmattr;
	pthread_mutexattr_t mattr;
	my_rwlock_t lib;
	my_rwlock_t cache;
	my_rwlock_t store;
  #ifdef __UCLIBC__
	pthread_mutex_t uclibc_mutex;
  #endif							/* __UCLIBC__ */
#endif							/* OW_MT */
} Mutex;



#if OW_MT
#define MUTEX_INIT(mut)	 	my_pthread_mutex_init(    &(mut) , &(Mutex.mattr) )
#define MUTEX_DESTROY(mut)	my_pthread_mutex_destroy( &(mut) )

#define MUTEX_LOCK(mut)		my_pthread_mutex_lock(   &(mut) )
#define MUTEX_UNLOCK(mut)	my_pthread_mutex_unlock( &(mut) )

#define RWLOCK_INIT(rw)		my_rwlock_init(    &(rw) )
#define RWLOCK_DESTROY(rw)	my_rwlock_destroy( &(rw) )

#define RWLOCK_WLOCK(mut)	my_rwlock_write_lock(   &(mut) )
#define RWLOCK_WUNLOCK(mut)	my_rwlock_write_unlock( &(mut) )

#define RWLOCK_RLOCK(mut)	my_rwlock_read_lock(   &(mut) )
#define RWLOCK_RUNLOCK(mut)	my_rwlock_read_unlock( &(mut) )

#define LIB_WLOCK         	RWLOCK_WLOCK(   Mutex.lib    ) ;
#define LIB_WUNLOCK       	RWLOCK_WUNLOCK( Mutex.lib    ) ;
#define LIB_RLOCK         	RWLOCK_RLOCK(   Mutex.lib    ) ;
#define LIB_RUNLOCK       	RWLOCK_RUNLOCK( Mutex.lib    ) ;

#define CACHE_WLOCK       	RWLOCK_WLOCK(   Mutex.cache  ) ;
#define CACHE_WUNLOCK     	RWLOCK_WUNLOCK( Mutex.cache  ) ;
#define CACHE_RLOCK       	RWLOCK_RLOCK(   Mutex.cache  ) ;
#define CACHE_RUNLOCK     	RWLOCK_RUNLOCK( Mutex.cache  ) ;

#define STORE_WLOCK       	RWLOCK_WLOCK(   Mutex.store ) ;
#define STORE_WUNLOCK     	RWLOCK_WUNLOCK( Mutex.store ) ;
#define STORE_RLOCK       	RWLOCK_RLOCK(   Mutex.store ) ;
#define STORE_RUNLOCK     	RWLOCK_RUNLOCK( Mutex.store ) ;

#define CONNIN_WLOCK      	RWLOCK_WLOCK(   Inbound_Control.lock ) ;
#define CONNIN_WUNLOCK    	RWLOCK_WUNLOCK( Inbound_Control.lock ) ;
#define CONNIN_RLOCK      	RWLOCK_RLOCK(   Inbound_Control.lock ) ;
#define CONNIN_RUNLOCK    	RWLOCK_RUNLOCK( Inbound_Control.lock ) ;

#define STATLOCK          	MUTEX_LOCK(  Mutex.stat_mutex   )
#define STATUNLOCK        	MUTEX_UNLOCK(Mutex.stat_mutex   )

#define CONTROLFLAGSLOCK  	MUTEX_LOCK(  Mutex.controlflags_mutex  )
#define CONTROLFLAGSUNLOCK	MUTEX_UNLOCK(Mutex.controlflags_mutex  )

#define FSTATLOCK         	MUTEX_LOCK(  Mutex.fstat_mutex  )
#define FSTATUNLOCK       	MUTEX_UNLOCK(Mutex.fstat_mutex  )

#define SIMULLOCK         	MUTEX_LOCK(  Mutex.simul_mutex  )
#define SIMULUNLOCK       	MUTEX_UNLOCK(Mutex.simul_mutex  )

#define DIRLOCK           	MUTEX_LOCK(  Mutex.dir_mutex    )
#define DIRUNLOCK         	MUTEX_UNLOCK(Mutex.dir_mutex    )

#define LIBUSBLOCK        	MUTEX_LOCK(  Mutex.libusb_mutex )
#define LIBUSBUNLOCK      	MUTEX_UNLOCK(Mutex.libusb_mutex )

#define TYPEDIRLOCK       	MUTEX_LOCK(  Mutex.typedir_mutex)
#define TYPEDIRUNLOCK     	MUTEX_UNLOCK(Mutex.typedir_mutex)

#define NAMEFINDLOCK      	MUTEX_LOCK(  Mutex.namefind_mutex)
#define NAMEFINDUNLOCK    	MUTEX_UNLOCK(Mutex.namefind_mutex)

#define ALIASFINDLOCK     	MUTEX_LOCK(  Mutex.aliasfind_mutex)
#define ALIASFINDUNLOCK   	MUTEX_UNLOCK(Mutex.aliasfind_mutex)

#define BUSLOCK(pn)       	BUS_lock(pn)
#define BUSUNLOCK(pn)     	BUS_unlock(pn)
#define BUSLOCKIN(in)     	BUS_lock_in(in)
#define BUSUNLOCKIN(in)   	BUS_unlock_in(in)
#ifdef __UCLIBC__
#define UCLIBCLOCK        	MUTEX_LOCK(  Mutex.uclibc_mutex)
#define UCLIBCUNLOCK      	MUTEX_UNLOCK(Mutex.uclibc_mutex)
#else							/* __UCLIBC__ */
#define UCLIBCLOCK		return_ok()
#define UCLIBCUNLOCK		return_ok()
#endif							/* __UCLIBC__ */

#else							/* OW_MT */
#define LIB_WLOCK		return_ok()
#define LIB_WUNLOCK		return_ok()
#define LIB_RLOCK		return_ok()
#define LIB_RUNLOCK		return_ok()

#define CACHE_WLOCK		return_ok()
#define CACHE_WUNLOCK		return_ok()
#define CACHE_RLOCK		return_ok()
#define CACHE_RUNLOCK		return_ok()

#define STORE_WLOCK		return_ok()
#define STORE_WUNLOCK		return_ok()
#define STORE_RLOCK		return_ok()
#define STORE_RUNLOCK		return_ok()

#define CONNIN_WLOCK		return_ok()
#define CONNIN_WUNLOCK		return_ok()
#define CONNIN_RLOCK		return_ok()
#define CONNIN_RUNLOCK		return_ok()

#define STATLOCK		return_ok()
#define STATUNLOCK		return_ok()

#define CONTROLFLAGSLOCK	return_ok()
#define CONTROLFLAGSUNLOCK	return_ok()

#define FSTATLOCK		return_ok()
#define FSTATUNLOCK		return_ok()

#define SIMULLOCK		return_ok()
#define SIMULUNLOCK		return_ok()

#define DIRLOCK			return_ok()
#define DIRUNLOCK		return_ok()

#define UCLIBCLOCK		return_ok()
#define UCLIBCUNLOCK		return_ok()

#define LIBUSBLOCK		return_ok()
#define LIBUSBUNLOCK		return_ok()

#define TYPEDIRBLOCK		return_ok()
#define TYPEDIRUNLOCK		return_ok()

#define NAMEFINDLOCK		return_ok()
#define NAMEFINDUNLOCK		return_ok()

#define ALIASFINDLOCK		return_ok()
#define ALIASFINDUNLOCK		return_ok()

#define UCLIBCLOCK		return_ok()
#define UCLIBCUNLOCK		return_ok()
#define BUSLOCK(pn)		return_ok()
#define BUSUNLOCK(pn)		return_ok()
#define BUSLOCKIN(in)		return_ok()
#define BUSUNLOCKIN(in)		return_ok()
#endif							/* OW_MT */

#endif							/* OW_MUTEXES_H */
