/*
 * card-blacklist.c: Support for cards with no driver
 *
 * Copyright (C) 2001, 2002  Juha Yrjölä <juha.yrjola@iki.fi>
 * Copyright (c) 2024 Doug Engert ,deengert@gmail.com
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "internal.h"

static struct sc_card_operations blacklist_ops;
static struct sc_card_driver blacklist_drv = {
	"blacklist driver for cards that must not be probed",
	"blacklist",
	&blacklist_ops,
	NULL, 0, NULL
};


static int
blacklist_match_card(struct sc_card *card)
{
	int i;
	SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

	/* only have ATRs that have been be added from opensc.conf
	 * or added when no driver matched an atr
	 * so it was added so dont prob it again 
	 */
	i = _sc_match_atr(card, blacklist_drv.atr_map, NULL);
	if  (i < 0)
		return 0;
	return 1;
}

static int
blacklist_init(struct sc_card *card)
{
	LOG_FUNC_CALLED(card->ctx);

	card->drv_data = NULL;
	/*
	 * set so reader treats the card in reader as not present
	 * thus preventing the use of the card in reader
	 * while this process is still active or C_Finalize is called by application
	 */
	card->drv_data = NULL;
	card->name = "Blacklisted card";
	sc_reset(card, SC_READER_CARD_BLACKLISTED);

	LOG_FUNC_RETURN(card->ctx, SC_ERROR_CARD_BLACKLISTED);
}

static struct sc_card_driver * sc_get_driver(void)
{
	struct sc_card_driver *iso_drv = sc_get_iso7816_driver();

	blacklist_ops = *iso_drv->ops;
	blacklist_ops.match_card = blacklist_match_card;
	blacklist_ops.init = blacklist_init;

	return &blacklist_drv;
}

struct sc_card_driver * sc_get_blacklist_driver(void)
{
	return sc_get_driver();
}
