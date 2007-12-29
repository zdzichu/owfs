/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: palfille@earthlink.net
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"
#include <assert.h>

struct transaction_bundle {
    const struct transaction_log *start ;
    int packets ;
    size_t max_size ;
    struct memblob mb ;
    int select_first ;
} ;

// static int BUS_transaction_length( const struct transaction_log * tl, const struct parsedname * pn ) ;
static int BUS_transaction_single(const struct transaction_log *t,
                           const struct parsedname *pn) ;

static int Bundle_pack( const struct transaction_log * tl, const struct parsedname * pn ) ;
static int Pack_item( const struct transaction_log * tl, struct transaction_bundle * tb ) ;
static int Bundle_ship( struct transaction_bundle * tb, const struct parsedname * pn ) ;
static int Bundle_enroute( struct transaction_bundle * tb, const struct parsedname * pn ) ;
static int Bundle_unpack( struct transaction_bundle * tb ) ;

static void Bundle_init( struct transaction_bundle * tb, const struct parsedname * pn ) ;

#define TRANSACTION_INCREMENT 1000

/* Bus transaction */
/* Encapsulates communication with a device, including locking the bus, reset and selection */
/* Then a series of bytes is sent and returned, including sending data and reading the return data */
int BUS_transaction(const struct transaction_log *tl,
					const struct parsedname *pn)
{
	int ret = 0;

	if (tl == NULL)
		return 0;

	BUSLOCK(pn);
	ret = BUS_transaction_nolock(tl, pn);
	BUSUNLOCK(pn);

	return ret;
}
int BUS_transaction_nolock(const struct transaction_log *tl,
                           const struct parsedname *pn)
{
    const struct transaction_log *t = tl;
    int ret = 0;

	if ( pn->selected_connection->iroutines.flags & ADAP_FLAG_bundle ) {
		return Bundle_pack( tl, pn ) ;
	}

    do {
        //printf("Transact type=%d\n",t->type) ;
        ret = BUS_transaction_single(t,pn) ;
        if ( ret == -ESRCH ) { // trxn_done flag
            ret = 0 ; // restore no error code
            break ; // but stop looping anyways
        }
        ++t;
    } while (ret == 0);
    return ret;
}

static int BUS_transaction_single(const struct transaction_log *t,
                           const struct parsedname *pn)
{
    int ret = 0;
    switch (t->type) {
        case trxn_select: // select a 1-wire device (by unique ID)
            ret = BUS_select(pn);
            LEVEL_DEBUG("  Transaction select = %d\n", ret);
            break;
        case trxn_compare: // match two strings -- no actual 1-wire
            if ( (t->in==NULL) || (t->out==NULL) || (memcmp(t->in,t->out,t->size)!=0) ) {
                ret = -EINVAL ;
            }
            LEVEL_DEBUG("  Transaction compare = %d\n", ret);
            break ;
        case trxn_match: // send data and compare response
            assert( t->in == NULL ) ; // else use trxn_compare
            if (t->size == 0) { /* ignore for both cases */
                break;
            } else {
                ret = BUS_send_data(t->out, t->size, pn);
                LEVEL_DEBUG("  Transaction send = %d\n", ret);
            }
            break;
        case trxn_read:
            assert( t->out == NULL ) ; // pure read
            if (t->size == 0) { /* ignore for all cases */
                break;
            } else if (t->out == NULL) {
                ret = BUS_readin_data(t->in, t->size, pn);
                LEVEL_DEBUG("  Transaction readin = %d\n", ret);
            }
            break;
        case trxn_modify: // write data and read response. No match needed
            ret = BUS_sendback_data(t->out, t->in, t->size, pn);
            LEVEL_DEBUG("  Transaction modify = %d\n", ret);
            break ;
        case trxn_blind: // write data ignore response
        {
            BYTE * dummy = malloc( t->size ) ;
            if ( dummy != NULL ) {
                ret = BUS_sendback_data(t->out, dummy, t->size, pn);
                free( dummy) ;
            } else {
                ret = -ENOMEM ;
            }
        }
            LEVEL_DEBUG("  Transaction blind = %d\n", ret);
            break ;
        case trxn_power:
            ret = BUS_PowerByte(t->out[0], t->in, t->size, pn);
            LEVEL_DEBUG("  Transaction power (%d usec) = %d\n", t->size, ret);
            break;
        case trxn_program:
            ret = BUS_ProgramPulse(pn);
            LEVEL_DEBUG("  Transaction program pulse = %d\n", ret);
            break;
        case trxn_crc8:
            ret = CRC8(t->out, t->size);
            LEVEL_DEBUG("  Transaction CRC8 = %d\n", ret);
            break;
        case trxn_crc8seeded:
            ret = CRC8seeded(t->out, t->size, ((UINT *) (t->in))[0]);
            LEVEL_DEBUG("  Transaction CRC8 = %d\n", ret);
            break;
        case trxn_crc16:
            ret = CRC16(t->out, t->size);
            LEVEL_DEBUG("  Transaction CRC16 = %d\n", ret);
            break;
        case trxn_crc16seeded:
            ret = CRC16seeded(t->out, t->size, ((UINT *) (t->in))[0]);
            LEVEL_DEBUG("  Transaction CRC16 = %d\n", ret);
            break;
        case trxn_delay:
            if ( t->size > 0 ) {
                UT_delay(t->size);
            }
            LEVEL_DEBUG("  Transaction Delay %d\n", t->size);
            break;
        case trxn_udelay:
            if ( t->size > 0 ) {
                UT_delay_us(t->size);
            }
            LEVEL_DEBUG("  Transaction Micro Delay %d\n", t->size);
            break;
        case trxn_reset:
            ret = BUS_reset(pn);
            LEVEL_DEBUG("  Transaction reset = %d\n", ret);
            break ;
        case trxn_end:
            LEVEL_DEBUG("  Transaction end = %d\n", ret);
            return -ESRCH ; // special "end" flag
        case trxn_verify:
        {
            struct parsedname pn2;
            memcpy(&pn2, pn, sizeof(struct parsedname));    //shallow copy
            pn2.selected_device = NULL;
            ret = BUS_select(&pn2) || BUS_verify(t->size, pn);
            LEVEL_DEBUG("  Transaction verify = %d\n", ret);
        }
        break;
        case trxn_nop:
            ret = 0;
            break;
    }
    return ret;
}

// initialize the bundle
static void Bundle_init( struct transaction_bundle * tb, const struct parsedname * pn )
{
    memset( tb, 0, sizeof( struct transaction_bundle ) ) ;
    MemblobInit( &tb->mb, TRANSACTION_INCREMENT ) ;
    tb->max_size = pn->selected_connection->bundling_length ;
}

static int Bundle_pack( const struct transaction_log * tl, const struct parsedname * pn )
{
    struct transaction_log * t_index ;
    struct transaction_bundle s_tb ;
    struct transaction_bundle * tb = &s_tb ;

	LEVEL_DEBUG("Transaction Bundle: Start\n") ; 

    Bundle_init( tb, pn ) ;

    for ( t_index = tl ; t_index->type != trxn_end ; ++t_index ) {
        switch ( Pack_item( t_index, tb ) ) {
            case 0:
	LEVEL_DEBUG("Transaction Bundle: Item added\n") ; 
                break ;
            case -EINVAL:
	LEVEL_DEBUG("Transaction Bundle: Item cannot be bundled\n") ; 
                if ( Bundle_ship( tb, pn ) ) return -EINVAL ;
                if ( BUS_transaction_single( t_index, pn ) ) return -EINVAL ;
                break ;
            case -EAGAIN:
	LEVEL_DEBUG("Transaction Bundle: Item too big\n") ; 
                if ( Bundle_ship( tb, pn ) ) return -EINVAL ;
                if ( Pack_item( t_index, tb ) == 0 ) break ;
                if ( BUS_transaction_single( t_index, pn ) ) return -EINVAL ;
                break ;
        }
    }
    return Bundle_ship( tb, pn ) ;
}

// Take a bundle, execute the transaction, unpack, and clear the memoblob
static int Bundle_ship( struct transaction_bundle * tb, const struct parsedname * pn )
{
	LEVEL_DEBUG("Transaction Bundle: Ship Packets=%d\n",tb->packets) ; 
    if ( tb->packets == 0 ) {
        return 0 ;
    }
    
    if ( Bundle_enroute( tb, pn ) != 0 ) {
        // clear the bundle
        MemblobClear( &tb->mb) ;
        tb->packets = 0 ;
        tb->select_first = 0 ;
        return -EINVAL ;
    }

    return Bundle_unpack( tb ) ;
}

// Execute a bundle transaction (actual bytes on 1-wire bus)
static int Bundle_enroute( struct transaction_bundle * tb, const struct parsedname * pn )
{
    if ( tb->select_first ) {
        return BUS_select_and_sendback( tb->mb.memory_storage, tb->mb.memory_storage, tb->mb.used, pn ) ;
    } else {
        return BUS_sendback_data(  tb->mb.memory_storage, tb->mb.memory_storage, tb->mb.used, pn ) ;
    }
}

/* See if the item can be packed
   return -EINVAL -- cannot be packed at all
   return -EAGAIN -- should be at start
   return -EINTR  -- should be at end (force end)
   return 0       -- added successfully
*/
static int Pack_item( const struct transaction_log * tl, struct transaction_bundle * tb )
{ 
    int ret = 0 ; //default return value for good packets;
    switch (tl->type) {
        case trxn_select: // select a 1-wire device (by unique ID)
		LEVEL_DEBUG("Transaction Bundle: pack=SELECT\n") ;
            if (tb->packets != 0 ) return -EAGAIN ; // select must be first
            tb->select_first = 1 ;
            break;
        case trxn_compare: // match two strings -- no actual 1-wire
		LEVEL_DEBUG("Transaction Bundle: pack=COMPARE\n") ;
            break;
        case trxn_read:
		LEVEL_DEBUG("Transaction Bundle: pack=READ\n") ;
            if ( tl->size > tb->max_size ) return -EINVAL ; // too big for any bundle
            if ( tl->size + tb->mb.used > tb->max_size ) return -EAGAIN ; // too big for this partial bundle
            if ( MemblobChar( 0xFF, tl->size, &tb->mb ) ) return -EINVAL ;
            break;
        case trxn_match: // write data and match response
        case trxn_modify: // write data and read response. No match needed
        case trxn_blind: // write data and ignore response
		LEVEL_DEBUG("Transaction Bundle: pack=MATCH MODIFY BLIND\n") ;
            if ( tl->size > tb->max_size ) return -EINVAL ; // too big for any bundle
            if ( tl->size + tb->mb.used > tb->max_size ) return -EAGAIN ; // too big for this partial bundle
            if ( MemblobAdd( tl->out, tl->size, &tb->mb ) ) return -EINVAL ;
            break;
        case trxn_power:
        case trxn_program:
		LEVEL_DEBUG("Transaction Bundle: pack=POWER PROGRAM\n") ;
            if ( 1 > tb->max_size ) return -EINVAL ; // too big for any bundle
            if ( 1 + tb->mb.used > tb->max_size ) return -EAGAIN ; // too big for this partial bundle
            if ( MemblobAdd( tl->out, 1, &tb->mb ) ) return -EINVAL ;
            ret = -EINTR ; // needs delay
            break;
        case trxn_crc8:
        case trxn_crc8seeded:
        case trxn_crc16:
        case trxn_crc16seeded:
		LEVEL_DEBUG("Transaction Bundle: pack=CRC*\n") ;
            break ;
        case trxn_delay:
        case trxn_udelay:
		LEVEL_DEBUG("Transaction Bundle: pack=(U)DELAYS\n") ;
            ret = -EINTR ;
            break;
        case trxn_reset:
        case trxn_end:
        case trxn_verify:
		LEVEL_DEBUG("Transaction Bundle: pack=RESET END VERIFY\n") ;
            return -EINVAL ;
        case trxn_nop:
		LEVEL_DEBUG("Transaction Bundle: pack=NOP\n") ;
            break;
    }
    if ( tb->packets == 0 ) {
        tb->start = tl ;
    }
    ++ tb->packets ;
    return ret ;
}

static int Bundle_unpack( struct transaction_bundle * tb )
{
    int packet_index ;
    const struct transaction_log * tl ;
    BYTE * data = tb->mb.memory_storage ;
    int ret = 0 ;

	LEVEL_DEBUG("Transaction Bundle: unpacking\n") ; 

    for ( packet_index = 0, tl = tb->start ; packet_index < tb->packets ; ++packet_index, ++tl ) {
        switch (tl->type) {
            case trxn_compare: // match two strings -- no actual 1-wire
	LEVEL_DEBUG("Transaction Bundle: unpacking #%d COMPARE\n",packet_index) ; 
                if ( (tl->in==NULL) || (tl->out==NULL) || (memcmp(tl->in,tl->out,tl->size)!=0) ) {
                        ret = -EINVAL ;
                    }
                break ;
            case trxn_match: // send data and compare response
	LEVEL_DEBUG("Transaction Bundle: unpacking #%d MATCH\n",packet_index) ; 
                if ( memcmp( tl->out, data, tl->size) != 0 ) {
                    ret = -EINVAL ;
                }
                data += tl->size ;
                break;
            case trxn_read:
            case trxn_modify: // write data and read response. No match needed
	LEVEL_DEBUG("Transaction Bundle: unpacking #%d READ MODIFY\n",packet_index) ; 
                memmove(tl->in, data, tl->size);
                data += tl->size ;
                break;
            case trxn_blind:
	LEVEL_DEBUG("Transaction Bundle: unpacking #%d BLIND\n",packet_index) ; 
                data += tl->size ;
                break;
            case trxn_power:
            case trxn_program:
	LEVEL_DEBUG("Transaction Bundle: unpacking #%d POWER PROGRAM\n",packet_index) ; 
                memmove(tl->in, data, 1);
                data += 1 ;
                UT_delay( tl->size ) ;
                break;
            case trxn_crc8:
	LEVEL_DEBUG("Transaction Bundle: unpacking #%d CRC8\n",packet_index) ; 
                if ( CRC8(tl->out, tl->size) != 0 ) {
                    ret = -EINVAL ;
                }
                break;
            case trxn_crc8seeded:
	LEVEL_DEBUG("Transaction Bundle: unpacking #%d CRC8 SEEDED\n",packet_index) ; 
                if ( CRC8seeded(tl->out, tl->size, ((UINT *) (tl->in))[0]) != 0 ) {
                    ret = -EINVAL ;
                }
                break;
            case trxn_crc16:
	LEVEL_DEBUG("Transaction Bundle: unpacking #%d CRC16\n",packet_index) ; 
                if ( CRC16(tl->out, tl->size) != 0 ) {
                    ret = -EINVAL ;
                }
                break;
            case trxn_crc16seeded:
	LEVEL_DEBUG("Transaction Bundle: unpacking #%d CRC16 SEEDED\n",packet_index) ; 
                if ( CRC16seeded(tl->out, tl->size, ((UINT *) (tl->in))[0]) != 0 ) {
                    ret = -EINVAL ;
                }
                break;
            case trxn_delay:
	LEVEL_DEBUG("Transaction Bundle: unpacking #%d DELAY\n",packet_index) ; 
                UT_delay(tl->size);
                break;
            case trxn_udelay:
	LEVEL_DEBUG("Transaction Bundle: unpacking #%d UDELAY\n",packet_index) ; 
                UT_delay_us(tl->size);
                break;
            case trxn_reset:
            case trxn_end:
            case trxn_verify:
                // should never get here
	LEVEL_DEBUG("Transaction Bundle: unpacking #%d RESET END VERIFY\n",packet_index) ; 
                ret = -EINVAL ;
                break ;
            case trxn_nop:
            case trxn_select:
	LEVEL_DEBUG("Transaction Bundle: unpacking #%d NOP or SELECT\n",packet_index) ; 
                break;
        }
        if ( ret != 0 ) {
            break ;
        }
    }
    
    // clear the bundle
    MemblobClear( &tb->mb) ;
    tb->packets = 0 ;
    tb->select_first = 0 ;
    
    return ret ;
}
