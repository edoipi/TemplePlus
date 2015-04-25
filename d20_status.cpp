#include "stdafx.h"
#include "d20_status.h"
#include "condition.h"
#include "obj.h"
#include "temple_functions.h"


D20StatusSystem d20StatusSys;

void D20StatusSystem::initRace(objHndl objHnd)
{
	if (objects.IsCritter(objHnd))
	{
		Dispatcher * dispatcher = objects.GetDispatcher(objHnd);
		if (objects.IsUndead(objHnd))
		{
			_ConditionAddToAttribs_NumArgs0(dispatcher, conds.ConditionMonsterUndead);
		}

		uint32_t objRace = objects.GetRace(objHnd);
		CondStruct ** condStructRace = conds.ConditionArrayRace + objRace;
		_ConditionAddToAttribs_NumArgs0(dispatcher, *condStructRace);

		if (objects.IsSubtypeFire(objHnd))
		{
			_ConditionAddToAttribs_NumArgs0(dispatcher, conds.ConditionSubtypeFire);
		}

		if (objects.IsOoze(objHnd))
		{
			_ConditionAddToAttribs_NumArgs0(dispatcher, conds.ConditionMonsterOoze);
		}
	}
}

void D20StatusSystem::initClass(objHndl objHnd)
{
	if (objects.IsCritter(objHnd))
	{
		Dispatcher * dispatcher = objects.GetDispatcher(objHnd);

		CondStruct ** condStructClass = conds.ConditionArrayClasses;

		uint32_t stat = stat_level_barbarian;
		for (uint32_t i = 0; i < NUM_CLASSES; i++)
		{
			if (objects.StatLevelGet(objHnd, (Stat)stat) > 0
				&& *condStructClass != nullptr)
			{
				_ConditionAddToAttribs_NumArgs0(dispatcher, *condStructClass);
			};

			condStructClass += 1;
			stat += 1;
		}

		if (objects.StatLevelGet(objHnd, stat_level_cleric) >= 1)
		{
			_D20StatusInitDomains(objHnd);
		}

		if (objects.StatLevelGet(objHnd, stat_level_paladin) >= 3)
		{
			_ConditionAddToAttribs_NumArgs0(dispatcher, conds.ConditionTurnUndead);
		}

		if (objects.StatLevelGet(objHnd, stat_level_bard) >= 1)
		{
			_ConditionAddToAttribs_NumArgs0(dispatcher, conds.ConditionBardicMusic);
		}

		if (objects.getInt32(objHnd, obj_f_critter_school_specialization) & 0xFF)
		{
			_ConditionAddToAttribs_NumArgs0(dispatcher, conds.ConditionSchoolSpecialization);
		}
	}
}

void D20StatusSystem::D20StatusInit(objHndl objHnd)
{
	static int debugLol = 0;
	Dispatcher * dispatcher = objects.GetDispatcher(objHnd);
	if (dispatcher != nullptr && (uint32_t)dispatcher != 0xFFFFFFFF)
	{
		return;
	}

	dispatcher = objects.dispatch.DispatcherInit(objHnd);

	objects.SetDispatcher(objHnd, (uint32_t)dispatcher);

	objects.dispatch.DispatcherClearAttribs(dispatcher);

	if (objects.IsCritter(objHnd))
	{
		initClass(objHnd);

		initRace(objHnd);

		initFeats(objHnd);

	}
	else
	{
		debugLol++;
		if (debugLol % 1000 == 1)
		{
			auto lololol = 0;
		}
	}

	initItemConditions(objHnd);

	objects.d20.D20StatusInitFromInternalFields(objHnd, dispatcher);

	objects.d20.AppendObjHndToArray10BCAD94(objHnd);

	if (*objects.d20.D20GlobalSthg10AA3284 != 0){ return; }

	if (objects.IsCritter(objHnd))
	{
		if (!objects.IsDeadNullDestroyed(objHnd))
		{
			uint32_t hpCur = objects.StatLevelGet(objHnd, stat_hp_current);
			uint32_t subdualDam = objects.getInt32(objHnd, obj_f_critter_subdual_damage);

			if ((uint32_t)hpCur != 0xFFFF0001)
			{
				if (hpCur < 0)
				{
					if (feats.HasFeatCount(objHnd, FEAT_DIEHARD))
					{
						_ConditionAdd_NumArgs0(dispatcher, conds.ConditionDisabled);
					}
					else
					{
						_ConditionAdd_NumArgs0(dispatcher, conds.ConditionUnconscious);
					}
				}

				else
				{
					if (hpCur == 0)
					{
						_ConditionAdd_NumArgs0(dispatcher, conds.ConditionDisabled);
					}
					else if (subdualDam > hpCur)
					{
						_ConditionAdd_NumArgs0(dispatcher, conds.ConditionUnconscious);
					}
				}

			}
		}
	}
	return;
}

void D20StatusSystem::initDomains(objHndl objHnd)
{
	Dispatcher * dispatcher = objects.GetDispatcher(objHnd);
	uint32_t domain_1 = objects.getInt32(objHnd, obj_f_critter_domain_1);
	uint32_t domain_2 = objects.getInt32(objHnd, obj_f_critter_domain_2);

	if (domain_1)
	{
		CondStruct * condStructDomain1 = *(conds.ConditionArrayDomains + 3 * domain_1);
		uint32_t arg1 = *(conds.ConditionArrayDomainsArg1 + 3 * domain_1);
		uint32_t arg2 = *(conds.ConditionArrayDomainsArg2 + 3 * domain_1);
		if (condStructDomain1 != nullptr)
		{
			_ConditionAddToAttribs_NumArgs2(dispatcher, condStructDomain1, arg1, arg2);
		}
	}

	if (domain_2)
	{
		CondStruct * condStructDomain2 = *(conds.ConditionArrayDomains + 3 * domain_2);
		uint32_t arg1 = *(conds.ConditionArrayDomainsArg1 + 3 * domain_2);
		uint32_t arg2 = *(conds.ConditionArrayDomainsArg2 + 3 * domain_2);
		if (condStructDomain2 != nullptr)
		{
			_ConditionAddToAttribs_NumArgs2(dispatcher, condStructDomain2, arg1, arg2);
		}
	}

	auto alignmentchoice = objects.getInt32(objHnd, obj_f_critter_alignment_choice);
	if (alignmentchoice == 2)
	{
		_ConditionAddToAttribs_NumArgs2(dispatcher, conds.ConditionTurnUndead, 1, 0);
	}
	else
	{
		_ConditionAddToAttribs_NumArgs2(dispatcher, conds.ConditionTurnUndead, 0, 0);
	}
}

void D20StatusSystem::initFeats(objHndl objHnd)
{
	Dispatcher * dispatcher = objects.GetDispatcher(objHnd);
	feat_enums featList[1000] = {};
	uint32_t numFeats = feats.FeatListElective(objHnd, featList);

	for (uint32_t i = 0; i < numFeats; i++)
	{
		CondStruct * cond;
		uint32_t arg2 = 0;
		if (_GetCondStructFromFeat(featList[i], &cond, &arg2))
		{
			_ConditionAddToAttribs_NumArgs2(dispatcher, cond, featList[i], arg2);
		}
	}
	_ConditionAddToAttribs_NumArgs0(dispatcher, conds.ConditionAttackOfOpportunity);
	_ConditionAddToAttribs_NumArgs0(dispatcher, conds.ConditionCastDefensively);
	_ConditionAddToAttribs_NumArgs0(dispatcher, conds.ConditionDealSubdualDamage);
	_ConditionAddToAttribs_NumArgs0(dispatcher, conds.ConditionDealNormalDamage);
	_ConditionAddToAttribs_NumArgs0(dispatcher, conds.ConditionFightDefensively);
}

void D20StatusSystem::initItemConditions(objHndl objHnd)
{
	Dispatcher * dispatcher = objects.GetDispatcher(objHnd);
	if (dispatcher == 0 || (int32_t)dispatcher == -1){ return; }
	if (objects.IsCritter(objHnd))
	{
		objects.dispatch.DispatcherClearItemConds(dispatcher);
		if (!objects.d20.d20Query(objHnd, DK_QUE_Polymorphed))
		{
			uint32_t invenCount = objects.getInt32(objHnd, obj_f_critter_inventory_num);
			for (uint32_t i = 0; i < invenCount; i++)
			{
				objHndl objHndItem = templeFuncs.Obj_Get_IdxField_ObjHnd(objHnd, obj_f_critter_inventory_list_idx, i);
				uint32_t itemInvocation = objects.getInt32(objHndItem, obj_f_item_inv_location);
				if (inventory.IsItemEffectingConditions(objHndItem, itemInvocation))
				{
					inventory.sub_100FF500(dispatcher, objHndItem, itemInvocation);
				}
			}
		}

	}
	return;
}