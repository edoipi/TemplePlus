
#include "stdafx.h"
#include "fixes.h"
#include "addresses.h"
#include "dependencies/python-2.2/Python.h"
#include "temple_functions.h""
#include "python_header.h"

static bool isTypeCritter(int nObjType){
	if (nObjType == obj_t_pc || nObjType == obj_t_npc){
		return 1;
	}
	return 0;
};

static bool isTypeContainer(int nObjType){
	if (nObjType == obj_t_container || nObjType == obj_t_bag ){
		return 1;
	}
	return 0;
};

static _nFieldIdx objGetInventoryListFieldIdx(TemplePyObjHandle* obj){
	int objType = templeFuncs.Obj_Get_Field_32bit(obj->objHandle, obj_f_type);
	if (isTypeCritter(objType)){
		return obj_f_critter_inventory_list_idx;
	}
	else if (isTypeContainer(objType)){
		return obj_f_container_inventory_list_idx;
	};
	return 0;
}

static PyObject * pyObjHandleType_Faction_Has(TemplePyObjHandle* obj, PyObject * pyTupleIn){
	int nFac;
	if (!PyArg_ParseTuple(pyTupleIn, "i", &nFac)) {
		return nullptr;
	};
	return PyInt_FromLong(templeFuncs.Obj_Faction_Has(obj->objHandle, nFac));
};

static PyObject * pyObjHandleType_Faction_Add(TemplePyObjHandle* obj, PyObject * pyTupleIn){
	int nFac;
	if (!PyArg_ParseTuple(pyTupleIn, "i", &nFac)) {
		return nullptr;
	};
	
	if (nFac == 0){
		return PyInt_FromLong(0);
	}
	if (! templeFuncs.Obj_Faction_Has(obj->objHandle, nFac) ) {
		templeFuncs.Obj_Faction_Add(obj->objHandle, nFac);
	}
	
	return PyInt_FromLong(1);
};

static PyObject * pyObjHandleType_Inventory(TemplePyObjHandle* obj, PyObject * pyTupleIn){
	int nArgs = PyTuple_Size(pyTupleIn);
	ObjHndl ObjHnd = obj->objHandle;
	int nModeSelect = 0;
	bool bIncludeBackpack = 1;
	bool bIncludeEquipped = 0;
	bool bRetunProtos = 0;
	int nInventoryFieldType = objGetInventoryListFieldIdx(obj);

	
	
	int nItems = templeFuncs.Obj_Get_IdxField_NumItems(ObjHnd, nInventoryFieldType);

	if (nArgs == 1){ // returns the entire non-worn inventory
		if (!PyArg_ParseTuple(pyTupleIn, "i", &nModeSelect)) {
			return nullptr;
		} 
		else if (nModeSelect == 1){
			bIncludeEquipped = 1;
		}
		else if (nModeSelect == 2){
			bIncludeBackpack = 0;
			bIncludeEquipped = 1;
		};
	};


	ObjHndl ItemObjHnds[8192] = {}; // seems large enough to cover any practical case :P

	int nMax = CRITTER_MAX_ITEMS;
	if (nInventoryFieldType == obj_f_container_inventory_list_idx){
		nMax = CONTAINER_MAX_ITEMS;
	};

	int j = 0;
	if (bIncludeBackpack){
		for (int i = 0; (j < nItems) & (i < nMax); i++){
			ItemObjHnds[j] = templeFuncs.Obj_Get_Item_At_Inventory_Location_n(ObjHnd, i);
			if (ItemObjHnds[j]){ j++; };
		}
	};

	if (bIncludeEquipped){
		for (int i = CRITTER_EQUIPPED_ITEM_OFFSET; (j < nItems) & (i < CRITTER_EQUIPPED_ITEM_OFFSET + CRITTER_EQUIPPED_ITEM_SLOTS); i++){
			ItemObjHnds[j] = templeFuncs.Obj_Get_Item_At_Inventory_Location_n(ObjHnd, i);
			if (ItemObjHnds[j]){ j++; };
		}
	};

	auto ItemPyTuple = PyTuple_New(j);
	if (!bRetunProtos){
		for (int i = 0; i < j; i++){
			PyTuple_SetItem(ItemPyTuple, i, templeFuncs.PyObj_From_ObjHnd(ItemObjHnds[i]));
		}
	}
	else {
		//TODO
	}
	
	
	return ItemPyTuple;
};

static PyObject * pyObjHandleType_Get_Field_64bit(TemplePyObjHandle* obj, PyObject * pyTupleIn){
	_nFieldIdx nFieldIdx = 0;
	uint64_t n64 = 0;
	if (!PyArg_ParseTuple(pyTupleIn, "i", &nFieldIdx)) {
		return nullptr;
	};
	return PyLong_FromLongLong( templeFuncs.Obj_Get_Field_64bit(obj->objHandle, nFieldIdx) );
};

static PyObject * pyObjHandleType_Set_Field_64bit(TemplePyObjHandle* obj, PyObject * pyTupleIn){
	_nFieldIdx nFieldIdx = 0;
	uint64_t n64 = 0;
	if (!PyArg_ParseTuple(pyTupleIn, "iL", &nFieldIdx, &n64)) {
		return PyInt_FromLong(0);
	};
	templeFuncs.Obj_Set_Field_64bit(obj->objHandle, nFieldIdx, n64);
	return PyInt_FromLong(1);
};

static PyObject * pyObjHandleType_Get_Field_ObjHandle(TemplePyObjHandle* obj, PyObject * pyTupleIn){
	_nFieldIdx nFieldIdx = 0;
	ObjHndl ObjHnd = 0;
	if (!PyArg_ParseTuple(pyTupleIn, "i", &nFieldIdx)) {
		return nullptr;
	};
	return PyLong_FromLongLong(templeFuncs.Obj_Get_Field_ObjHnd__fastout(obj->objHandle, nFieldIdx));
};

static PyObject * pyObjHandleType_Set_Field_ObjHandle(TemplePyObjHandle* obj, PyObject * pyTupleIn){
	_nFieldIdx nFieldIdx = 0;
	ObjHndl ObjHnd = 0;
	if (!PyArg_ParseTuple(pyTupleIn, "iL", &nFieldIdx, &ObjHnd)) {
		return PyInt_FromLong(0);
	};
	templeFuncs.Obj_Set_Field_ObjHnd(obj->objHandle, nFieldIdx, ObjHnd);
	return PyInt_FromLong(1);
};

static PyObject * pyObjHandleType_Get_IdxField_32bit(TemplePyObjHandle* obj, PyObject * pyTupleIn){
	_nFieldIdx nFieldIdx = 0;
	_nFieldSubIdx nFieldSubIdx = 0;
	uint64_t n32 = 0;
	if (!PyArg_ParseTuple(pyTupleIn, "ii", &nFieldIdx, &nFieldSubIdx)) {
		return nullptr;
	};
	return PyInt_FromLong(templeFuncs.Obj_Get_IdxField_32bit(obj->objHandle, nFieldIdx, nFieldSubIdx));
};

static PyObject * pyObjHandleType_Set_IdxField_32bit(TemplePyObjHandle* obj, PyObject * pyTupleIn){
	_nFieldIdx nFieldIdx = 0;
	_nFieldSubIdx nFieldSubIdx = 0;
	uint64_t n32 = 0;
	if (!PyArg_ParseTuple(pyTupleIn, "iii", &nFieldIdx, &nFieldSubIdx, &n32)) {
		return PyInt_FromLong(0);
	};
	templeFuncs.Obj_Set_IdxField_32bit(obj->objHandle, nFieldIdx, nFieldSubIdx, n32);
	return PyInt_FromLong(1);
};


static PyObject * pyObjHandleType_Remove_From_All_Groups(TemplePyObjHandle* obj, PyObject * pyTupleIn){
	templeFuncs.Obj_Remove_From_All_Group_Arrays(obj->objHandle);
	return PyInt_FromLong(1);
};



static PyObject * pyObjHandleType_PC_Rejoin(TemplePyObjHandle* obj, PyObject * pyTupleIn){

	templeFuncs.Obj_Add_to_PC_Group(obj->objHandle);
	return PyInt_FromLong(1);
};

static PyMethodDef pyObjHandleMethods_New[] = {
	"faction_has", (PyCFunction)pyObjHandleType_Faction_Has, METH_VARARGS, "Check if NPC has faction. Doesn't work on PCs!",
	"faction_add", (PyCFunction)pyObjHandleType_Faction_Add, METH_VARARGS, "Add a faction to an NPC. Doesn't work on PCs!",
	"inventory", (PyCFunction)pyObjHandleType_Inventory, METH_VARARGS, "Fetches a tuple of the object's inventory (items are Python Objects). Optional argument int nModeSelect : 0 - backpack only (excludes equipped items); 1 - backpack + equipped; 2 - equipped only",
	"obj_get_field_64bit", (PyCFunction)pyObjHandleType_Get_Field_64bit, METH_VARARGS, "Gets 64 bit field",
	"obj_set_field_64bit", (PyCFunction)pyObjHandleType_Set_Field_64bit, METH_VARARGS, "Sets 64 bit field",
	"obj_get_field_objhndl", (PyCFunction)pyObjHandleType_Get_Field_ObjHandle, METH_VARARGS, "Gets ObjHndl field",
	"obj_set_field_objhndl", (PyCFunction)pyObjHandleType_Set_Field_ObjHandle, METH_VARARGS, "Sets ObjHndl field",
	"obj_get_idxfield_32bit", (PyCFunction)pyObjHandleType_Get_IdxField_32bit, METH_VARARGS, "Gets 32 bit index field",
	"obj_set_idxfield_32bit", (PyCFunction)pyObjHandleType_Set_IdxField_32bit, METH_VARARGS, "Sets 32 bit index field",
	"obj_remove_from_all_groups", (PyCFunction)pyObjHandleType_Remove_From_All_Groups, METH_VARARGS, "Removes the object from all the groups (GroupList, PCs, NPCs, AI controlled followers, Currently Selected",
	"pc_rejoin", (PyCFunction)pyObjHandleType_PC_Rejoin, METH_VARARGS, "Removes the object from all the groups (GroupList, PCs, NPCs, AI controlled followers, Currently Selected",

	0, 0, 0, 0
};



PyObject* __cdecl  pyObjHandleType_getAttrNew(TemplePyObjHandle *obj, char *name) {
	LOG(info) << "Tried getting property: " << name;
	if (!_strcmpi(name, "co8rocks")) {
		return PyString_FromString("IT SURE DOES!");
	}

	if (!_strcmpi(name, "ObjHandle")) {
		return  PyLong_FromLongLong(obj->objHandle); 
	}

	if (!_strcmpi(name, "factions")) {
		ObjHndl ObjHnd = obj->objHandle;
		int a[50] = {};
		int n = 0;

		for (int i = 0; i < 50; i ++){
			int fac = templeFuncs.Obj_Get_IdxField_32bit(ObjHnd, obj_f_npc_faction, i);
			if (fac == 0) break;
			a[i] = fac;
			n++;
		};

		auto outTup = PyTuple_New(n);
		for (int i = 0; i < n ; i++){
			PyTuple_SetItem(outTup, i, PyInt_FromLong(a[i]));
		};

		
		return  outTup; 
	}

	if (!_strcmpi(name, "faction_has")) {
		return Py_FindMethod(pyObjHandleMethods_New, obj, "faction_has");
	}
	else if (!_strcmpi(name, "faction_add"))
	{
		return Py_FindMethod(pyObjHandleMethods_New, obj, "faction_add");
	} 
	else if (!_strcmpi(name, "substitute_inventory"))
	{
		ObjHndl ObjSubsInv = templeFuncs.Obj_Get_Substitute_Inventory(obj->objHandle);
		return templeFuncs.PyObj_From_ObjHnd(ObjSubsInv);
	};
	

	if (!_strcmpi(name, "inventory")) {
		return Py_FindMethod(pyObjHandleMethods_New, obj, "inventory");
	}
	else if (!_strcmpi(name, "inventory_item")){
		return Py_FindMethod(pyObjHandleMethods_New, obj, "inventory_item");
	}
	else if (!_strcmpi(name, "inventory_room_left")){
		return Py_FindMethod(pyObjHandleMethods_New, obj, "inventory_room_left");
	};


	if (!_strcmpi(name, "obj_get_field_64bit")) {
		return Py_FindMethod(pyObjHandleMethods_New, obj, "obj_get_field_64bit");
	}
	else if (!_strcmpi(name, "obj_get_field_ObjHndl")){
		return Py_FindMethod(pyObjHandleMethods_New, obj, "obj_get_field_objhndl");
	}
	else if (!_strcmpi(name, "obj_get_idxfield_numitems")){
		return Py_FindMethod(pyObjHandleMethods_New, obj, "obj_get_idxfield_numitems");
	}
	else if (!_strcmpi(name, "obj_get_idxfield_32bit")){
		return Py_FindMethod(pyObjHandleMethods_New, obj, "obj_get_idxfield_32bit");
	}
	else if (!_strcmpi(name, "obj_get_idxfield_64bit")){
		return Py_FindMethod(pyObjHandleMethods_New, obj, "obj_get_idxfield_64bit");
	}
	else if (!_strcmpi(name, "obj_get_idxfield_ObjHndl")){
		return Py_FindMethod(pyObjHandleMethods_New, obj, "obj_get_idxfield_objhandle");
	}
	else if (!_strcmpi(name, "obj_get_idxfield_256bit")){
		return Py_FindMethod(pyObjHandleMethods_New, obj, "obj_get_idxfield_256bit");
	}
	else if (!_strcmpi(name, "obj_set_field_64bit")) {
		return Py_FindMethod(pyObjHandleMethods_New, obj, "obj_set_field_64bit");
	}
	else if (!_strcmpi(name, "obj_set_field_ObjHndl")) {
		return Py_FindMethod(pyObjHandleMethods_New, obj, "obj_set_field_objhndl");
	}
	else if (!_strcmpi(name, "obj_set_idxfield_32bit")){
		return Py_FindMethod(pyObjHandleMethods_New, obj, "obj_set_idxfield_32bit");
	};

	if (!_strcmpi(name, "pc_stay_behind")){
		return Py_FindMethod(pyObjHandleMethods_New, obj, "obj_remove_from_all_groups");
	} 
	else if (!_strcmpi(name, "pc_rejoin")){
		return Py_FindMethod(pyObjHandleMethods_New, obj, "pc_rejoin");
	};


	return pyObjHandleTypeGetAttr(obj, name);
}




int __cdecl  pyObjHandleType_setAttrNew(TemplePyObjHandle *obj, char *name, TemplePyObjHandle *obj2) {
	LOG(info) << "Tried setting property: " << name;
	if (!strcmp(name, "co8rocks")) {
		return 0;
	}

	if (!strcmp(name, "substitute_inventory")) {

		if (obj2 != nullptr)  {
			if (obj->ob_type == obj2->ob_type){
				templeFuncs.Obj_Set_Field_ObjHnd(obj->objHandle, obj_f_npc_substitute_inventory, obj2->objHandle);
			}
		}
		return 0;
	}


	return pyObjHandleTypeSetAttr(obj, name, obj2);
}

class PythonExtensions : public TempleFix {
public:
	const char* name() override {
		return "Python Script Extensions";
	}
	void apply() override;
} pythonExtension;

void PythonExtensions::apply() {

	// Hook the getattr function of obj handles
	pyObjHandleTypeGetAttr = pyObjHandleType->tp_getattr;
	pyObjHandleType->tp_getattr = (getattrfunc) pyObjHandleType_getAttrNew;

	pyObjHandleTypeSetAttr = pyObjHandleType->tp_setattr;
	pyObjHandleType->tp_setattr = (setattrfunc) pyObjHandleType_setAttrNew;


	//a[0] = pyObjHandleMethods->ml_meth;
	
}