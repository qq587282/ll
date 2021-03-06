//-----------------------------------------------------------------------------
//!\file pet_pocket.cpp
//!\author xlguo
//!
//!\date 2009-04-10
//! last 
//!
//!\brief 宠物带
//!
//!	Copyright (c) 2004 TENGWU Entertainment All rights reserved.
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "pet_pocket.h"

#include "world.h"
#include "role.h"

#include "pet_define.h"
#include "pet_soul.h"
#include "pet.h"
#include "pet_skill.h"
#include "pet_equip.h"
#include "role_mgr.h"
#include "item_creator.h"
#include "../WorldDefine/filter.h"
#include "../ServerDefine/log_cmdid_define.h"
#include "../WorldDefine/msg_pet.h"
#include "../ServerDefine/msg_pet.h"
#include "../WorldDefine/msg_pet_skill.h"
#include "../WorldDefine/msg_pet_exchange.h"

INT MAX_PETSOUL_NUM = 40;      //宝宝总的上限
INT MAX_FOLLOWPETSOUL_NUM = 20; //宠物默认的个数
INT MAX_RIDINGPETSOUL_NUM = 20; //坐骑默认的个数
INT FOLLOWPETTYPE  =  6;        //跟随宠物类型　

//----------------------------------------------------------------------------------------------------
// 测试
//----------------------------------------------------------------------------------------------------
BOOL PetPocket::DynamicTest(INT nTestNo, INT nArg1, INT nArg2)
{
	DWORD dwRtv = E_Pets_Success;

	DWORD	dwPetIDS[CONST_MAX_PETSOUL_NUM];
	INT		nPetNum = 0;
	GetAllPetID(dwPetIDS, nPetNum);
	DWORD	dwPetID = dwPetIDS[nArg1];

	DWORD	dwSkillTypeID = 11101;

	switch(nTestNo)
	{
	case -1:
		{
			TList<tagItem*> itemlist;
			// 宠物蛋typeid 4000001~4000006
			m_pMaster->GetItemMgr().GetBagSameItemList(itemlist, 4000001);
			if (itemlist.Empty())
			{
				return FALSE;
			}
			else
			{
				dwRtv = HatchEgg((*itemlist.Begin())->GetKey(), _T("helloworld"));
			}			
			
		}

		break;
	case 0:
		{
			m_pMaster->GetPetPocket()->RestPet(dwPetID);
// 			PetSoul* pSoul = GetPetSoul(dwPetID);
// 			dwRtv = pSoul->LearnNormalSkill(1);//pSoul->LearnSkill(dwSkillTypeID);
		}
		break;
	case 1:
		{
			m_pMaster->GetPetPocket()->CallPet(dwPetID);
		}
		break;
	case 2:
		{
			PetSoul* pTakeAway = m_pMaster->GetPetPocket()->GetAway(dwPetID);
			PetSoul::DeleteSoul(pTakeAway, TRUE);
		}
		break;
	case 3:
		{
			DWORD dwRTv = E_Success;
			PetSoul* pSoul = m_pMaster->GetPetPocket()->GetPetSoul(dwPetID);
			TList<tagItem*> itemlist;
			// 宠物蛋typeid 4000001~4000006
			m_pMaster->GetItemMgr().GetBagSameItemList(itemlist, 4010001);
			if (itemlist.Empty())
			{
				return FALSE;
			}
			else
			{
				tagItem* pItem = *itemlist.Begin();
				dwRTv = pSoul->Equip(pItem->GetKey(), GT_INVALID, TRUE);
			}
		}
		break;
	case 4:
		{
			DWORD dwRTv = E_Success;
			PetSoul* pSoul = m_pMaster->GetPetPocket()->GetPetSoul(dwPetID);
			BYTE buffer[1024] = {0};
			
			tagPetInitAttr* pPetInitAtt = (tagPetInitAttr*)buffer;//new (buffer) tagPetInitAttr;
			pSoul->FillClientPetAtt(pPetInitAtt);
			if (pPetInitAtt->nPetEquipNum != 0)
			{
				tagPetEquipMsgInfo* pEquipMsgInfo = (tagPetEquipMsgInfo*)pPetInitAtt->byData;
				INT i=0; 
				INT64 n64ID = pEquipMsgInfo[nArg2].n64ItemID;
				dwRTv = pSoul->UnEquip(n64ID, GT_INVALID, TRUE);
			}
		}
		break;
	case 5:
		{
			PetSoul* pSoul = m_pMaster->GetPetPocket()->GetPetSoul(dwPetID);
			pSoul->GetPetAtt().ExpChange(nArg2, TRUE);
		}
		break;
	case 6:
		{
			TList<tagItem*> itemlist;
			// 宠物蛋typeid 4000001~4000006
			m_pMaster->GetItemMgr().GetBagSameItemList(itemlist, nArg2);
			if (itemlist.Empty())
			{
				return FALSE;
			}
			else
			{
				tagItem* pItem = *itemlist.Begin();
				PetSoul* pSoul = m_pMaster->GetPetPocket()->GetPetSoul(dwPetID);
				dwRtv = pSoul->LearnBookSkill(pItem->GetKey());				
			}	

		}
		break;
	case 7:
		{
			DWORD dwRoleID = nArg1;
			Role* pRole = g_roleMgr.GetRolePtrByID(dwRoleID);
			if (!P_VALID(pRole))
			{
				break;
			}

			if (E_Pets_Success == pRole->GetPetPocket()->CanSetRideAfter(m_pMaster, TRUE))
			{
				if (E_Pets_Success == m_pMaster->GetPetPocket()->CanAddPassenger(pRole))
				{
					pRole->GetPetPocket()->RideAfter(m_pMaster);
				}
				;

			}
			;
		}
		break;
	case 8:
		{
			DWORD dwRoleID = nArg1;
			Role* pRole = g_roleMgr.GetRolePtrByID(dwRoleID);
			if (P_VALID(pRole))
			{
				pRole->GetPetPocket()->StopRideAfter(m_pMaster);
			}
		}
		break;
	default:
		break;
	}
	return TRUE;
}

//----------------------------------------------------------------------------------------------------
// 初始化
//----------------------------------------------------------------------------------------------------
BOOL PetPocket::Init(const BYTE* &pData, INT nNum, Role* pRole)
{
	m_bKeepMount = FALSE;
	m_dwPetIDForUpStep = GT_INVALID;
	m_nCalledPets = 0;
	m_dwOthersPetID = GT_INVALID;
    m_n16FollowPetCount = 0 ;
	m_n16RidingPetCount = 0 ;
	if (!P_VALID(pRole))
	{
		return FALSE;
	}

	m_pMaster = pRole;
	
	// 初始化宠物灵魂
	for (INT i=0; i<nNum; ++i)
	{
		PetSoul* pSoul = PetSoul::CreateSoulByDBData(pData, FALSE);
		if (P_VALID(pSoul))
		{
			if (E_Success != PutIn(pSoul, FALSE, FALSE))
			{
				ILOG->Write(_T("Can not create pet soul when role init petid:%u, masterid:%u\n"), pSoul->GetID(), pRole->GetID());
				PetSoul::LogPet(m_pMaster->GetID(), pSoul->GetID(), ELCLD_PET_DEL_PET_INIT);
				PetSoul::DeleteSoul(pSoul, FALSE);
			}
			
		}
	}
	
	return TRUE;
}

//----------------------------------------------------------------------------------------------------
// 更新
//----------------------------------------------------------------------------------------------------
VOID PetPocket::Update()
{
	DWORD		dwPetID	= GT_INVALID;
	PetSoul*	pSoul	= NULL;
	m_mapAllSoul.ResetIterator();
	while( m_mapAllSoul.PeekNext(dwPetID, pSoul) )
	{
		if (!P_VALID(pSoul)) continue;
		pSoul->Update();
		//begin 2010-08-04  Alex 灵兽时间到了后就删除
		if ( !pSoul->GetLiveState())
		{
            DWORD    dwDeletePetID  =  pSoul->GetID();
			PetSoul* pDeleteSoul = GetAway(dwDeletePetID, TRUE);
			if (P_VALID(pDeleteSoul))
			{
				if ( P_VALID(pDeleteSoul->GetMaster()))
				{
					PetSoul::LogPet(pDeleteSoul->GetMaster()->GetID(), dwDeletePetID, ELCLD_PET_DEL_PET_CLIENT);
					PetSoul::DeleteSoul(pDeleteSoul, TRUE); 

				}
			}
		}
		//end 2010-08-04  Alex 灵兽时间到了后就删除
	
		
	}
}

//----------------------------------------------------------------------------------------------------
// 获取所有宠物id
//----------------------------------------------------------------------------------------------------
#include <list>
VOID PetPocket::GetAllPetID( DWORD* dwPetIDbuf, INT& num )
{
	std::list<DWORD> kList;
	m_mapAllSoul.ExportAllKey(kList);
	num = 0;
	for (std::list<DWORD>::iterator itr = kList.begin();
		 itr != kList.end();
		 ++itr)
	{
		dwPetIDbuf[num++] = *itr;
	}
}

//----------------------------------------------------------------------------------------------------
// 使用宠物蛋
//----------------------------------------------------------------------------------------------------
DWORD PetPocket::HatchEgg(INT64 n64ItemID, LPCTSTR tszName)
{
	// 宠物数目检查
	if( !P_VALID(m_pMaster) )
		GT_INVALID;

	// 宠物蛋
	tagItem* pEgg = m_pMaster->GetItemMgr().GetBagItem(n64ItemID);
	if (!P_VALID(pEgg) || pEgg->pProtoType->eSpecFunc != EISF_PetEgg)
	{
		return E_Pets_Soul_PetEggErr;
	}
	if (pEgg->pProtoType->nSpecFuncVal2 > m_pMaster->GetLevel())
	{
		return E_PetRoleLvlNotEnough;
	}
	DWORD dwTypeID	= pEgg->pProtoType->nSpecFuncVal1;
	INT	nQuality	= pEgg->pProtoType->byQuality;
    const tagPetProto* pPetProto = g_attRes.GetPetProto(dwTypeID);
	if( !P_VALID(pPetProto))
	{
		return GT_INVALID;
	}
	if ( FOLLOWPETTYPE == pPetProto->nType3)  //跟随宠物
	{
         if ( GetFollowPetCount() >= m_pMaster->FollowPetPocketValue() )
         {
			 return  E_FollowPets_Pocket_Full ;
         }
           
	}
	else
	{
		if ( GetRidingPetCount() >= m_pMaster->RidingPetPocketValue() )
		{
			 return  E_RidingPets_Pocket_Full ;
		}

	}
	// 检测名称
	if (E_Success != Filter::CheckName(tszName,X_PET_NAME, 1, g_attRes.GetNameFilterWords()))
	{
		return E_Pets_Soul_NameIllegal;
	}
    
	//DB创建PetSoul
	if(!PetSoul::CreateDBSoul(dwTypeID, tszName, m_pMaster, nQuality, n64ItemID))
		return GT_INVALID;

	

	return E_Pets_Success;
}

//----------------------------------------------------------------------------------------------------
// 拿走宠物
//----------------------------------------------------------------------------------------------------
PetSoul* PetPocket::GetAway( DWORD dwPetID, BOOL bSync /*= FALSE*/ )
{
	PetSoul* pSoul2Get = m_mapAllSoul.Peek(dwPetID);
	if (!P_VALID(pSoul2Get))
	{

		// 		if (bSync)
		// 		{
		// 			tagNS_DeletePet send;
		// 			send.dwPetID = dwPetID;
		// 			send.dwErrCode = E_Pets_Soul_NotExist;
		// 			m_pMaster->SendMessage(&send, send.dwSize);
		// 		}
		return NULL;
	}
	else if (pSoul2Get->IsLocked())
	{
		if (bSync)
		{
			tagNS_DeletePet send;
			send.dwPetID = dwPetID;
			send.dwErrCode = E_Pets_Lock_Locked;
			m_pMaster->SendMessage(&send, send.dwSize);
		}
		return NULL;
	}
	else if (pSoul2Get->HasEquip())
	{
		if (bSync)
		{
			tagNS_DeletePet send;
			send.dwPetID = dwPetID;
			send.dwErrCode = E_Pets_Delete_HasEquip;
			m_pMaster->SendMessage(&send, send.dwSize);
		}
		return NULL;
	}
	// 取消召唤
	if (pSoul2Get->IsCalled())
	{
		DWORD dwRtv = RestPet(pSoul2Get->GetID());
		if (E_Pets_Success != dwRtv)
		{
			if (bSync)
			{
				tagNS_DeletePet send;
				send.dwPetID = dwPetID;
				send.dwErrCode = dwRtv;
				m_pMaster->SendMessage(&send, send.dwSize);
			}
			return NULL;
		}
	}

	// 取消骑乘
	if (pSoul2Get->IsMounted())
	{
		m_pMaster->StopMount();
	}



	// 取消工作
	if (pSoul2Get->CanSetWroking(FALSE, pSoul2Get->GetWorkingSkillTypeID()))
	{
		pSoul2Get->SetWorking(FALSE, pSoul2Get->GetWorkingSkillTypeID());
	}

	//Remove from Pocket
	m_mapAllSoul.Erase(dwPetID);

	RefreshPetCount();

	//Free it
	//tbd:
	pSoul2Get->DetachFromRole();

	//tbd:同步客户端
	if (bSync)
	{
		tagNS_DeletePet send;
		send.dwPetID = dwPetID;
		send.dwErrCode = E_Pets_Success;
		m_pMaster->SendMessage(&send, send.dwSize);
	}

	// 同步数据库
	pSoul2Get->SendSaveAtt2DB();

	return pSoul2Get;
}

PetSoul* PetPocket::Recall(DWORD dwPetID, BOOL bSync)
{
	tagNS_RecallPet send;
	send.dwPetID = dwPetID;
	send.dwErrCode = E_Success;

	if (NULL == m_pMaster)
	{
		return NULL;
	}

	ItemMgr &itemMgr = m_pMaster->GetItemMgr();
	ItemContainer &bag = itemMgr.GetBag();

	if (bag.GetSameItemCount(PET_RECALL_ITEM_ID) <= 0)
	{
		send.dwErrCode = E_Pet_RecallWithoutItem;
		m_pMaster->SendMessage(&send, send.dwSize);
		return NULL;
	}

	if (itemMgr.GetBagFreeSize() <= 0)
	{
		send.dwErrCode = E_Bag_NotEnoughSpace;
		m_pMaster->SendMessage(&send, send.dwSize);
		return NULL;
	}

	PetSoul* pSoul2Get = GetAway(dwPetID, bSync);
	if (!P_VALID(pSoul2Get))
	{
		send.dwErrCode = E_Pet_RecallDeleteErr;
		m_pMaster->SendMessage(&send, send.dwSize);
		return NULL;
	}

	//删除道具
	if (E_Success != itemMgr.RemoveFromRole(PET_RECALL_ITEM_ID, 1, ELCLD_PET_RecallPet))
	{
		send.dwErrCode = E_Pet_RecallDeleteErr;
		m_pMaster->SendMessage(&send, send.dwSize);
		return NULL;
	}

	//添加宠物，解除绑定
	const tagPetProto* pProto = pSoul2Get->GetPetAtt().GetProto();
	if (!P_VALID(pProto))
	{
		return NULL;
	}
	tagItem *pItemNew = ItemCreator::CreateEx(EICM_RecallPet, GT_INVALID, pProto->dwItemID, 1, EIQ_White);
	if(!P_VALID(pItemNew))
	{
		ASSERT(P_VALID(pItemNew));
		return NULL;
	}
	if (P_VALID(pItemNew->pProtoType) && pItemNew->pProtoType->eSpecFunc != EISF_ZhanDouFu)
		pItemNew->dw1stGainTime	 = g_world.GetWorldTime();

	pItemNew->byBind = EBS_Unbind;

	itemMgr.Add2Bag(pItemNew, ELCLD_PET_RecallPet, TRUE);

	return pSoul2Get;
}


//----------------------------------------------------------------------------------------------------
// 放入宠物
//----------------------------------------------------------------------------------------------------
DWORD PetPocket::PutIn( PetSoul* pSoul, BOOL bSaveDB, BOOL bSend /*= TRUE*/ )
{
	if (!P_VALID(pSoul))
	{
		return E_Pets_Soul_PtrNotValid;
	}

	// num check
	// 	if (m_mapAllSoul.Size() >= m_pMaster->PetPocketValve  ())
	// 	{
	// 		return E_Pets_Soul_NumExceed;
	// 	}

	//pSoul is not in Pocket
	if (P_VALID(m_mapAllSoul.Peek(pSoul->GetID())))
	{
		return E_Pets_Soul_AlreadyExist;
	}

	//Tag it
	pSoul->IntegrateInRole(m_pMaster);

	//Add to my pocket
	m_mapAllSoul.Add(pSoul->GetID(), pSoul);

	if (pSoul->IsInState(EPS_Called))
	{
		CallPet(pSoul->GetID());
	}
	RefreshPetCount();

	//Sync
	if (bSend)
	{
		// 同步到客户端
		BYTE buffer[1024] = {0};
		tagNS_GetPetAttr* pSend = (tagNS_GetPetAttr*)buffer;
		tagNS_GetPetAttr tmp;
		pSend->dwID = tmp.dwID;
		pSend->dwRoleID = m_pMaster->GetID();
		pSoul->FillClientPetAtt(&pSend->petAttr);
		pSend->dwSize = sizeof(tagNS_GetPetAttr) - 1 + 
			sizeof(tagPetSkillMsgInfo) * pSend->petAttr.nPetSkillNum + 
			sizeof(tagPetEquipMsgInfo) * pSend->petAttr.nPetEquipNum;
		m_pMaster->SendMessage(pSend, pSend->dwSize);
	}

	// 同步到数据库
	if(bSaveDB)
		pSoul->SendSaveAtt2DB();

	return E_Pets_Success;
}

DWORD PetPocket::CanPutIn( PetSoul* pSoul )
{
	if (!P_VALID(pSoul))
	{
		return E_Pets_Soul_PtrNotValid;
	}

	// num check
	// 	if (m_mapAllSoul.Size() >= m_pMaster->PetPocketValve  ())
	// 	{
	// 		return E_Pets_Soul_NumExceed;
	// 	}

	//pSoul is not in Pocket
	if (P_VALID(m_mapAllSoul.Peek(pSoul->GetID())))
	{
		return E_Pets_Soul_AlreadyExist;
	}

	return E_Pets_Success;
}

//----------------------------------------------------------------------------------------------------
// 召唤宠物
//----------------------------------------------------------------------------------------------------
DWORD PetPocket::CallPet(DWORD dwPetID)
{
	//all precondition
	if( !P_VALID(m_pMaster) ||
		!IS_PET(dwPetID)	
		)
	{
		ASSERT(0);
		return E_Pets_UnkownErr;
	}

	//Get PetSoul by PetID
	PetSoul* pCalledSoul = GetPetSoul(dwPetID);
	if (!P_VALID(pCalledSoul))
	{
		ASSERT(0);
		return E_Pets_Soul_NotExist;
	}

	// Whether Can Call Pet?
	if (!pCalledSoul->GetLiveState())
	{
		ASSERT(0);
		return E_Pets_Soul_LiveEnd;
	}

	if (pCalledSoul->IsCalled())
	{
		ASSERT(0);
		return E_Pets_Pet_AlreadyCalled;
	}

	if (m_nCalledPets >= MAX_CALLED_PET_NUM)
	{
		if (RestAPet() != E_Pets_Success)
		{
			ASSERT(0);
			return E_Pets_Pet_CalledNumExceed;
		}
		ASSERT(m_nCalledPets < MAX_CALLED_PET_NUM);
	}

	if (!pCalledSoul->SetCalled(TRUE, TRUE))
	{
		return E_Pets_UnkownErr;
	}
	++m_nCalledPets;


	if(pCalledSoul->GetProto()->dwAddBuffID != GT_INVALID)
		m_pMaster->TryAddBuffToMyself(pCalledSoul->GetProto()->dwAddBuffID);
	
	return E_Pets_Success;
}

//----------------------------------------------------------------------------------------------------
// 宠物休息
//----------------------------------------------------------------------------------------------------
DWORD PetPocket::RestPet(DWORD dwPetID)
{
	PetSoul* pSoul = m_mapAllSoul.Peek(dwPetID);
	if (!P_VALID(pSoul))
	{
		ASSERT(0);
		return E_Pets_Soul_NotExist;
	}

	if (!pSoul->IsCalled())
	{
		ASSERT(0);
		return E_Pets_Pet_NotCalled;
	}

	if (pSoul->IsWorking())
	{
		ASSERT(0);
		return E_Pets_Soul_IsWorking;
	}


	pSoul->SetCalled(FALSE, TRUE);
	--m_nCalledPets;

	if(pSoul->GetProto()->dwAddBuffID != GT_INVALID)
		m_pMaster->RemoveBuff(pSoul->GetProto()->dwAddBuffID/100, TRUE);

	return E_Pets_Success;
}

//----------------------------------------------------------------------------------------------------
// 获取PetSoul
//----------------------------------------------------------------------------------------------------
PetSoul* PetPocket::GetPetSoul(DWORD dwPetID)
{
	return m_mapAllSoul.Peek(dwPetID);
}

//----------------------------------------------------------------------------------------------------
// 召唤出的宠物进入宠物带
//----------------------------------------------------------------------------------------------------
VOID PetPocket::CalledPetEnterPocket()
{
	PetSoul* pSoul = NULL;
	SoulTMap::TMapIterator itr = m_mapAllSoul.Begin();
	while (m_mapAllSoul.PeekNext(itr, pSoul))
	{
		if (P_VALID(pSoul) && pSoul->IsCalled() && !pSoul->IsMounted())
		{
			if (pSoul->IsPetInMap())
			{
				pSoul->BodyLeaveMap();
			}
		}
	}
}

//----------------------------------------------------------------------------------------------------
// 召唤出的宠物离开宠物带
//----------------------------------------------------------------------------------------------------
VOID PetPocket::CalledPetLeavePocket()
{
	PetSoul* pSoul = NULL;
	SoulTMap::TMapIterator itr = m_mapAllSoul.Begin();
	while (m_mapAllSoul.PeekNext(itr, pSoul))
	{
		if (P_VALID(pSoul) && pSoul->IsCalled() && !pSoul->IsMounted() && !pSoul->IsPetInMap())
		{
			pSoul->BodyEnterMap();
		}
	}
}

//----------------------------------------------------------------------------------------------------
// tbc:内存泄露销毁
//----------------------------------------------------------------------------------------------------
VOID PetPocket::Destroy()
{
	//delete all soul
	PetSoul* pSoul = NULL;
	SoulTMap::TMapIterator itr = m_mapAllSoul.Begin();
	
	while (m_mapAllSoul.PeekNext(itr, pSoul))
	{
		if (!P_VALID(pSoul))
			continue;
		if (pSoul->IsCalled())
			RestPet(pSoul->GetID());
		PetSoul::DeleteSoul(pSoul, FALSE);
	}
	m_mapAllSoul.Clear();
	m_nCalledPets = 0;
}

//----------------------------------------------------------------------------------------------------
// 保存数据
//----------------------------------------------------------------------------------------------------
BOOL PetPocket::SaveToDB( IN LPVOID pData, OUT LPVOID &pOutPointer, OUT INT32 &nNum )
{
	pOutPointer = pData;
	nNum = 0;

	SoulTMap::TMapIterator itr = m_mapAllSoul.Begin();
	PetSoul* pSoul = NULL;

	while (m_mapAllSoul.PeekNext(itr, pSoul))
	{
		if (!P_VALID(pSoul))
			continue;

		if (!pSoul->SaveToDB(pData, pData))
			continue;

		++nNum;
	}

	pOutPointer = pData;

	return TRUE;
}

//----------------------------------------------------------------------------------------------------
// 设置一个宠物到休息状态
//----------------------------------------------------------------------------------------------------
DWORD PetPocket::RestAPet()
{
	SoulTMap::TMapIterator itr = m_mapAllSoul.Begin();
	PetSoul* pSoul = NULL;
	while (m_mapAllSoul.PeekNext(itr, pSoul))
	{
		if (P_VALID(pSoul) && pSoul->IsCalled())
		{
			if (E_Pets_Success == RestPet(pSoul->GetID()))
			{
				return E_Pets_Success;
			}
		}
	}
	
	return GT_INVALID;
}

//----------------------------------------------------------------------------------------------------
// 骑乘宠物
//----------------------------------------------------------------------------------------------------
DWORD PetPocket::RidePet()
{
	// 找到预备的宠物
	PetSoul* pSoul = GetPreparingPetSoul();
	if (!P_VALID(pSoul))
	{
		return GT_INVALID;
	}
	// 状态判断
	if (!pSoul->IsPreparing() || pSoul->IsMounted())
	{
		return GT_INVALID;
	}
	// 若可以骑乘
	if (pSoul->CanSetMount(TRUE))
	{
		// 骑乘
		pSoul->SetMount(TRUE);
	}
	// 若不可骑乘
	else
	{
		// return why
		return GT_INVALID;
	}

	return E_Pets_Success;
}

//----------------------------------------------------------------------------------------------------
// 找到预备驾驭的宠物
//----------------------------------------------------------------------------------------------------
PetSoul* PetPocket::GetPreparingPetSoul()
{
	SoulTMap::TMapIterator itr = m_mapAllSoul.Begin();
	PetSoul* pSoul = NULL;
	while (m_mapAllSoul.PeekNext(itr, pSoul))
	{
		if (!P_VALID(pSoul))
			continue;

		if (pSoul->IsPreparing())
		{
			return pSoul;
		}
	}

	return NULL;
}

//----------------------------------------------------------------------------------------------------
// 得到当前骑乘的宠物
//----------------------------------------------------------------------------------------------------
PetSoul* PetPocket::GetMountPetSoul()
{
	SoulTMap::TMapIterator itr = m_mapAllSoul.Begin();
	PetSoul* pSoul = NULL;
	while (m_mapAllSoul.PeekNext(itr, pSoul))
	{
		if (!P_VALID(pSoul))
			continue;

		if (pSoul->IsMounted())
		{
			return pSoul;
		}
	}

	return NULL;
}

//----------------------------------------------------------------------------------------------------
// 下坐骑
//----------------------------------------------------------------------------------------------------
DWORD PetPocket::UnRidePet()
{
	// 找到预备的宠物
	PetSoul* pSoul = GetMountPetSoul();
	if (!P_VALID(pSoul))
	{
		return GT_INVALID;
	}
	// 状态判断
	if (!pSoul->IsMounted())
	{
		return GT_INVALID;
	}
	// 若可以骑乘
	if (pSoul->CanSetMount(FALSE))
	{
		// 骑乘
		pSoul->SetMount(FALSE);
	}
	// 若不可骑乘
	else{
		// return why
		return GT_INVALID;
	}

	return E_Pets_Success;
}

//----------------------------------------------------------------------------------------------------
// 设置预备驾驭
//----------------------------------------------------------------------------------------------------
DWORD PetPocket::PreparePet( DWORD dwPetID )
{
	PetSoul* pToSet = GetPetSoul(dwPetID);
	if (P_VALID(pToSet) && pToSet->CanSetPreparing(TRUE))
	{
		PetSoul* pSoul = GetPreparingPetSoul();
		if (P_VALID(pSoul))
		{
			pSoul->SetPreparing(FALSE);
		}

		pToSet->SetPreparing(TRUE);
		return E_Pets_Success;
	}
	return GT_INVALID;
}

//----------------------------------------------------------------------------------------------------
// 取消预备驾驭
//----------------------------------------------------------------------------------------------------
DWORD PetPocket::UnPreparePet( DWORD dwPetID )
{
	PetSoul* pToUnSet = GetPetSoul(dwPetID);
	if (P_VALID(pToUnSet) && pToUnSet->CanSetPreparing(FALSE))
	{
		pToUnSet->SetPreparing(FALSE);
		return E_Pets_Success;
	}

	return GT_INVALID;
}

//----------------------------------------------------------------------------------------------------
// 取得当前骑乘的宠物
//----------------------------------------------------------------------------------------------------
PetSoul* PetPocket::GetCalledPetSoul()
{
	SoulTMap::TMapIterator itr = m_mapAllSoul.Begin();
	PetSoul* pSoul = NULL;
	while (m_mapAllSoul.PeekNext(itr, pSoul))
	{
		if (!P_VALID(pSoul))
			continue;

		if (pSoul->IsCalled())
		{
			return pSoul;
		}
	}

	return NULL;
}

//----------------------------------------------------------------------------------------------------
// 能否骑乘宠物
//----------------------------------------------------------------------------------------------------
DWORD PetPocket::CanRidePet()
{
	// 找到预备的宠物
	PetSoul* pSoul = GetPreparingPetSoul();
	if (!P_VALID(pSoul))
	{
		return E_UseSkill_Mount_NoPreparingPet;
	}
	// 状态判断
	if (!pSoul->IsPreparing() || pSoul->IsMounted())
	{
		return E_UseSkill_SelfStateLimit;
	}
	// 若不可以骑乘
	if (!pSoul->CanSetMount(TRUE))
	{
		return E_UseSkill_SelfStateLimit;
	}

	return E_Pets_Success;
}
// Jason 2010-3-9 v1.4.0
VOID PetPocket::CancleLastPetPreparingStatus()
{
	//PetSoul* pSoul = GetPreparingPetSoul();
	//if (P_VALID(pSoul))
	//{
	//	if( pSoul->IsPreparing() )
	//		UnPreparePet(pSoul->GetID());
	//}
	//UnRidePet();
	if( !P_VALID(m_pMaster) )
		return;
	if( m_pMaster->IsHaveBuff (Buff::GetIDFromTypeID(MOUNT_BUFF_ID)) )
		m_pMaster->CancelBuff  (Buff::GetIDFromTypeID(MOUNT_BUFF_ID) );
	if( m_pMaster->IsHaveBuff  (Buff::GetIDFromTypeID(MOUNT2_BUFF_ID)) )
		m_pMaster->CancelBuff  (Buff::GetIDFromTypeID(MOUNT2_BUFF_ID) );
}

//----------------------------------------------------------------------------------------------------
// 某个宠物能否被交易
//----------------------------------------------------------------------------------------------------
DWORD PetPocket::CanExchange( DWORD dwPetID )
{
	PetSoul* pSoul = GetPetSoul(dwPetID);
	if (!P_VALID(pSoul))
	{
		return GT_INVALID;
	}

	if (pSoul->IsLocked())
	{
		return E_Pet_Exchange_PetLocked;
	}
	else if (pSoul->GetProto()->bBind)
	{
		return E_Pet_Exchange_PetBinded;
	}
	else if (pSoul->IsWorking())
	{
		return E_Pet_Exchange_PetStateNotAllow;
	}
	else if(pSoul->IsMounted())
	{
		return E_Pet_Exchange_PetStateNotAllow;
	}
	else if(pSoul->IsCalled())
	{
		return E_Pet_Exchange_PetStateNotAllow;
	}
	else if(pSoul->HasEquip())
	{
		return E_Pet_Exchange_PetHasEquip;
	}
	else
	{
		return E_Success;
	}
}

//----------------------------------------------------------------------------------------------------
// 检查指定的宠物是可以交易
//----------------------------------------------------------------------------------------------------
BOOL PetPocket::CheckExistInPocket( DWORD *dwPetIDs, INT nNum )
{
	for (INT i=0; i<nNum; ++i)
	{
		DWORD dwPetID = dwPetIDs[i];
		if (P_VALID(dwPetID) && E_Success != CanExchange(dwPetID))
		{
			return FALSE;
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------------------------------
// 从宠物带中拿走
//----------------------------------------------------------------------------------------------------
VOID PetPocket::TakeFromPocket( PetSoul* *pSouls, INT nSizeSouls, DWORD* dwPetIDs, INT nNumPetID )
{
	INT iSouls=0;

	for (INT i=0; i<nNumPetID; ++i)
	{
		PetSoul* pSoul = GetAway(dwPetIDs[i], TRUE);
		if (P_VALID(pSoul))
		{
			PetSoul::LogPet(m_pMaster->GetID(), pSoul->GetID(), ELCLD_PET_LOSE_PET);
			pSouls[iSouls++] = pSoul;
			if (iSouls >= nSizeSouls)
			{
				break;
			}
		}
	}
}

//----------------------------------------------------------------------------------------------------
// 放入宠物带
//----------------------------------------------------------------------------------------------------
VOID PetPocket::PutInPocket( PetSoul* *pSouls, INT nSizeSouls )
{
	for (INT i=0; i<nSizeSouls; ++i)
	{
		PetSoul* pSoul = pSouls[i];
		if (P_VALID(pSoul))
		{
			DWORD dwErr = PutIn(pSoul, TRUE, TRUE);
			if (dwErr == E_Pets_Success)
			{
				PetSoul::LogPet(m_pMaster->GetID(), pSoul->GetID(), ELCLD_PET_GAIN_PET);
			}
			else
			{
				ILOG->Write(_T("Can not put in petpocket when exchange pet petid:%u, masterid:%u\n"), pSoul->GetID(), m_pMaster->GetID());
				break;
			}
		}		
	}
}

//----------------------------------------------------------------------------------------------------
// 得到可用宠物带空间
//----------------------------------------------------------------------------------------------------
BOOL PetPocket::GetFreeSize()
{
	return m_pMaster->PetPocketValve  () - m_mapAllSoul.Size();
}

//----------------------------------------------------------------------------------------------------
// 得到剩余跟随宠物带空间
//----------------------------------------------------------------------------------------------------
UINT16	PetPocket::GetFreeFollowPetSize()const
{
	UINT16 nFreeFollowPetCount =  m_pMaster->FollowPetPocketValue() - GetFollowPetCount();
    return nFreeFollowPetCount;
}

UINT16	PetPocket::GetFreeRidingPetSize()const
{
    UINT16 nFreeRidingPetCount = m_pMaster->RidingPetPocketValue() - GetRidingPetCount();
	return nFreeRidingPetCount;
}

//----------------------------------------------------------------------------------------------------
// 得到可用坐骑栏空间
//----------------------------------------------------------------------------------------------------
UINT16   PetPocket::GetFollowPetCount()const
{
     return m_n16FollowPetCount ;
}

//----------------------------------------------------------------------------------------------------
// 得到可用坐骑栏空间
//----------------------------------------------------------------------------------------------------
UINT16   PetPocket::GetRidingPetCount()const
{
    return  m_n16RidingPetCount;
}

//----------------------------------------------------------------------------------------------------
// 刷新宠物个数
//----------------------------------------------------------------------------------------------------
VOID    PetPocket::RefreshPetCount()
{
       UINT16 n16FollowPetCount =  0 ;
	   PetSoul* pPetSoul = NULL;
	   SoulTMap::TMapIterator it = m_mapAllSoul.Begin();
	   while (m_mapAllSoul.PeekNext(it,pPetSoul))
	   {
           if (P_VALID(pPetSoul))
           {
			   if ( FOLLOWPETTYPE == pPetSoul->GetProto()->nType3)
			   {
				   n16FollowPetCount++;
			   }
           }
	   }
	   m_n16FollowPetCount = n16FollowPetCount;
	   m_n16RidingPetCount =(UINT16)m_mapAllSoul.Size() - m_n16FollowPetCount;
}

//----------------------------------------------------------------------------------------------------
// 召唤的宠物提升资质
//----------------------------------------------------------------------------------------------------
DWORD PetPocket::CalledSoulEnhance( INT64 n64ItemID )
{
	MTRANS_ELSE_RET(pCalled, GetCalledPetSoul(), PetSoul, E_Pets_Pet_NotCalled);

	INT nQuality = pCalled->GetPetAtt().GetAttVal(EPA_Quality);
	if (pCalled->GetPetAtt().GetAttVal(EPA_Aptitude) >= pCalled->GetProto()->nAptitudeMax[nQuality])
	{
		return E_Pets_AlreadyMaxAptitude;
	}

	MTRANS_ELSE_RET(pItem, m_pMaster->GetItemMgr().GetBagItem(n64ItemID), tagItem, E_Pets_ItemNotValid);
	if (pItem->pProtoType->eSpecFunc != EISF_PetEnhance)
	{
		return E_Pets_ItemNotValid;
	}

	if (pItem->pProtoType->nSpecFuncVal1 != pCalled->GetPetAtt().GetAttVal(EPA_Quality))
	{
		return E_Pets_QualityNotFit;
	}

	INT nAptitudeAdd = IUTIL->RandomInRange(0, pItem->pProtoType->nSpecFuncVal2);
	if (nAptitudeAdd == 0)
	{
		return E_Pets_EnhanceFailed;
	}

	if (E_Success != m_pMaster->GetItemMgr().ItemUsedFromBag(n64ItemID, 1, ELCLD_PET_ITEM_PET_Enhance))
	{
		return E_Pets_ItemNotValid;
	}

	return pCalled->Enhance(nAptitudeAdd);
}

//----------------------------------------------------------------------------------------------------
// 召唤的宠物升阶
//----------------------------------------------------------------------------------------------------
DWORD PetPocket::CalledSoulUpStep( INT64 n64ItemID, DWORD &dwSkillID, INT &nDstStep)
{
	if (GT_VALID(n64ItemID) && GT_VALID(m_dwPetIDForUpStep))
	{
		m_dwPetIDForUpStep = GT_INVALID;
	}
	if (GT_VALID(n64ItemID) && !GT_VALID(m_dwPetIDForUpStep))
	{
		dwSkillID = GT_INVALID;
		nDstStep = GT_INVALID;

		MTRANS_ELSE_RET(pCalled, GetCalledPetSoul(), PetSoul, E_Pets_Pet_NotCalled);
		MTRANS_ELSE_RET(pItem, m_pMaster->GetItemMgr().GetBagItem(n64ItemID), tagItem, E_Pets_ItemNotValid);
		if (pItem->pProtoType->eSpecFunc != EISF_PetLvlupStep)
		{
			return E_Pets_ItemNotValid;
		}

		if (pItem->pProtoType->nSpecFuncVal1 != pCalled->GetPetAtt().GetStep() + 1)
		{
			return E_Pets_ItemNotValid;
		}

		if (pItem->pProtoType->byMinUseLevel > m_pMaster->GetLevel())
		{
			return E_Pets_ItemNotValid;
		}

		nDstStep = pCalled->GetPetAtt().GetStep() + 1;

		const tagPetLvlUpItemProto* pLvlUpItemProto = g_attRes.GetPetLvlUpItemProto(pItem->dwTypeID);
		if (!P_VALID(pLvlUpItemProto))
		{
			dwSkillID = GT_INVALID;
		}
		else
		{
			INT nSkillIndex = IUTIL->RandomInRange(1, pItem->pProtoType->nSpecFuncVal2);
			dwSkillID = pLvlUpItemProto->dwSkillIDs[nSkillIndex - 1];
		}
		m_dwPetIDForUpStep = pCalled->GetID();
		m_pMaster->GetItemMgr().ItemUsedFromBag(n64ItemID, 1, ELCLD_PET_ITEM_PET_Enhance);

		return E_Pets_Success;
	}
	else if (GT_VALID(nDstStep) && !GT_VALID(n64ItemID) && GT_VALID(m_dwPetIDForUpStep))
	{
		PetSoul* pUpSoul = GetPetSoul(m_dwPetIDForUpStep);
		if (nDstStep != pUpSoul->GetPetAtt().GetStep() + 1)
		{
			return E_Pets_ItemNotValid;
		}
		m_dwPetIDForUpStep = GT_INVALID;
		return pUpSoul->UpStep(dwSkillID);
	}
	else
	{
		ASSERT(0);
		return GT_INVALID;
	}
	
}

//----------------------------------------------------------------------------------------------------
// 骑在另一个玩家的坐骑上
//----------------------------------------------------------------------------------------------------
DWORD PetPocket::RideAfter( Role* pMaster )
{
	if (P_VALID(pMaster))
	{
		BOOL bOk = m_pMaster->TryAddBuff(pMaster, g_attRes.GetBuffProto(MOUNT2_BUFF_ID), NULL, NULL, NULL);
		if (bOk)
		{
			PetSoul* pSoul = pMaster->GetPetPocket()->GetMountPetSoul();
			if (P_VALID(pSoul))
			{
				tagNS_Mount2 send;
				send.dwPetID = pSoul->GetID();
				send.dwRoleID = m_pMaster->GetID();
				m_pMaster->GetMap()->SendBigVisTileMsg(m_pMaster, &send, send.dwSize);
			}
			else
			{
				ASSERT(0);
			}
		}
		return bOk ? E_Pets_Success : GT_INVALID;
	}
	
	return GT_INVALID;
}

//----------------------------------------------------------------------------------------------------
// 从另一个玩家的坐骑上下来
//----------------------------------------------------------------------------------------------------
DWORD PetPocket::StopRideAfter( Role* pMaster )
{
	if (P_VALID(pMaster))
	{
		m_pMaster->RemoveBuff(Buff::GetIDFromTypeID(MOUNT2_BUFF_ID), FALSE);
		return E_Pets_Success;
	}
	
	return GT_INVALID;
}

//----------------------------------------------------------------------------------------------------
// 设置为副骑乘
//----------------------------------------------------------------------------------------------------
DWORD PetPocket::SetRideAfter( Role* pHost, BOOL bSet )
{
	if (!P_VALID(pHost))	return GT_INVALID;
	MTRANS_ELSE_RET(pOtherPet, pHost->GetPetPocket()->GetMountPetSoul(), PetSoul, GT_INVALID);

	if (bSet)
	{
		m_pMaster->SetRoleState(ERS_Mount2, TRUE);
		m_dwOthersPetID = pOtherPet->GetID();
		m_pMaster->GetMoveData().ForceTeleport(pHost->GetMoveData().m_vPos, FALSE);
	}
	else
	{
		m_pMaster->UnsetRoleState(ERS_Mount2, TRUE);
		m_dwOthersPetID = GT_INVALID;
	}

	return E_Pets_Success;
}

//----------------------------------------------------------------------------------------------------
// 可以作为副骑乘
//----------------------------------------------------------------------------------------------------
DWORD PetPocket::CanSetRideAfter( Role* pHost, BOOL bSet )
{
	if (m_pMaster->IsInStateCantMove())
	{
		return GT_INVALID;
	}
	else if (m_pMaster->IsInRoleStateAny(ERS_Stall | ERS_PrisonArea))
	{
		return GT_INVALID;
	}
	else if (IsRideAfter() || m_pMaster->IsInRoleState(ERS_Mount2))
	{
		return GT_INVALID;
	}
	else if (P_VALID(GetMountPetSoul()) || m_pMaster->IsInRoleState(ERS_Mount))
	{
		return GT_INVALID;
	}
	else if (pHost->GetMapID() != m_pMaster->GetMapID())
	{
		return GT_INVALID;
	}
	else if (!pHost->IsInDistance(*m_pMaster, MAX_MOUNTINVITE_DISTANCE))
	{
		return GT_INVALID;
	}

	return E_Pets_Success;
}

//----------------------------------------------------------------------------------------------------
// 是否可以副骑乘
//----------------------------------------------------------------------------------------------------
BOOL PetPocket::IsRideAfter()
{
	return GT_VALID(m_dwOthersPetID) && m_pMaster->IsInRoleState(ERS_Mount2);
}

//----------------------------------------------------------------------------------------------------
// 作为主骑乘，是否可以添加一个副骑乘
//----------------------------------------------------------------------------------------------------
DWORD PetPocket::CanAddPassenger( Role* pPassenger )
{
	MTRANS_ELSE_RET(pSoul, GetMountPetSoul(), PetSoul, GT_INVALID);
	if (!pSoul->CanAddPassenger(pPassenger))
	{
		return GT_INVALID;
	}

	return E_Pets_Success;
}

//----------------------------------------------------------------------------------------------------
// 添加一个副骑乘
//----------------------------------------------------------------------------------------------------
DWORD PetPocket::AddPassenger( Role* pPassenger )
{
	MTRANS_ELSE_RET(pSoul, GetMountPetSoul(), PetSoul, GT_INVALID);
	if (!P_VALID(pPassenger))
	{
		return GT_INVALID;
	}
	ASSERT(pSoul->CanAddPassenger(pPassenger));
	
	return pSoul->AddPassenger(pPassenger);
}

//----------------------------------------------------------------------------------------------------
// 移除一个副骑乘
//----------------------------------------------------------------------------------------------------
DWORD PetPocket::RemovePassenger( Role* pPassenger )
{
	MTRANS_ELSE_RET(pSoul, GetMountPetSoul(), PetSoul, GT_INVALID);
	if (!P_VALID(pPassenger))
	{
		return GT_INVALID;
	}
	ASSERT(pSoul->CanRemovePassenger(pPassenger));

	return pSoul->RemovePassenger(pPassenger);
}

//----------------------------------------------------------------------------------------------------
// 能否移除一个副骑乘
//----------------------------------------------------------------------------------------------------
DWORD PetPocket::CanRemovePassenger( Role* pPassenger )
{
	MTRANS_ELSE_RET(pSoul, GetMountPetSoul(), PetSoul, GT_INVALID);
	if (!pSoul->CanRemovePassenger(pPassenger))
	{
		return GT_INVALID;
	}

	return E_Pets_Success;
}

DWORD PetPocket::LockPet( DWORD dwPetID, INT64 n64ItemID )
{
	// 获得宠物
	MTRANS_ELSE_RET(pToLock, GetPetSoul(dwPetID), PetSoul, E_Pets_Pet_NotExist);

	// 获得物品
	MTRANS_ELSE_RET(pItem, m_pMaster->GetItemMgr().GetBagItem(n64ItemID), tagItem, E_Pets_ItemNotValid);
	if (pItem->pProtoType->eSpecFunc != EISF_PetLock)
	{
		return E_Pets_ItemNotValid;
	}

	// 宠物可锁定
	if (pToLock->IsLocked())
	{
		return E_Pets_Lock_AlreadyLocked;
	}
	// 扣除物品
	if(E_Success == m_pMaster->GetItemMgr().ItemUsedFromBag(n64ItemID, 1, ELCLD_PET_ITEM_PET_Lock))
	{
		// 锁定
		pToLock->SetLocked(TRUE);
	}
	return E_Success;
}

DWORD PetPocket::UnLockPet( DWORD dwPetID, INT64 n64ItemID )
{
	// 获得宠物
	MTRANS_ELSE_RET(pToUnLock, GetPetSoul(dwPetID), PetSoul, E_Pets_Pet_NotExist);

	// 获得物品
	MTRANS_ELSE_RET(pItem, m_pMaster->GetItemMgr().GetBagItem(n64ItemID), tagItem, E_Pets_ItemNotValid);
	if (pItem->pProtoType->eSpecFunc != EISF_PetUnLock)
	{
		return E_Pets_ItemNotValid;
	}

	// 宠物可解锁
	if (!pToUnLock->IsLocked())
	{
		return E_Pets_Lock_NotLocked;
	}
	// 扣除物品
	if(E_Success == m_pMaster->GetItemMgr().ItemUsedFromBag(n64ItemID, 1, ELCLD_PET_ITEM_PET_UnLock))
	{
		// 解锁
		pToUnLock->SetLocked(FALSE);
	}

	return E_Success;
}

DWORD PetPocket::CalledPetFeed( INT64 n64ItemID )
{
	PetSoul* pCalledSoul = GetCalledPetSoul();
	if (!P_VALID(pCalledSoul))
	{
		return E_Pets_Soul_NotExist;
	}

	tagItem* pItem = m_pMaster->GetItemMgr().GetBagItem(n64ItemID);
	if (!P_VALID(pItem) || pItem->pProtoType->eSpecFunc != EISF_PetFood)
	{
		return E_Pets_ItemNotValid;
	}

	if (m_pMaster->GetItemMgr().IsItemCDTime(pItem->dwTypeID))
	{
		return E_Pets_Food_CoolingDown;
	}

	if (pCalledSoul->GetProto()->nRoleLvlLim < pItem->pProtoType->nSpecFuncVal1)
	{
		return E_Pets_Carrylevel_NotEnough;
	}

	DWORD dwRtv = pCalledSoul->Feed(pItem->pProtoType->nSpecFuncVal2);

	if (E_Pets_Success == dwRtv)
	{
		DWORD dwTypeID = pItem->dwTypeID;
		m_pMaster->GetItemMgr().ItemUsedFromBag(n64ItemID, 1, ELCLD_PET_ITEM_PET_Food);

		// 加入物品公共冷却时间
		m_pMaster->GetItemMgr().Add2CDTimeMap(dwTypeID);

	}

	return dwRtv;
}