﻿#include "stdafx.h"
#include "DBAgent.h"

void CUser::ShoppingMall(Packet & pkt)
{
	uint8_t opcode = pkt.read<uint8_t>();

	switch (opcode)
	{
	case STORE_OPEN:
		HandleStoreOpen(pkt);
		break;

	case STORE_CLOSE:
		HandleStoreClose();
		break;

	case STORE_BUY:
	case STORE_MINI: // not sure what this is
	case STORE_PROCESS:
		/* fairly certain there's no need to emulate these as they're unused */
		break;

	default:
		TRACE("Unknown shoppingmall packet: %X\n", opcode);
	}
}

// We're opening the PUS...
void CUser::HandleStoreOpen(Packet & pkt)
{
	Packet result(WIZ_SHOPPING_MALL);
	int16_t sErrorCode = 1, sFreeSlot = -1;
	result << uint8_t(STORE_OPEN);

	if (isDead())
		sErrorCode = -2;
	else if (isTrading())
		sErrorCode = -3;
	else if (isMerchanting())
		sErrorCode = -4;
	// Not allowed in private arenas
	else if (GetZoneID() >= 40 && GetZoneID() <= 45)
		sErrorCode = -5;
	else if (isStoreOpen())
		sErrorCode = -7;

	if (sErrorCode != 1)
		goto fail_return;

	for (int i = SLOT_MAX; i < INVENTORY_TOTAL; i++)
	{
		if (GetItem(i)->nNum == 0)
		{
			sFreeSlot = i;
			break;
		}
	}

	if (sFreeSlot < 0)
	{
		sErrorCode = -8;
		goto fail_return;
	}

	m_bStoreOpen = true;
	UserDataSaveToAgent();

fail_return:
	result << sErrorCode << sFreeSlot;
	Send(&result);
}

// We're closing the PUS so that we can call LOAD_WEB_ITEMMALL and load the extra items.
void CUser::HandleStoreClose()
{
	Packet result(WIZ_SHOPPING_MALL);
	result << uint8_t(STORE_CLOSE);
	m_bStoreOpen = false;
	g_pMain->AddDatabaseRequest(result, this);
}

void CUser::ReqLoadWebItemMall()
{
	std::vector<_ITEM_DATA> itemList;

	if (!g_DBAgent.LoadWebItemMall(itemList, this))
		return;

	// reuse the GiveItem() method for giving them the item, just don't send the packet
	// as it's handled by STORE_CLOSE.
	foreach (itr, itemList)
		GiveItem(itr->nNum, itr->sCount, false); 

	Packet result(WIZ_SHOPPING_MALL);
	result << uint8_t(STORE_CLOSE);
	for (int i = SLOT_MAX; i < SLOT_MAX+HAVE_MAX; i++)
	{
		_ITEM_DATA * pItem = GetItem(i);
		result	<< pItem->nNum
			<< pItem->sDuration
			<< pItem->sCount
			<< pItem->bFlag // item type flag (e.g. rented)
			<< pItem->sRemainingRentalTime; // remaining time
	}
	Send(&result);
}