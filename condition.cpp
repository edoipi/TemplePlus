#include "stdafx.h"
#include "common.h"
#include "dispatcher.h"
#include "condition.h"
#include "temple_functions.h"
#include "obj.h"
#include "bonus.h"
#include "radialmenu.h"
#include "combat.h"
#include "critter.h"
#include "location.h"
#include "damage.h"

ConditionSystem conds;
CondStructNew conditionDisableAoO;
CondStructNew conditionGreaterTwoWeaponFighting;
CondStructNew condGreaterTWFRanger;
CondStructNew condDivineMight;
CondStructNew condDivineMightBonus;
CondStructNew condRecklessOffense;
CondStructNew condKnockDown;
CondStructNew condDeadlyPrecision;
CondStructNew condPersistentSpell;


CondStructNew condGreaterRage;
CondStructNew condIndomitableWill ;
CondStructNew condTirelessRage;
CondStructNew condMightyRage;
CondStructNew condDisarm;
CondStructNew condImprovedDisarm;

// monsters
CondStructNew condRend;

CondStructNew condGreaterWeaponSpecialization;


struct ConditionSystemAddresses : AddressTable
{
	void(__cdecl* SetPermanentModArgsFromDataFields)(Dispatcher* dispatcher, CondStruct* condStruct, int* condArgs);
	ConditionSystemAddresses()
	{
		rebase(SetPermanentModArgsFromDataFields, 0x100E1B90);
	}
} addresses;

class ConditionFunctionReplacement : public TempleFix {
public:
	const char* name() override {
		return "Condition Function Replacement";
	}

	void apply() override {
		logger->info("Replacing Condition-related Functions");
		
		replaceFunction(0x100E19C0, _CondStructAddToHashtable);
		replaceFunction(0x100E1A80, _GetCondStructFromHashcode);
		replaceFunction(0x100E1AB0, _CondNodeGetArg);
		replaceFunction(0x100E1AD0, _CondNodeSetArg);
		replaceFunction(0x100E1DD0, _CondNodeAddToSubDispNodeArray);

		replaceFunction(0x100E22D0, _ConditionAddDispatch);
		replaceFunction(0x100E24C0, _ConditionAddToAttribs_NumArgs0);
		replaceFunction(0x100E2500, _ConditionAddToAttribs_NumArgs2);
		replaceFunction(0x100E24E0, _ConditionAdd_NumArgs0);
		replaceFunction(0x100E2530, _ConditionAdd_NumArgs2);
		replaceFunction(0x100E2560, _ConditionAdd_NumArgs3);
		replaceFunction(0x100E2590, _ConditionAdd_NumArgs4);
		replaceFunction(0x100E25C0, InitCondFromCondStructAndArgs);
		
		replaceFunction(0x100EABB0, BarbarianRageStatBonus);
		replaceFunction(0x100EABE0, BarbarianRageSaveBonus);
		
		replaceFunction(0x100ECF30, ConditionPrevent);
		replaceFunction(0x100EE050, GlobalGetArmorClass);
		replaceFunction(0x100EE280, GlobalToHitBonus);
		

		replaceFunction(0x100F7B60, _FeatConditionsRegister);
		replaceFunction(0x100F7BE0, _GetCondStructFromFeat);

		replaceFunction(0x100F7D60, CombatExpertiseRadialMenu);
		replaceFunction(0x100F7F00, CombatExpertiseSet);
		replaceFunction(0x100F88C0, TwoWeaponFightingBonus);
		replaceFunction(0x100F8940, TwoWeaponFightingBonusRanger);
		replaceFunction(0x100FEBA0, BarbarianDamageResistance);

		
		replaceFunction(0x10101150, SkillBonusCallback);
	}
} condFuncReplacement;


CondNode::CondNode(CondStruct *cond) {
	memset(this, 0, sizeof(CondNode));
	condStruct = cond;
}


#pragma region Condition Add Functions

int32_t _CondNodeGetArg(CondNode* condNode, uint32_t argIdx)
{
	return conds.CondNodeGetArg(condNode, argIdx);
}

void _CondNodeSetArg(CondNode* condNode, uint32_t argIdx, uint32_t argVal)
{
	conds.CondNodeSetArg(condNode, argIdx, argVal);
}

uint32_t _ConditionAddDispatch(Dispatcher* dispatcher, CondNode** ppCondNode, CondStruct* condStruct, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
	assert(condStruct->numArgs >= 0 && condStruct->numArgs <= 6);

	vector<int> args;
	if (condStruct->numArgs > 0) {
		args.push_back(arg1);
	}
	if (condStruct->numArgs > 1) {
		args.push_back(arg2);
	}
	if (condStruct->numArgs > 2) {
		args.push_back(arg3);
	}
	if (condStruct->numArgs > 3) {
		args.push_back(arg4);
	}


	return _ConditionAddDispatchArgs(dispatcher, ppCondNode, condStruct, args);
};

uint32_t _ConditionAddDispatchArgs(Dispatcher* dispatcher, CondNode** ppCondNode, CondStruct* condStruct, const vector<int> &args) {
	assert(condStruct->numArgs >= args.size());

	// pre-add section (may abort adding condition, or cause another condition to be deleted first)
	DispIoCondStruct dispIO14h;
	dispIO14h.dispIOType = dispIoTypeCondStruct;
	dispIO14h.condStruct = condStruct;
	dispIO14h.outputFlag = 1;
	dispIO14h.arg1 = 0;
	dispIO14h.arg2 = 0;
	if (args.size() > 0) {
		dispIO14h.arg1 = args[0];
	}
	if (args.size() > 1) {
		dispIO14h.arg2 = args[1];
	}

	_DispatcherProcessor(dispatcher, dispTypeConditionAddPre, 0, (DispIO*)&dispIO14h);

	if (dispIO14h.outputFlag == 0) {
		return 0;
	}

	// adding condition
	auto condNodeNew = new CondNode(condStruct);
	for (unsigned int i = 0; i < condStruct->numArgs; ++i) {
		if (i < args.size()) {
			condNodeNew->args[i] = args[i];
		} else {
			// Fill the rest with zeros
			condNodeNew->args[i] = 0;
		}
	}

	CondNode** ppNextCondeNode = ppCondNode;

	while (*ppNextCondeNode != nullptr) {
		ppNextCondeNode = &(*ppNextCondeNode)->nextCondNode;
	}
	*ppNextCondeNode = condNodeNew;

	_CondNodeAddToSubDispNodeArray(dispatcher, condNodeNew);


	auto dispatcherSubDispNodeType1 = dispatcher->subDispNodes[1];
	while (dispatcherSubDispNodeType1 != nullptr) {
		if (dispatcherSubDispNodeType1->subDispDef->dispKey == 0
			&& (dispatcherSubDispNodeType1->condNode->flags & 1) == 0
			&& condNodeNew == dispatcherSubDispNodeType1->condNode) {
			dispatcherSubDispNodeType1->subDispDef->dispCallback(dispatcherSubDispNodeType1, dispatcher->objHnd, dispTypeConditionAdd, 0, nullptr);
		}

		dispatcherSubDispNodeType1 = dispatcherSubDispNodeType1->next;
	}

	return 1;
};

void _CondNodeAddToSubDispNodeArray(Dispatcher* dispatcher, CondNode* condNode) {
	auto subDispDef = condNode->condStruct->subDispDefs;

	while (subDispDef->dispType != 0) {
		auto subDispNodeNew = (SubDispNode *)allocFuncs._malloc_0(sizeof(SubDispNode));
		subDispNodeNew->subDispDef = subDispDef;
		subDispNodeNew->next = nullptr;
		subDispNodeNew->condNode = condNode;


		auto dispType = subDispDef->dispType;
		assert(dispType >= 0 && dispType < dispTypeCount);

		auto ppDispatcherSubDispNode = &(dispatcher->subDispNodes[dispType]);

		if (*ppDispatcherSubDispNode != nullptr) {
			while ((*ppDispatcherSubDispNode)->next != nullptr) {
				ppDispatcherSubDispNode = &((*ppDispatcherSubDispNode)->next);
			}
			(*ppDispatcherSubDispNode)->next = subDispNodeNew;
		}
		else {
			dispatcher->subDispNodes[subDispDef->dispType] = subDispNodeNew;
		}


		subDispDef += 1;
	}

};


uint32_t _ConditionAddToAttribs_NumArgs0(Dispatcher* dispatcher, CondStruct* condStruct) {
	return _ConditionAddDispatch(dispatcher, &dispatcher->permanentMods, condStruct, 0, 0, 0, 0);
};

uint32_t _ConditionAddToAttribs_NumArgs2(Dispatcher* dispatcher, CondStruct* condStruct, uint32_t arg1, uint32_t arg2) {
	return _ConditionAddDispatch(dispatcher, &dispatcher->permanentMods, condStruct, arg1, arg2, 0, 0);
};

uint32_t _ConditionAdd_NumArgs0(Dispatcher* dispatcher, CondStruct* condStruct) {
	return _ConditionAddDispatch(dispatcher, &dispatcher->conditions, condStruct, 0, 0, 0, 0);
};

uint32_t _ConditionAdd_NumArgs1(Dispatcher* dispatcher, CondStruct* condStruct, uint32_t arg1) {
	return _ConditionAddDispatch(dispatcher, &dispatcher->conditions, condStruct, arg1, 0, 0, 0);
};

uint32_t _ConditionAdd_NumArgs2(Dispatcher* dispatcher, CondStruct* condStruct, uint32_t arg1, uint32_t arg2) {
	return _ConditionAddDispatch(dispatcher, &dispatcher->conditions, condStruct, arg1, arg2, 0, 0);
};

uint32_t _ConditionAdd_NumArgs3(Dispatcher* dispatcher, CondStruct* condStruct, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
	return _ConditionAddDispatch(dispatcher, &dispatcher->conditions, condStruct, arg1, arg2, arg3, 0);
};

uint32_t _ConditionAdd_NumArgs4(Dispatcher* dispatcher, CondStruct* condStruct, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
	return _ConditionAddDispatch(dispatcher, &dispatcher->conditions, condStruct, arg1, arg2, arg3, arg4);
}

void InitCondFromCondStructAndArgs(Dispatcher* dispatcher, CondStruct* condStruct, int* condargs)
{
	conds.InitCondFromCondStructAndArgs(dispatcher, condStruct, condargs);
};

#pragma endregion

int ConditionPrevent(DispatcherCallbackArgs args)
{
	DispIoCondStruct * dispIO = _DispIoCheckIoType1((DispIoCondStruct*)args.dispIO);
	if (dispIO == nullptr)
	{
		logger->error("Dispatcher Error! Condition {} fuckup, wrong DispIO type", args.subDispNode->condNode->condStruct->condName);
		return 0; // if we get here then VERY BAD!
	}
	if (dispIO->condStruct == (CondStruct *)args.subDispNode->subDispDef->data1)
	{
		dispIO->outputFlag = 0;
	}
	return 0;
};

int ConditionPreventWithArg(DispatcherCallbackArgs args)
{
	int arg1 = arg1 = conds.CondNodeGetArg(args.subDispNode->condNode, 0);
	DispIoCondStruct *dispIo = dispatch.DispIoCheckIoType1((DispIoCondStruct *)args.dispIO);;
	
	if (dispIo->condStruct == (CondStruct *)args.subDispNode->subDispDef->data1 && dispIo->arg1 == arg1)
		dispIo->outputFlag = 0;
	return 0;
}


int ConditionRemoveCallback(DispatcherCallbackArgs args)
{
	conds.ConditionRemove(args.objHndCaller, args.subDispNode->condNode);
	return 0;
};

int DivineMightEffectTooltipCallback(DispatcherCallbackArgs args)
{
	void * dispIo = args.dispIO;
	int (__cdecl* callback )(int, int, int)= (int(__cdecl*)(int, int, int))temple_address(0x100F4760);
	const char shit[] = "Divine Might";
	callback( *((int*)dispIo + 1), args.subDispNode->subDispDef->data1, (int)shit);
	return 0;
};

int __cdecl CondNodeSetArg0FromSubDispDef(DispatcherCallbackArgs args)
{
	conds.CondNodeSetArg(args.subDispNode->condNode, 0, args.subDispNode->subDispDef->data1);
	return 0;
};

int __cdecl CondNodeSetArgFromSubDispDef(DispatcherCallbackArgs args)
{
	// sets arg[data1] from data2  e.g. data1 = 0 data2 = 15  will set arg0 to 15
	conds.CondNodeSetArg(args.subDispNode->condNode, args.subDispNode->subDispDef->data1,
		args.subDispNode->subDispDef->data2);
	return 0;
};


int __cdecl CondArgDecrement(DispatcherCallbackArgs args)
{
	conds.CondNodeSetArg(args.subDispNode->condNode, args.subDispNode->subDispDef->data1, 
		conds.CondNodeGetArg(args.subDispNode->condNode, args.subDispNode->subDispDef->data1) - 1);
	return 0;
};

int __cdecl SkillBonusCallback(DispatcherCallbackArgs args)
{
	/*
	used by conditions: Skill Circumstance Bonus, Skill Competence Bonus
	*/
	SkillEnum skillEnum = (SkillEnum)conds.CondNodeGetArg(args.subDispNode->condNode, 0);
	int bonValue = conds.CondNodeGetArg(args.subDispNode->condNode, 1);
	int bonType = args.subDispNode->subDispDef->data1;
	if (args.dispKey - 20 == skillEnum)
	{
		int invIdx = conds.CondNodeGetArg(args.subDispNode->condNode, 2);
		objHndl itemHnd = inventory.GetItemAtInvIdx(args.objHndCaller, invIdx);
		DispIoBonusAndObj * dispIo = dispatch.DispIoCheckIoType10((DispIoBonusAndObj*)args.dispIO);
		const char * name = description.getDisplayName(itemHnd, args.objHndCaller);
		bonusSys.bonusAddToBonusListWithDescr(dispIo->bonOut, bonValue, bonType, 112, (char*)name);
	}
	return 0;
}

int __cdecl GlobalToHitBonus(DispatcherCallbackArgs args)
{
	DispIoAttackBonus * dispIo = dispatch.DispIoCheckIoType5((DispIoAttackBonus*)args.dispIO);
	dispatch.DispatchToHitBonusBase(args.objHndCaller, dispIo);

	// natural attack - get attack bonus from internal defs
	if (dispIo->attackPacket.dispKey >= (ATTACK_CODE_NATURAL_ATTACK+1) 
		 && !d20Sys.d20Query(args.objHndCaller, DK_QUE_Polymorphed) )
	{
		int attackIdx = dispIo->attackPacket.dispKey - (ATTACK_CODE_NATURAL_ATTACK + 1);
		int bonValue = 0; // temporarily used as an index value for obj_f_attack_bonus_idx field
		for (int i = 0,  j=0; i < 4; i++)
		{
			j += objects.getArrayFieldInt32(args.objHndCaller, obj_f_critter_attacks_idx, i); // number of attacks
			if (attackIdx < j){
				bonValue = i;
				break;
			}
		}
		bonValue = objects.getArrayFieldInt32(args.objHndCaller, obj_f_attack_bonus_idx, bonValue);
		bonusSys.bonusAddToBonusList(&dispIo->bonlist, bonValue, 1, 118); // base attack
	}

	if (dispIo->attackPacket.flags & D20CAF_RANGED) // get to hit modifier from DEX
	{
		bonusSys.bonusAddToBonusList(&dispIo->bonlist, 
			objects.GetModFromStatLevel(objects.abilityScoreLevelGet(args.objHndCaller, stat_dexterity, 0)) , 3, 104);
	} else // get to hit mod from STR
	{
		bonusSys.bonusAddToBonusList(&dispIo->bonlist,
			objects.GetModFromStatLevel(objects.abilityScoreLevelGet(args.objHndCaller, stat_strength, 0)), 2, 103);
	}

	int attackCode = dispIo->attackPacket.dispKey;
	if (attackCode < ATTACK_CODE_NATURAL_ATTACK) // apply penalties for Nth attack
	{
		int attackNumber = 1;
		int dualWielding = 0;
		d20Sys.ExtractAttackNumber(args.objHndCaller, attackCode, &attackNumber, &dualWielding);
		if (attackNumber <= 0)
		{
			int dummy = 1;
			assert(attackNumber > 0);
		}
		switch (attackNumber)
		{
		case 1: 
			break;
		case 2:
			bonusSys.bonusAddToBonusList(&dispIo->bonlist, -(attackNumber-1) * 5, 24, 119);
			break;
		default:
			bonusSys.bonusAddToBonusList(&dispIo->bonlist, -(attackNumber - 1) * 5, 25, 120);
		}
		if (dualWielding)
		{
			if (d20Sys.UsingSecondaryWeapon(args.objHndCaller, attackCode))
				bonusSys.bonusAddToBonusList(&dispIo->bonlist, -10, 26, 121); // penalty for dualwield on offhand attack
			else
				bonusSys.bonusAddToBonusList(&dispIo->bonlist, -6, 27, 122); // penalty for dualwield on primary attack

			auto offhand = inventory.ItemWornAt(args.objHndCaller, 4);
			if (offhand)
			{
				if (inventory.GetWieldType(dispIo->attackPacket.attacker, offhand) == 0)
					bonusSys.bonusAddToBonusList(&dispIo->bonlist, 2, 0, 167); // Light Off-hand Weapon
			}
		}
	}

	// helplessness bonus
	if (dispIo->attackPacket.victim
		&& d20Sys.d20Query(dispIo->attackPacket.victim, DK_QUE_Helpless)
		&& !d20Sys.d20Query(dispIo->attackPacket.victim, DK_QUE_Critter_Is_Stunned))
		bonusSys.bonusAddToBonusList(&dispIo->bonlist, 4, 30, 136);
	
	// flanking bonus
	if (combatSys.IsFlankedBy(dispIo->attackPacket.victim, dispIo->attackPacket.attacker))
	{
		bonusSys.bonusAddToBonusList(&dispIo->bonlist, 2, 0, 201);
		*(int*)(&dispIo->attackPacket.flags ) |= (int)D20CAF_FLANKED;
	}

	// size bonus / penalty
	int sizeCategory = dispatch.DispatchGetSizeCategory(args.objHndCaller);
	int sizeCatBonus = critterSys.GetBonusFromSizeCategory(sizeCategory);
	bonusSys.bonusAddToBonusList(&dispIo->bonlist, sizeCatBonus, 0, 115);

	if (dispIo->attackPacket.flags & D20CAF_RANGED)
	{

		if (dispIo->attackPacket.victim)
		{
			// firing into melee penalty
			objHndl canMeleeList[100];

			int numEnemiesCanMelee = combatSys.GetEnemiesCanMelee(dispIo->attackPacket.victim, canMeleeList);
			if (numEnemiesCanMelee > 0
				&& (numEnemiesCanMelee != 1 || canMeleeList[0]!= args.objHndCaller)
				&& !feats.HasFeatCount(args.objHndCaller, FEAT_PRECISE_SHOT))
				bonusSys.bonusAddToBonusList(&dispIo->bonlist, -4, 0, 150); 
		
			// range penalty 
			objHndl weapon = combatSys.GetWeapon(&dispIo->attackPacket);
			float dist = locSys.DistanceToObj(args.objHndCaller, dispIo->attackPacket.victim);
			if (dist < 0.0) dist = 0.0;
			if (weapon)
			{
				long double weaponRange = (long double)objects.getInt32(weapon, obj_f_weapon_range);
				int rangePenaltyFacotr = (int)(dist / weaponRange);
				if ((int)rangePenaltyFacotr > 0)
					bonusSys.bonusAddToBonusList(&dispIo->bonlist, -2 * rangePenaltyFacotr, 0, 303);
			}
		}
	}

	return 0;
}

int GlobalGetArmorClass(DispatcherCallbackArgs args) // the basic AC value (initial value and some misc. global modifiers)
{
	DispIoAttackBonus * dispIo = dispatch.DispIoCheckIoType5((DispIoAttackBonus*)args.dispIO);
	BonusList *bonlist = &dispIo->bonlist;

	// Armor Class initial value 10
	bonusSys.bonusAddToBonusList(bonlist, 10, 1, 102); 
	
	// add npc natural armor AC bonus
	if (!(dispIo->attackPacket.flags & D20CAF_TOUCH_ATTACK))
	{
		objHndl defender = args.objHndCaller;
		int polymorphedTo = d20Sys.d20Query(args.objHndCaller, DK_QUE_Polymorphed);
		if (polymorphedTo)
			defender = objects.GetProtoHandle(polymorphedTo);
		if (objects.GetType(args.objHndCaller) == obj_t_npc || polymorphedTo)
		{
			bonusSys.bonusAddToBonusList(bonlist, objects.getInt32(defender, obj_f_npc_ac_bonus), 9, 123);
		}
	}

	// add size bonus / penalty to AC
	int sizeCat = dispatch.DispatchGetSizeCategory(args.objHndCaller);
	int sizeCatBonus = critterSys.GetBonusFromSizeCategory(sizeCat);
	bonusSys.bonusAddToBonusList(bonlist, sizeCatBonus, 0, 115); 

	// dex bonus
	int dexScore = objects.abilityScoreLevelGet(args.objHndCaller, stat_dexterity, 0);
	int dexMod = objects.GetModFromStatLevel(dexScore);
	bonusSys.bonusAddToBonusList(bonlist, dexMod, 3, 104);

	// dodging trap
	if (dispIo->attackPacket.flags & D20CAF_TRAP)
	{
		if (dispIo->attackPacket.victim && feats.HasFeatCount(dispIo->attackPacket.victim, FEAT_UNCANNY_DODGE))
		{
			bonusSys.zeroBonusSetMeslineNum(bonlist, 165); // dex bonus retained due to Uncanny Dodge
			return 0;
		}
		bonusSys.bonusCapAdd(bonlist, 8, 0, 0x99u); // flatfooted
		bonusSys.bonusCapAdd(bonlist, 3, 0, 0x99u);
	}

	if (dispIo->attackPacket.flags & D20CAF_COVER)
	{
		if (dispIo->attackPacket.attacker && feats.HasFeatCount(dispIo->attackPacket.attacker, FEAT_IMPROVED_PRECISE_SHOT))
		{
			bonusSys.zeroBonusSetMeslineNum(bonlist, 335); // Cover negated by Imp. Precise Shot
			return 0;
		}

		if (dispIo->attackPacket.attacker && feats.HasFeatCountByClass(dispIo->attackPacket.attacker, FEAT_IMPROVED_PRECISE_SHOT_RANGER, (Stat)0, 0))
		{
			if (critterSys.IsWearingLightOrNoArmor(dispIo->attackPacket.attacker))
			{
				bonusSys.zeroBonusSetMeslineNum(bonlist, 335); // Cover negated by Imp. Precise Shot
				return 0;
			}
						
		}

		if (dispIo->attackPacket.attacker && feats.HasFeatCount(dispIo->attackPacket.attacker, FEAT_SHARP_SHOOTING))
		{
			bonusSys.bonusAddToBonusList(bonlist, 2, 0, 336);; // Cover (diminished by Sharp-Shooting)
			return 0;
		}
		bonusSys.bonusAddToBonusList(bonlist, 4, 0, 309);
	}
		
	return 0;
}

void _FeatConditionsRegister()
{
	conds.hashmethods.CondStructAddToHashtable(conds.ConditionAttackOfOpportunity);
	conds.hashmethods.CondStructAddToHashtable(conds.ConditionCastDefensively);
	conds.hashmethods.CondStructAddToHashtable(conds.ConditionCombatCasting);
	conds.hashmethods.CondStructAddToHashtable(conds.ConditionDealSubdualDamage );
	conds.hashmethods.CondStructAddToHashtable(conds.ConditionDealNormalDamage);
	conds.hashmethods.CondStructAddToHashtable(conds.ConditionFightDefensively);
	conds.hashmethods.CondStructAddToHashtable(conds.ConditionAnimalCompanionAnimal);
	conds.hashmethods.CondStructAddToHashtable(conds.ConditionAutoendTurn);

	// New Conditions!
	conds.hashmethods.CondStructAddToHashtable((CondStruct*)conds.mConditionDisableAoO);
	conds.hashmethods.CondStructAddToHashtable((CondStruct*)conds.mCondGreaterTwoWeaponFighting);
	conds.hashmethods.CondStructAddToHashtable((CondStruct*)conds.mCondGreaterTWFRanger);
	conds.hashmethods.CondStructAddToHashtable((CondStruct*)conds.mCondDivineMight);
	conds.hashmethods.CondStructAddToHashtable((CondStruct*)conds.mCondDivineMightBonus);
	conds.hashmethods.CondStructAddToHashtable((CondStruct*)conds.mCondRecklessOffense);
	conds.hashmethods.CondStructAddToHashtable((CondStruct*)conds.mCondGreaterWeaponSpecialization);
	// conds.hashmethods.CondStructAddToHashtable((CondStruct*)conds.mCondSuperiorExpertise); // will just be patched inside Combat Expertise callbacks

	/*
	conds.hashmethods.CondStructAddToHashtable((CondStruct*)conds.mCondDeadlyPrecision);
	conds.hashmethods.CondStructAddToHashtable((CondStruct*)conds.mCondDisarm);
	conds.hashmethods.CondStructAddToHashtable((CondStruct*)conds.mCondGreaterRage);
	conds.hashmethods.CondStructAddToHashtable((CondStruct*)conds.mCondImprovedDisarm);
	conds.hashmethods.CondStructAddToHashtable((CondStruct*)conds.mCondIndomitableWill);
	conds.hashmethods.CondStructAddToHashtable((CondStruct*)conds.mCondKnockDown);
	conds.hashmethods.CondStructAddToHashtable((CondStruct*)conds.mCondMightyRage);
	conds.hashmethods.CondStructAddToHashtable((CondStruct*)conds.mCondPersistentSpell);
	
	conds.hashmethods.CondStructAddToHashtable((CondStruct*)conds.mCondRend);
	conds.hashmethods.CondStructAddToHashtable((CondStruct*)conds.mCondTirelessRage);
	*/

	for (unsigned int i = 0; i < 84; i++)
	{
		conds.hashmethods.CondStructAddToHashtable(conds.FeatConditionDict[i].condStruct);
	}
}

uint32_t  _GetCondStructFromFeat(feat_enums featEnum, CondStruct ** condStructOut, uint32_t * arg2Out)
{
	switch (featEnum)
	{
	case FEAT_GREATER_TWO_WEAPON_FIGHTING:
		*condStructOut = (CondStruct*)conds.mCondGreaterTwoWeaponFighting;
		*arg2Out = 0;
		return 1;
	case FEAT_GREATER_TWO_WEAPON_FIGHTING_RANGER:
		*condStructOut = (CondStruct*)conds.mCondGreaterTWFRanger;
		*arg2Out = 0;
		return 1;
	case FEAT_DIVINE_MIGHT:
		*condStructOut = (CondStruct*)conds.mCondDivineMight;
		*arg2Out = 0;
		return 1;
	case FEAT_RECKLESS_OFFENSE:
		*condStructOut = (CondStruct*)conds.mCondRecklessOffense;
		*arg2Out = 0;
		return 1;
	case FEAT_KNOCK_DOWN:
		*condStructOut = (CondStruct*)conds.mCondKnockDown;
		*arg2Out = 0;
		return 1;
	case FEAT_SUPERIOR_EXPERTISE:
		return 0; // willl just be patched inside Combat Expertise
	case FEAT_DEADLY_PRECISION:
		*condStructOut = (CondStruct*)conds.mCondDeadlyPrecision;
		*arg2Out = 0;
		return 1;
	case FEAT_PERSISTENT_SPELL:
		*condStructOut = (CondStruct*)conds.mCondPersistentSpell;
		*arg2Out = 0;
		return 1;
	default:
		break;
	}

	if (featEnum >= FEAT_GREATER_WEAPON_SPECIALIZATION_GAUNTLET && featEnum <= FEAT_GREATER_WEAPON_SPECIALIZATION_GRAPPLE)
	{
		*condStructOut = (CondStruct*)conds.mCondGreaterWeaponSpecialization;
		*arg2Out = featEnum - FEAT_GREATER_WEAPON_SPECIALIZATION_GAUNTLET;
		return 1;
	}

	feat_enums * featFromDict = & ( conds.FeatConditionDict->featEnum );
	uint32_t iter = 0;
	while (
		( (int32_t)featEnum != featFromDict[0] || featFromDict[1] != -1)
		&&  ( (int32_t)featEnum < (int32_t)featFromDict[0] 
				|| (int32_t)featEnum >= (int32_t)featFromDict[1]  )
		)
	{
		iter += 16;
		featFromDict += 4;
		if (iter >= 0x540){ return 0; }
	}

	*condStructOut = (CondStruct *)*(featFromDict - 1);
	*arg2Out = featEnum + featFromDict[2] - featFromDict[0];
	return 1;
}

uint32_t _CondStructAddToHashtable(CondStruct * condStruct)
{
	return conds.hashmethods.CondStructAddToHashtable(condStruct);
}

CondStruct * _GetCondStructFromHashcode(uint32_t key)
{
	return conds.hashmethods.GetCondStruct(key);
}

CondStruct* ConditionSystem::GetByName(const string& name) {
	auto key = templeFuncs.StringHash(name.c_str());
	return hashmethods.GetCondStruct(key);
}

void ConditionSystem::AddToItem(objHndl item, const CondStruct* cond, const vector<int>& args) {
	assert(args.size() == cond->numArgs);

	auto curCondCount = templeFuncs.Obj_Get_IdxField_NumItems(item, obj_f_item_pad_wielder_condition_array);
	auto curCondArgCount = templeFuncs.Obj_Get_IdxField_NumItems(item, obj_f_item_pad_wielder_argument_array);

	// Add the condition name hash to the list
	auto key = templeFuncs.StringHash(cond->condName);
	templeFuncs.Obj_Set_IdxField_byValue(item, obj_f_item_pad_wielder_condition_array, curCondCount, key);

	auto idx = curCondArgCount;
	for (auto arg : args) {
		templeFuncs.Obj_Set_IdxField_byValue(item, obj_f_item_pad_wielder_argument_array, idx, arg);
		idx++;
	}
}

bool ConditionSystem::AddTo(objHndl handle, const CondStruct* cond, const vector<int>& args) {
	assert(args.size() == cond->numArgs);

	auto dispatcher = objects.GetDispatcher(handle);

	if (!dispatcher) {
		return false;
	}

	return _ConditionAddDispatchArgs(dispatcher, &dispatcher->conditions, const_cast<CondStruct*>(cond), args) != 0;
}

bool ConditionSystem::AddTo(objHndl handle, const string& name, const vector<int>& args) {
	auto cond = GetByName(name);
	if (!cond) {
		logger->warn("Unable to find condition {}", name);
		return false;
	}

	return AddTo(handle, cond, args);
}

int32_t ConditionSystem::CondNodeGetArg(CondNode* condNode, uint32_t argIdx)
{
	if (argIdx < condNode->condStruct->numArgs)
	{
		return condNode->args[argIdx];
	}
	return 0;
}

void ConditionSystem::CondNodeSetArg(CondNode* condNode, uint32_t argIdx, uint32_t argVal)
{
	if (argIdx < condNode->condStruct->numArgs)
	{
		condNode->args[argIdx] = argVal;
	}
}

void ConditionSystem::CondNodeAddToSubDispNodeArray(Dispatcher* dispatcher, CondNode* condNodeNew)
{
	_CondNodeAddToSubDispNodeArray(dispatcher, condNodeNew);
}

void ConditionSystem::InitCondFromCondStructAndArgs(Dispatcher* dispatcher, CondStruct* condStruct, int* condargs)
{
	CondNode **v4; 
	SubDispNode *subDispNode; 
	CondNode *condNode; 

	auto *condNodeNew = new CondNode(condStruct);
	v4 = &dispatcher->conditions;
	while (*v4)
	{
		v4 = & (*v4)->nextCondNode;
	}
	*v4 = condNodeNew;

	for (auto i = 0; i < condStruct->numArgs; i++)
	{
		condNodeNew->args[i] = condargs[i];
	}
	conds.CondNodeAddToSubDispNodeArray(dispatcher, condNodeNew);
	for (subDispNode = dispatcher->subDispNodes[dispTypeConditionAddFromD20StatusInit]; subDispNode; subDispNode = subDispNode->next)
	{
		if (subDispNode->subDispDef->dispKey == 0)
		{
			condNode = subDispNode->condNode;
			if (!(condNode->flags & 1) && condNode == condNodeNew)
				subDispNode->subDispDef->dispCallback(subDispNode,dispatcher->objHnd, dispTypeConditionAddFromD20StatusInit, 0, nullptr);
		}
	}
}

void ConditionSystem::RegisterNewConditions()
{
	CondStructNew * cond;
	char * condName;

	// Disable AoO
	mConditionDisableAoO = &conditionDisableAoO;
	cond = mConditionDisableAoO; 	condName = mConditionDisableAoOName;
	memset(condName, 0, sizeof(condName)); 	memcpy(condName, "Disable AoO", sizeof("Disable AoO"));

	cond->condName = mConditionDisableAoOName;
	cond->numArgs = 1;

	DispatcherHookInit(cond, 0, dispTypeD20Query, DK_QUE_AOOPossible, AoODisableQueryAoOPossible,	0, 0);
	DispatcherHookInit(cond, 1, dispTypeRadialMenuEntry, 0, AoODisableRadialMenuInit, 0, 0);
	DispatcherHookInit(cond, 2, dispTypeConditionAddPre, 0, ConditionPrevent, (uint32_t)mConditionDisableAoO, 0);
	DispatcherHookInit(cond, 3, dispType0, 0, 0, 0, 0);

	// Greater Two Weapon Fighting
	mCondGreaterTwoWeaponFighting = &conditionGreaterTwoWeaponFighting;
	cond = mCondGreaterTwoWeaponFighting; 	condName = mConditionGreaterTwoWeaponFightingName;
	memset(condName, 0, sizeof(condName));
	memcpy(condName, "Greater Two Weapon Fighting", sizeof("Greater Two Weapon Fighting"));

	cond->condName = mConditionGreaterTwoWeaponFightingName;
	cond->numArgs = 2;

	DispatcherHookInit(cond, 0, dispTypeConditionAddPre, 0, ConditionPrevent, (uint32_t)mCondGreaterTwoWeaponFighting, 0);
	DispatcherHookInit(cond, 1, dispTypeConditionAddPre, 0, ConditionPrevent, (uint32_t)mCondGreaterTWFRanger, 0);
	DispatcherHookInit(cond, 2, dispTypeGetNumAttacksBase, 0, GreaterTwoWeaponFighting, 0, 0); // same callback as Improved TWF (it just adds an extra attack... logic is inside the action sequence / d20 / GlobalToHit functions
	DispatcherHookInit(cond, 3, dispType0, 0, nullptr, 0, 0);

	// Greater TWF Ranger
	mCondGreaterTWFRanger = &condGreaterTWFRanger;
	cond = mCondGreaterTWFRanger; 	condName = mCondGreaterTWFRangerName;
	memset(condName, 0, sizeof(condName));
	memcpy(condName, "Greater Two Weapon Fighting Ranger", sizeof("Greater Two Weapon Fighting Ranger"));

	cond->condName = mCondGreaterTWFRangerName;
	cond->numArgs = 2;

	DispatcherHookInit(cond, 0, dispTypeConditionAddPre, 0, ConditionPrevent, (uint32_t)mCondGreaterTwoWeaponFighting, 0);
	DispatcherHookInit(cond, 1, dispTypeConditionAddPre, 0, ConditionPrevent, (uint32_t)mCondGreaterTWFRanger, 0); // TODO: add TWF_RANGER
	DispatcherHookInit(cond, 2, dispTypeGetNumAttacksBase, 0, GreaterTWFRanger, 0, 0); // same callback as Improved TWF (it just adds an extra attack... logic is inside the action sequence / d20 / GlobalToHit functions
	DispatcherHookInit(cond, 3, dispType0, 0, nullptr, 0, 0);

	// Divine Might Ability
	mCondDivineMight = &condDivineMight;
	cond = mCondDivineMight; 	condName = mCondDivineMightName;
	memset(condName, 0, sizeof(condName)); 	memcpy(condName, "Divine Might", sizeof("Divine Might"));

	cond->condName = condName;
	cond->numArgs = 2;

	DispatcherHookInit(cond, 0, dispTypeConditionAddPre, 0 ,ConditionPrevent, (uint32_t) cond , 0 );
	DispatcherHookInit(cond, 1, dispTypeConditionAdd, 0, CondNodeSetArg0FromSubDispDef, 1, 0);
	DispatcherHookInit(cond, 2, dispTypeRadialMenuEntry, 0, DivineMightRadial, 0, 0);

	DispatcherHookInit((CondStructNew*)ConditionTurnUndead, 6, dispTypeD20ActionPerform, DK_D20A_DIVINE_MIGHT, CondArgDecrement, 1, 0); // decrement the number of turn charges remaining; 
	DispatcherHookInit((CondStructNew*)ConditionGreaterTurning, 6, dispTypeD20ActionPerform, DK_D20A_DIVINE_MIGHT, CondArgDecrement, 1, 0); // decrement the number of turn charges remaining
	
	// Divine Might Bonus (gets activated when you choose the action from the Radial Menu)
	mCondDivineMightBonus = &condDivineMightBonus;
	cond = mCondDivineMightBonus; 	condName = mCondDivineMightBonusName;
	memset(condName, 0, sizeof(condName)); 	memcpy(condName, "Divine Might Bonus", sizeof("Divine Might Bonus"));

	cond->condName = condName;
	cond->numArgs = 2;

	DispatcherHookInit(cond, 0, dispTypeConditionAddPre, 0, ConditionPrevent, (uint32_t)cond, 0);
	DispatcherHookInit(cond, 1, dispTypeDealingDamage, 0, DivineMightDamageBonus, 0, 0);
	DispatcherHookInit(cond, 2, dispTypeBeginRound, 0, ConditionRemoveCallback, 0, 0);
	DispatcherHookInit(cond, 3, dispTypeD20Signal, DK_SIG_Killed, ConditionRemoveCallback, 0, 0);
	DispatcherHookInit(cond, 4, dispTypeEffectTooltip, 0, DivineMightEffectTooltipCallback, 81, 0);

	// Reckless Offense
	mCondRecklessOffense = &condRecklessOffense;
	cond = mCondRecklessOffense; 	condName = mCondRecklessOffenseName;
	memset(condName, 0, sizeof(condName)); 	memcpy(condName, "Reckless Offense", sizeof("Reckless Offense"));

	cond->condName = condName;
	cond->numArgs = 2;

	DispatcherHookInit(cond, 0, dispTypeConditionAddPre, 0, ConditionPrevent, (uint32_t)cond, 0);
	DispatcherHookInit(cond, 1, dispTypeRadialMenuEntry, 0, RecklessOffenseRadialMenuInit, 0, 0);
	DispatcherHookInit(cond, 2, dispTypeGetAC, 0, RecklessOffenseAcPenalty, 0, 0);
	DispatcherHookInit(cond, 3, dispTypeToHitBonus2, 0, RecklessOffenseToHitBonus, 0, 0);
	DispatcherHookInit(cond, 4, dispTypeD20Signal, DK_SIG_Attack_Made, TacticalOptionAbusePrevention, 0, 0);
	DispatcherHookInit(cond, 5, dispTypeBeginRound, 0, CondNodeSetArgFromSubDispDef, 1, 0);
	DispatcherHookInit(cond, 6, dispTypeConditionAdd, 0, CondNodeSetArgFromSubDispDef, 0, 0);

	// Knock Down
	mCondKnockDown = &condKnockDown;
	cond = mCondKnockDown; 	condName = mCondKnockDownName;
	memset(condName, 0, sizeof(condName)); 	memcpy(condName, "Knock-Down", sizeof("Knock-Down"));

	cond->condName = condName;
	cond->numArgs = 2;

	// Knock Down
	mCondGreaterWeaponSpecialization = &condGreaterWeaponSpecialization;
	cond = mCondGreaterWeaponSpecialization; 	condName = mCondGreaterWeaponSpecializationName;
	memset(condName, 0, sizeof(condName)); 	memcpy(condName, "Greater Weapon Specialization", sizeof("Greater Weapon Specialization"));

	cond->condName = condName;
	cond->numArgs = 2;

	DispatcherHookInit(cond, 0, dispTypeConditionAddPre, 0, ConditionPreventWithArg, (uint32_t)cond, 0);
	DispatcherHookInit(cond, 1, dispTypeDealingDamage, 0, GreaterWeaponSpecializationDamage, 0, 0);

	/*
	char mCondDeadlyPrecisionName[100];
	CondStructNew *mCondDeadlyPrecision;
	char mCondPersistenSpellName[100];
	CondStructNew *mCondPersistentSpell;

	char mCondGreaterRageName[100];
	CondStructNew *mCondGreaterRage;
	char mCondIndomitableWillName[100];
	CondStructNew *mCondIndomitableWill;
	char mCondTirelessRageName[100];
	CondStructNew *mCondTirelessRage;
	char mCondMightyRageName[100];
	CondStructNew *mCondMightyRage;
	char mCondDisarmName[100];
	CondStructNew *mCondDisarm;
	char mCondImprovedDisarmName[100];
	CondStructNew *mCondImprovedDisarm;

	// monsters
	char mCondRendName[100];
	CondStructNew *mCondRend;
	
	*/

}

void ConditionSystem::DispatcherHookInit(SubDispDefNew* sdd, enum_disp_type dispType, int key, void* callback, int data1, int data2)
{
	sdd->dispType = dispType;
	sdd->dispKey = key;
	sdd->dispCallback = ( int(__cdecl*)(DispatcherCallbackArgs))callback;
	sdd->data1 = data1;
	sdd->data2 = data2;
}

void ConditionSystem::DispatcherHookInit(CondStructNew* cond, int hookIdx, enum_disp_type dispType, int key, void* callback, int data1, int data2)
{
	assert(cond->subDispDefs[hookIdx].dispType == 0);
	DispatcherHookInit(&cond->subDispDefs[hookIdx], dispType, key, callback, data1, data2 );
}

void ConditionSystem::SetPermanentModArgsFromDataFields(Dispatcher* dispatcher, CondStruct* condStruct, int* condArgs)
{
	addresses.SetPermanentModArgsFromDataFields(dispatcher, condStruct, condArgs);
}

void ConditionSystem::DispatcherCondsResetFlag2(Dispatcher* dispatcher)
{
	CondNode *condNode; 
	for (condNode = dispatcher->permanentMods; condNode; condNode = condNode->nextCondNode)
		condNode->flags &= 0xFFFFFFFD;
	for (condNode = dispatcher->itemConds; condNode; condNode = condNode->nextCondNode)
		condNode->flags &= 0xFFFFFFFD;
}

int ConditionSystem::GetActiveCondsNum(Dispatcher* dispatcher)
{ 
	int numConds=0; 

	CondNode *cond = dispatcher->conditions;
	while (cond)
	{
		if (!(cond->flags & 1))
			++numConds;
		cond = cond->nextCondNode;
	}
	return numConds;
}

int ConditionSystem::GetPermanentModsAndItemCondCount(Dispatcher* dispatcher)
{
	int numConds = 0;
	CondNode *cond = dispatcher->permanentMods;

	while (cond)
	{
		if (!(cond->flags & 1))
			++numConds;
		cond = cond->nextCondNode;
	}

	cond = dispatcher->itemConds;
	while(cond)
	{
		if (!(cond->flags & 1))
			++numConds;
		cond = cond->nextCondNode;
	}
	return numConds;
}

int ConditionSystem::ConditionsExtractInfo(Dispatcher* dispatcher, int activeCondIdx, int* hashkeyOut, int* condArgsOut)
{
	CondNode *cond;
	int n; 
	int numArgs; 


	cond = dispatcher->conditions;              
	n = 0;
	while (cond)
	{
		if (!(cond->flags & 1))
		{
			if (n == activeCondIdx)
			{
				*hashkeyOut = conds.hashmethods.GetCondStructHashkey(cond->condStruct);
				int numArgs = cond->condStruct->numArgs;
				for (int i = 0; i < numArgs; i++)
				{
					condArgsOut[i] = cond->args[i];
				}
				return cond->condStruct->numArgs;
			}
			++n;
		}
		cond = cond->nextCondNode;
	}
	if (!cond) return 0;

	*hashkeyOut = conds.hashmethods.GetCondStructHashkey(cond->condStruct);
	numArgs = cond->condStruct->numArgs;
	for (int i = 0; i < numArgs; i++)
	{
		condArgsOut[i] = cond->args[i];
	}
	return cond->condStruct->numArgs;
}

int ConditionSystem::PermanentAndItemModsExtractInfo(Dispatcher* dispatcher, int permModIdx, int* hashkeyOut, int* condArgsOut)
{
	
	int i=0; 
	CondNode *cond;

	cond = dispatcher->permanentMods;
	while (cond )
	{
		if (!(cond->flags & 1))
		{
			if (i == permModIdx)
			{
				*hashkeyOut = conds.hashmethods.GetCondStructHashkey(cond->condStruct);
				int numArgs = cond->condStruct->numArgs;
				for (int i = 0; i < numArgs; i++)
				{
					condArgsOut[i] = cond->args[i];
				}
				return cond->condStruct->numArgs;
			}
			++i;
		}
		cond = cond->nextCondNode;
	}


	cond = dispatcher->itemConds;
	while (cond)
	{
		if (! (cond->flags & 1))
		{
			if (i == permModIdx)
			{
				*hashkeyOut = conds.hashmethods.GetCondStructHashkey(cond->condStruct);
				int numArgs = cond->condStruct->numArgs;
				for (i = 0; i < numArgs; i++)
				{
					condArgsOut[i] = cond->args[i];
				}
				return cond->condStruct->numArgs;
			}
			i++;
		}
		cond = cond->nextCondNode;
	}
	if (!cond) return 0;
}

void ConditionSystem::ConditionRemove(objHndl objHnd, CondNode* cond)
{
	Dispatcher * dispatcher = objects.GetDispatcher(objHnd);
	if (dispatch.dispatcherValid(dispatcher))
	{
		dispatch.DispatchConditionRemove(dispatcher, cond);
	}
}

int* ConditionSystem::CondNodeGetArgPtr(CondNode* condNode, int argIdx)
{
	if (argIdx < condNode->condStruct->numArgs)
		return (int*)&condNode->args[argIdx];
	return 0;
}

#pragma region NewConditionCallbacks

 int __cdecl AoODisableRadialMenuInit(DispatcherCallbackArgs args)
{
	RadialMenuEntry radEntry;
	radEntry.SetDefaults();
	radEntry.maxArg = 1;
	radEntry.minArg = 0;
	radEntry.type = RadialMenuEntryType::Toggle;
	radEntry.actualArg = (int)conds.CondNodeGetArgPtr(args.subDispNode->condNode, 0);
	radEntry.callback = (void (__cdecl*)(objHndl, RadialMenuEntry*))temple_address(0x100F0200);
	MesLine mesLine;
	mesLine.key = 5105; //disable AoOs
	if (!mesFuncs.GetLine(*combatSys.combatMesfileIdx, &mesLine) )
	{
		sprintf((char*)temple_address(0x10EEE228), "Disable Attacks of Opportunity");
		mesLine.value = (char*) temple_address(0x10EEE228);
	};
	//mesFuncs.GetLine_Safe(*combatSys.combatMesfileIdx, &mesLine);
	radEntry.text = (char*)mesLine.value;
	radEntry.helpId = conds.hashmethods.StringHash("TAG_RADIAL_MENU_DISABLE_AOOS");
	int parentNode = radialMenus.GetStandardNode(RadialMenuStandardNode::Options);
	radialMenus.AddChildNode(args.objHndCaller, &radEntry, parentNode);
	return 0;
}

int __cdecl AoODisableQueryAoOPossible(DispatcherCallbackArgs args)
{
	DispIoD20Query * dispIo = dispatch.DispIoCheckIoType7((DispIoD20Query*)args.dispIO);
	if (dispIo->return_val && conds.CondNodeGetArg(args.subDispNode->condNode, 0))
	{
		dispIo->return_val = 0;
	}
	return 0;
}

int __cdecl GreaterTwoWeaponFighting(DispatcherCallbackArgs args)
{
	DispIoD20ActionTurnBased *dispIo = dispatch.DispIOCheckIoType12((DispIoD20ActionTurnBased*)args.dispIO);
	objHndl mainWeapon = inventory.ItemWornAt(args.objHndCaller, 3);
	objHndl offhand = inventory.ItemWornAt(args.objHndCaller, 4);
	
	if (mainWeapon != offhand)
	{
		if (mainWeapon)
		{
			if (offhand)
			{
				int weapFlags = objects.getInt32(mainWeapon, obj_f_weapon_flags);
				if (!(weapFlags & (4<<8)) && objects.getInt32(offhand, obj_f_type) != obj_t_armor)
					++dispIo->returnVal;
			}
		}
	}
	return 0;

}

int __cdecl GreaterTWFRanger(DispatcherCallbackArgs args)
{
	if (!critterSys.IsWearingLightOrNoArmor(args.objHndCaller))
	{
		return 0;
	}
	return GreaterTwoWeaponFighting(args);
};

int __cdecl TwoWeaponFightingBonus(DispatcherCallbackArgs args)
{
	DispIoAttackBonus *dispIo = dispatch.DispIoCheckIoType5((DispIoAttackBonus*)args.dispIO);
	char *featName; // eax@3 MAPDST

	feat_enums feat = (feat_enums)conds.CondNodeGetArg(args.subDispNode->condNode, 0);
	int attackCode = dispIo->attackPacket.dispKey;
	int dualWielding = 0;
	int attackNumber = 1;
	if (d20Sys.UsingSecondaryWeapon(args.objHndCaller, attackCode))
	{
		featName = feats.GetFeatName(feat);
		bonusSys.bonusAddToBonusListWithDescr(&dispIo->bonlist, 6, 0, 114, featName);
	}
		else if ( d20Sys.ExtractAttackNumber(args.objHndCaller, attackCode, &attackNumber, &dualWielding), dualWielding != 0)
	{
		featName = feats.GetFeatName(feat);
		bonusSys.bonusAddToBonusListWithDescr(&dispIo->bonlist, 2, 0, 114, featName);
	}
	return 0;
}

int TwoWeaponFightingBonusRanger(DispatcherCallbackArgs args)
{
	DispIoAttackBonus * dispIo = dispatch.DispIoCheckIoType5((DispIoAttackBonus*)args.dispIO);
	if ( !critterSys.IsWearingLightOrNoArmor(args.objHndCaller))
	{
		bonusSys.zeroBonusSetMeslineNum(&dispIo->bonlist, 166);
		return 0;
	}
	
	
	feat_enums feat = FEAT_TWO_WEAPON_FIGHTING;
	char * featName;
	int attackCode = dispIo->attackPacket.dispKey;
	int dualWielding = 0;
	int attackNumber = 1;
	if (d20Sys.UsingSecondaryWeapon(args.objHndCaller, attackCode))
	{
		featName = feats.GetFeatName(feat);
		bonusSys.bonusAddToBonusListWithDescr(&dispIo->bonlist, 6, 0, 114, featName);
	}
	else if (d20Sys.ExtractAttackNumber(args.objHndCaller, attackCode, &attackNumber, &dualWielding), dualWielding != 0)
	{
		featName = feats.GetFeatName(feat);
		bonusSys.bonusAddToBonusListWithDescr(&dispIo->bonlist, 2, 0, 114, featName);
	}
	return 0;

}



int __cdecl DivineMightRadial(DispatcherCallbackArgs args)
{
	if (d20Sys.d20Query(args.objHndCaller, DK_QUE_IsFallenPaladin))
		return 0;

	RadialMenuEntry radEntry;
	radEntry.SetDefaults();
	radEntry.type = RadialMenuEntryType::Action;
	radEntry.d20ActionType = D20A_DIVINE_MIGHT;
	radEntry.d20ActionData1 = 0;
	MesLine mesLine;
	mesLine.key = 5106; // Divine Might
	if (!mesFuncs.GetLine(*combatSys.combatMesfileIdx, &mesLine))
	{
		sprintf((char*)temple_address(0x10EEE228), "Divine Might");
		mesLine.value = (char*)temple_address(0x10EEE228);
	};
	radEntry.text = (char*)mesLine.value;
	radEntry.helpId = conds.hashmethods.StringHash("TAG_DIVINE_MIGHT");
	int parentNode = radialMenus.GetStandardNode(RadialMenuStandardNode::Feats);
	radialMenus.AddChildNode(args.objHndCaller, &radEntry, parentNode);
	return 0;
}


int __cdecl DivineMightDamageBonus(DispatcherCallbackArgs args)
{
	DispIoDamage * dispIo = dispatch.DispIoCheckIoType4((DispIoDamage*)args.dispIO);
	char * desc = feats.GetFeatName(FEAT_DIVINE_MIGHT);
	int damBonus = conds.CondNodeGetArg(args.subDispNode->condNode, 0);
	damage.AddDamageBonus(&dispIo->damage, damBonus, 0, 114, desc);
	return 0;
}


int GreaterWeaponSpecializationDamage(DispatcherCallbackArgs args)
{
	int weaponType; 
	char *featName; 

	feat_enums feat = (feat_enums)conds.CondNodeGetArg(args.subDispNode->condNode, 0);
	WeaponTypes wpnTypeFromCond = (WeaponTypes)conds.CondNodeGetArg(args.subDispNode->condNode, 1);
	DispIoDamage * dispIo = dispatch.DispIoCheckIoType4(args.dispIO);
	objHndl weapon = combatSys.GetWeapon(&dispIo->attackPacket);
	if (weapon)
		weaponType = objects.getInt32(weapon, obj_f_weapon_type);
	else
		weaponType = 1;
	if (weaponType == wpnTypeFromCond)
	{
		featName = feats.GetFeatName(feat);
		damage.AddDamageBonus(&dispIo->damage, 2, 0, 114, featName);
	}
	return 0;
}

int __cdecl RecklessOffenseRadialMenuInit(DispatcherCallbackArgs args)
{
	RadialMenuEntry radEntry;
	radEntry.SetDefaults();
	radEntry.maxArg = 1;
	radEntry.minArg = 0;
	radEntry.type = RadialMenuEntryType::Toggle;
	radEntry.actualArg = (int)conds.CondNodeGetArgPtr(args.subDispNode->condNode, 0);
	radEntry.callback = (void(__cdecl*)(objHndl, RadialMenuEntry*))temple_address(0x100F0200);
	MesLine mesLine;
	mesLine.key = 5107; //disable AoOs
	if (!mesFuncs.GetLine(*combatSys.combatMesfileIdx, &mesLine))
	{
		sprintf((char*)temple_address(0x10EEE228), "Reckless Offense");
		mesLine.value = (char*)temple_address(0x10EEE228);
	};
	radEntry.text = (char*)mesLine.value;
	radEntry.helpId = conds.hashmethods.StringHash("TAG_FEAT_RECKLESS_OFFENSE");
	int parentNode = radialMenus.GetStandardNode(RadialMenuStandardNode::Feats);
	radialMenus.AddChildNode(args.objHndCaller, &radEntry, parentNode);
	return 0;
}

int RecklessOffenseAcPenalty(DispatcherCallbackArgs args)
{
	if (conds.CondNodeGetArg(args.subDispNode->condNode, 0))
	{
		if (conds.CondNodeGetArg(args.subDispNode->condNode, 1))
		{
			DispIoAttackBonus* dispIo = dispatch.DispIoCheckIoType5(args.dispIO);
			BonusList* bonlist = &dispIo->bonlist;
			bonusSys.bonusAddToBonusList(bonlist, -4, 8, 337);
		}
	}
	return 0;
}

int RecklessOffenseToHitBonus(DispatcherCallbackArgs args)
{
	if (conds.CondNodeGetArg(args.subDispNode->condNode, 0))
	{
		DispIoAttackBonus* dispIo = dispatch.DispIoCheckIoType5(args.dispIO);
		if (!(dispIo->attackPacket.flags & D20CAF_RANGED))
			bonusSys.bonusAddToBonusList(&dispIo->bonlist, 2, 0, 337);
	}
	return 0;
}

int TacticalOptionAbusePrevention(DispatcherCallbackArgs args)
{ // signifies that an attack has been made using that tactical option (so user doesn't toggle it off and shrug off the penalties)
	DispIoD20Signal * dispIo = dispatch.DispIoCheckIoType6(args.dispIO);
	if (!(*(char*)(dispIo->data1 + 32) & 4))
		_CondNodeSetArg(args.subDispNode->condNode, 1, 1);
	return 0;
}

int __cdecl CombatExpertiseRadialMenu(DispatcherCallbackArgs args)
{
	int bab = dispatch.DispatchToHitBonusBase(args.objHndCaller, 0);
	if (bab > 0)
	{
		RadialMenuEntry radEntry;
		radEntry.SetDefaults();
		radEntry.maxArg = bab;
		if (bab > 5 && !feats.HasFeatCount(args.objHndCaller, FEAT_SUPERIOR_EXPERTISE))
			radEntry.maxArg = 5;
		radEntry.minArg = 0;
		radEntry.type = RadialMenuEntryType::Slider;
		radEntry.actualArg = (int)conds.CondNodeGetArgPtr(args.subDispNode->condNode, 0);
		radEntry.callback = (void(__cdecl*)(objHndl, RadialMenuEntry*))temple_address(0x100F0200);
		MesLine mesLine;
		mesLine.key = 5007; // Combat Expertise
		if (!mesFuncs.GetLine(*combatSys.combatMesfileIdx, &mesLine))
		{
			sprintf((char*)temple_address(0x10EEE228), "Combat Expertise");
			mesLine.value = (char*)temple_address(0x10EEE228);
		}
		radEntry.text = (char*)mesLine.value;
		radEntry.helpId = conds.hashmethods.StringHash("TAG_COMBAT_EXPERTISE");
		int parentNode = radialMenus.GetStandardNode(RadialMenuStandardNode::Feats);
		radialMenus.AddChildNode(args.objHndCaller, &radEntry, parentNode);
	}
	return 0;
}

int CombatExpertiseSet(DispatcherCallbackArgs args)
{
	int bab = dispatch.DispatchToHitBonusBase(args.objHndCaller, 0);
	if (bab > 5 && !feats.HasFeatCount(args.objHndCaller, FEAT_SUPERIOR_EXPERTISE))
		bab = 5;
	DispIoD20Signal * dispIo = dispatch.DispIoCheckIoType6(args.dispIO);
	int bonus = dispIo->data1;
	if (bonus > bab)
		bonus = bab;
	if (bonus < 0)
		bonus = 0;
	conds.CondNodeSetArg(args.subDispNode->condNode, 0, bonus);
	return 0;
}

int BarbarianRageStatBonus(DispatcherCallbackArgs args)
{
	DispIoBonusList * dispIo = dispatch.DispIoCheckIoType2(args.dispIO);
	if (feats.HasFeatCountByClass(args.objHndCaller, FEAT_GREATER_RAGE, (Stat)0, 0))
		bonusSys.bonusAddToBonusList(&dispIo->bonlist, 6, 0, 338); // Greater Rage
	else
		bonusSys.bonusAddToBonusList(&dispIo->bonlist, 4, 0, 195); // normal rage
	return 0;
}

int BarbarianRageSaveBonus(DispatcherCallbackArgs args)
{
	DispIoSavingThrow * dispIo = dispatch.DispIoCheckIoType3(args.dispIO);
	if (feats.HasFeatCountByClass(args.objHndCaller, FEAT_GREATER_RAGE, (Stat)0, 0))
		bonusSys.bonusAddToBonusList(&dispIo->bonlist, 3, 0, 338); // Greater Rage
	else
		bonusSys.bonusAddToBonusList(&dispIo->bonlist, 2, 0, 195); // normal rage
	return 0;
}

int BarbarianDamageResistance(DispatcherCallbackArgs args)
{
	DamagePacket *damagePacket; 
	int barbLvl; 
	int damRes = 0;

	damagePacket = &dispatch.DispIoCheckIoType4(args.dispIO)->damage;
	barbLvl = objects.StatLevelGet(args.objHndCaller, stat_level_barbarian);
	if (barbLvl >= 7)
	{
		damRes = 1 + (barbLvl - 7) / 3;
		damage.AddPhysicalDR(damagePacket, damRes, 1, 126u);
	}
	
	return 0;
}


void __cdecl BarbarianTirelessRageCheck(objHndl obj)
{
	if (objects.StatLevelGet(obj, stat_level_barbarian) < 17)
		conds.AddTo(obj, "Barbarian_Fatigued", {0,0});
};

class BarbarianTirelessRagePatch : public TempleFix
{
public:
	const char* name() override {
		return "Barbarian Tireless Rage patch";
	}

	void apply() override {
		redirectCall(0x100EADBF, BarbarianTirelessRageCheck);
	}

} barbarianTirelessRagePatch;

#pragma endregion