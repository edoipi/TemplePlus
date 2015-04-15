#pragma once

//#include "temple_functions.h"
#include "dispatcher.h"
#include "common.h"
#include "d20.h"
#include "feat.h"
#include "description.h"
#include "faction.h"
#include "feat.h"
#include "inventory.h"

struct FloatLineSystem;
//forward decl
struct LocationSys;
struct Pathfinding;
struct FieldDataMax { uint32_t data[8]; }; // for wrapping "objSetField" calls that get input by value; this is the largest data size that I know of


struct Objects : AddressTable {

	// Retrieves the object flags for the given object handle
	uint32_t GetFlags(objHndl obj) {
		return _GetInternalFieldInt32(obj, obj_f_flags);
	}
	uint32_t getInt32(objHndl obj, obj_f fieldIdx);
	void setInt32(objHndl obj, obj_f fieldIdx, uint32_t dataIn);
	void setArrayFieldByValue(objHndl obj, obj_f fieldIdx, uint32_t subIdx, FieldDataMax data);
	int32_t getArrayFieldInt32(objHndl obj, obj_f fieldIdx, uint32_t subIdx);
	uint32_t abilityScoreLevelGet(objHndl, Stat, DispIO *);

#pragma region Common
	ObjectType GetType(objHndl obj);
	uint32_t IsDeadNullDestroyed(objHndl obj);
	uint32_t IsUnconscious(objHndl obj);
	int32_t GetHPCur(objHndl obj);
	uint32_t GetRace(objHndl obj);
	bool IsCritter(objHndl obj);
	bool IsPlayerControlled(objHndl obj);
	uint32_t ObjGetProtoNum(objHndl obj);

	uint32_t StatLevelGet(objHndl obj, Stat stat);
#pragma endregion

#pragma region Category
	MonsterCategory GetCategory(objHndl objHnd);
	bool IsCategoryType(objHndl objHnd, MonsterCategory categoryType);
	bool IsCategorySubtype(objHndl objHnd, MonsterCategory categoryType);
	bool IsUndead(objHndl objHnd);
	bool IsOoze(objHndl objHnd);
	bool IsSubtypeFire(objHndl objHnd);
#pragma endregion

#pragma region Dispatcher Stuff

	Dispatcher* GetDispatcher(objHndl obj) {
		return (Dispatcher *)(_GetInternalFieldInt32(obj, obj_f_dispatcher));
	}

	void SetDispatcher(objHndl obj, uint32_t data32) {
		_SetInternalFieldInt32(obj, obj_f_dispatcher, data32);
		return;
	}

#pragma endregion

#pragma region Subsystems
	DispatcherSystem dispatch;

	D20System d20;

	FeatSystem feats;

	DescriptionSystem description;

	FactionSystem factions;

	InventorySystem inventory;

	LocationSys * loc;

	Pathfinding * pathfinding;

	FloatLineSystem  * floats;
#pragma endregion

#pragma region Memory Internals
	uint32_t DoesTypeSupportField(uint32_t objType, _fieldIdx objField);

	void PropFetcher(GameObjectBody* objBody, obj_f fieldIdx, void * dataOut);

	void InsertDataIntoInternalStack(GameObjectBody * objBody, obj_f fieldIdx, void * dataIn);

#pragma endregion 



	Objects();
#pragma region Privates
private:
	void setArrayFieldLowLevel(GameObjectBody * objBody, void * sourceData, obj_f fieldIdx, uint32_t subIdx);
	void fieldNonexistantDebug(objHndl obj, GameObjectBody* objBody, obj_f fieldIdx, uint32_t objType, char* accessType);
	void getArrayFieldInternal(GameObjectBody * objBody, void * outAddr, obj_f fieldIdx, uint32_t subIdx); // _nFieldIdx@<eax>, _nFieldSubIdx@<ecx>

	int(__cdecl *_GetInternalFieldInt32)(objHndl ObjHnd, int nFieldIdx);
	int64_t(__cdecl *_GetInternalFieldInt64)(objHndl ObjHnd, int nFieldIdx);
	int32_t(__cdecl *_StatLevelGet)(objHndl ObjHnd, Stat);
	void(__cdecl *_SetInternalFieldInt32)(objHndl objHnd, obj_f fieldIdx, uint32_t data32);
	void(__cdecl * _setArrayFieldLowLevel)(obj_f fieldIdx, uint32_t subIdx); // GameObjectBody *@<ecx>, sourceData *@<eax>
	bool(__cdecl * _IsPlayerControlled)(objHndl objHnd);
	uint32_t(__cdecl *_ObjGetProtoNum)(objHndl);
	uint32_t(__cdecl *_IsObjDeadNullDestroyed)(objHndl);
	GameObjectBody * (__cdecl *_GetMemoryAddress)(objHndl ObjHnd);
	bool(__cdecl *_DoesObjectFieldExist)();
	void(__cdecl * _ObjectPropFetcher)();
	void (__cdecl *_getArrayFieldInternal)(GameObjectBody * objBody, void * out); // _nFieldIdx@<eax>, _nFieldSubIdx@<ecx>
	char ** _DLLFieldNames;
	void(__cdecl * _InsetDataIntoInternalStack)();//(int nFieldIdx, void *, ToEEObjBody *@<eax>);

	
#pragma endregion
} ;

extern Objects objects;

// const auto TestSizeOfDispatcher = sizeof(Dispatcher); // 0x138 as it should be

uint32_t _obj_get_int(objHndl obj, obj_f fieldIdx);
void _obj_set_int(objHndl obj, obj_f fieldIdx, uint32_t dataIn);
uint32_t _abilityScoreLevelGet(objHndl obj, Stat abScore, DispIO * dispIO);
void _setArrayFieldByValue(objHndl obj, obj_f fieldIdx, uint32_t subIdx, FieldDataMax data);
int32_t _getArrayFieldInt32(objHndl obj, obj_f fieldIdx, uint32_t subIdx);