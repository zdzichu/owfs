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

// static int BUS_transaction_length( const struct transaction_log * tl, const struct parsedname * pn ) ;


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

	do {
		//printf("Transact type=%d\n",t->type) ;
		switch (t->type) {
		case trxn_select:
			ret = BUS_select(pn);
			LEVEL_DEBUG("  Transaction select = %d\n", ret);
			break;
		case trxn_match:
			if (t->size == 0) {	/* ignore for both cases */
				break;
			} else if (t->in) {	/* just compare out and in */
				ret = memcmp(t->out, t->in, t->size);
				LEVEL_DEBUG("  Transaction match = %d\n", ret);
			} else {
				ret = BUS_send_data(t->out, t->size, pn);
				LEVEL_DEBUG("  Transaction send = %d\n", ret);
			}
			break;
		case trxn_read:
			if (t->size == 0) {	/* ignore for all cases */
				break;
			} else if (t->out == NULL) {
				ret = BUS_readin_data(t->in, t->size, pn);
				LEVEL_DEBUG("  Transaction readin = %d\n", ret);
			} else if (t->in == NULL) {
				BYTE dummy[64];
				size_t s = t->size;
				while (s > 0) {
					size_t ss = (s > 64) ? 64 : s;
					s -= ss;
					ret = BUS_sendback_data(t->out, dummy, ss, pn);
					LEVEL_DEBUG
						("  Transaction null sendback (%lu of %lu)= %d\n",
						 ss, t->size, ret);
					if (ret)
						break;
				}
			} else {
				ret = BUS_sendback_data(t->out, t->in, t->size, pn);
				LEVEL_DEBUG("  Transaction sendback = %d\n", ret);
			}
			break;
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
			return 0;
		case trxn_end:
			LEVEL_DEBUG("  Transaction end = %d\n", ret);
			return 0;
		case trxn_verify:
			{
				struct parsedname pn2;
				memcpy(&pn2, pn, sizeof(struct parsedname));	//shallow copy
				pn2.selected_device = NULL;
				ret = BUS_select(&pn2) || BUS_verify(t->size, pn);
				LEVEL_DEBUG("  Transaction verify = %d\n", ret);
			}
			break;
		case trxn_nop:
			ret = 0;
			break;
		}
		++t;
	} while (ret == 0);
	return ret;
}

#if 0
/* start of bundled transactions

struct transaction_bundle {
    const struct transaction_log *start ;
    int packets ;
    size_t bytes ;
    int select_first ;
} ;

void bundle_init( struct transaction_bundle * tb )
{
    tb->start = NULL ,
    tb->packets = 0 ;
    tb->bytes = 0 ;
    select_first = 0 ;
}

int bundle_add( const struct transaction_log * tl, struct transaction_bundle * tb )
{
    if ( tb->start == NULL ) {
        tb->start = tl ;
    }
    switch (tl->type) {
        case trxn_select:
            select_first = 1 ;
            break;
        case trxn_match:
            if ( (tl->size>0) && (tl->in==NULL) {
                 tb->bytes += tl->size ;
            }
            break;
        case trxn_read:
                tb->bytes += tl->size ;
                break;
} else if (t->out == NULL) {
                ret = BUS_readin_data(t->in, t->size, pn);
                LEVEL_DEBUG("  Transaction readin = %d\n", ret);
} else if (t->in == NULL) {
                BYTE dummy[64];
                size_t s = t->size;
                while (s > 0) {
                    size_t ss = (s > 64) ? 64 : s;
                    s -= ss;
                    ret = BUS_sendback_data(t->out, dummy, ss, pn);
                    LEVEL_DEBUG
                        ("  Transaction null sendback (%lu of %lu)= %d\n",
                         ss, t->size, ret);
                    if (ret)
                        break;
}
} else {
                ret = BUS_sendback_data(t->out, t->in, t->size, pn);
                LEVEL_DEBUG("  Transaction sendback = %d\n", ret);
}
            break;
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
            return 0;
    case trxn_end:
            LEVEL_DEBUG("  Transaction end = %d\n", ret);
            return 0;
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
    ++ tb->packets ;
    
    
    
#endif