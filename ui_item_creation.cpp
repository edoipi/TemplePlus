
#include "stdafx.h"
#include "ui.h"
#include "fixes.h"
#include "tig_mes.h"
#include "tig_font.h"
#include "temple_functions.h"
#include "tig_tokenizer.h"
#include "ui_item_creation.h"

struct UiSystemSpecs {
	UiSystemSpec systems[43];
};
static GlobalStruct<UiSystemSpecs, 0x102F6C10> templeUiSystems;
GlobalPrimitive<ItemCreationType, 0x10BEDF50> itemCreationType;
GlobalPrimitive<objHndl, 0x10BECEE0> globObjHndCrafter;

GlobalPrimitive<int32_t, 0x10BEE3A4> craftInsufficientXP;
GlobalPrimitive<int32_t, 0x10BEE3A8> craftInsufficientFunds;
GlobalPrimitive<int32_t, 0x10BEE3AC> craftSkillReqNotMet;
GlobalPrimitive<int32_t, 0x10BEE3B0> dword_10BEE3B0;

GlobalPrimitive<char *, 0x10BED6D4> itemCreationUIStringSkillRequired;
GlobalPrimitive<char *, 0x10BEDB50> itemCreationUIStringItemCost;
GlobalPrimitive<char *, 0x10BED8A4> itemCreationUIStringXPCost;
GlobalPrimitive<char *, 0x10BED8A8> itemCreationUIStringValue;
GlobalPrimitive<tig_text_style, 0x10BEE338> itemCreationTextStyle; // so far used by "Item Cost: %d" and "Experience Cost: %d"
GlobalPrimitive<tig_text_style, 0x10BED938> itemCreationTextStyle2; // so far used by "Value: %d"



class UiSystem {
public:

	static UiSystemSpec *getUiSystem(const char *name) {
		// Search for the ui system to replace
		for (auto &system : templeUiSystems->systems) {
			if (!strcmp(name, system.name)) {
				return &system;
			}
		}

		logger->error("Couldn't find UI system {}! Replacement failed.", name);
		return nullptr;
	}
};

struct ButtonStateTextures {
	int normal;
	int hover;
	int pressed;
	ButtonStateTextures() : normal(-1), hover(-1), pressed(-1) {}

	void loadAccept() {
		uiFuncs.GetAsset(UiAssetType::Generic, UiGenericAsset::AcceptNormal, &normal);
		uiFuncs.GetAsset(UiAssetType::Generic, UiGenericAsset::AcceptHover, &hover);
		uiFuncs.GetAsset(UiAssetType::Generic, UiGenericAsset::AcceptPressed, &pressed);
	}

	void loadDecline() {
		uiFuncs.GetAsset(UiAssetType::Generic, UiGenericAsset::DeclineNormal, &normal);
		uiFuncs.GetAsset(UiAssetType::Generic, UiGenericAsset::DeclineHover, &hover);
		uiFuncs.GetAsset(UiAssetType::Generic, UiGenericAsset::DeclinePressed, &pressed);
	}
};

static MesHandle mesItemCreationText;
static MesHandle mesItemCreationRules;
static MesHandle mesItemCreationNamesText;
static ImgFile *background = nullptr;
static ButtonStateTextures acceptBtnTextures;
static ButtonStateTextures declineBtnTextures;
static int disabledBtnTexture;



int32_t CreateItemResourceCheck(objHndl ObjHnd, objHndl ObjHndItem){
	bool canCraft = 1;
	bool xpCheck = 0;
	int32_t * globInsuffXP = craftInsufficientXP.ptr();
	int32_t * globInsuffFunds = craftInsufficientFunds.ptr();
	int32_t *globSkillReqNotMet = craftSkillReqNotMet.ptr();
	int32_t *globB0 = dword_10BEE3B0.ptr();
	uint32_t crafterLevel = templeFuncs.ObjStatGet(ObjHnd, stat_level);
	uint32_t minXPForCurrentLevel = templeFuncs.XPReqForLevel(crafterLevel); 
	uint32_t crafterXP = templeFuncs.Obj_Get_Field_32bit(ObjHnd, obj_f_critter_experience);
	uint32_t surplusXP = crafterXP - minXPForCurrentLevel;
	uint32_t craftingCostCP = 0;
	uint32_t partyMoney = templeFuncs.PartyMoney();

	*globInsuffXP = 0;
	*globInsuffFunds = 0;
	*globSkillReqNotMet = 0;
	*globB0 = 0;

	
	// Check GP Section
	if (itemCreationType == ItemCreationType(8) ){ 
		craftingCostCP = templeFuncs.ItemWorthFromEnhancements( 41 );
	}
	else
	{
		// current method for crafting stuff:
		craftingCostCP =  templeFuncs.Obj_Get_Field_32bit(ObjHndItem, obj_f_item_worth) / 2;

		// TODO: create new function
		// // craftingCostCP = CraftedItemWorthDueToAppliedLevel()
	};

	if ( ( (uint32_t)partyMoney ) < craftingCostCP){
		*globInsuffFunds = 1;
		canCraft = 0;
	};


	// Check XP section (and maybe spell prerequisites too? explore sub_10152280)
	if ( itemCreationType != ItemCreationType(8) ){
		if ( templeFuncs.sub_10152280(ObjHnd, ObjHndItem) == 0){ //TODO explore function
			*globB0 = 1;
			canCraft = 0;
		};

		// TODO make XP cost calculation take applied caster level into account
		uint32_t itemXPCost = templeFuncs.Obj_Get_Field_32bit(ObjHndItem, obj_f_item_worth) / 2500; 
		xpCheck = surplusXP > itemXPCost;
	} else 
	{
		uint32_t magicArmsAndArmorXPCost = templeFuncs.CraftMagicArmsAndArmorSthg(41);
		xpCheck = surplusXP > magicArmsAndArmorXPCost;
	}
		
	if (xpCheck){
		return canCraft;
	} else
	{
		*globInsuffXP = 1;
		return 0;
	};

};

void CraftScrollWandPotionSetItemSpellData(objHndl objHndItem, objHndl objHndCrafter){

	// the new and improved Wands/Scroll Property Setting Function

	auto pytup = PyTuple_New(2);
	PyTuple_SetItem(pytup, 0, templeFuncs.PyObjFromObjHnd(objHndItem));
	PyTuple_SetItem(pytup, 1, templeFuncs.PyObjFromObjHnd(objHndCrafter));

	if (itemCreationType == ItemCreationType(3)){
		// do wand specific stuff
		
		char * wandNewName = PyString_AsString(templeFuncs.PyScript_Execute("crafting", "craft_wand_new_name", pytup));

		templeFuncs.Obj_Set_Field_32bit(objHndItem, obj_f_description, templeFuncs.CustomNameNew(wandNewName)); // TODO: create function that appends effect of caster level boost
		//templeFuncs.GetGlo
	};
	if (itemCreationType == ItemCreationType(2)){
		// do scroll specific stuff
		// templeFuncs.Obj_Set_Field_32bit(objHndItem, obj_f_description, templeFuncs.CustomNameNew("Scroll of LOL"));
	};

	if (itemCreationType == ItemCreationType(1)){
		// do potion specific stuff
		// templeFuncs.Obj_Set_Field_32bit(objHndItem, obj_f_description, templeFuncs.CustomNameNew("Potion of Commotion"));
		// TODO: change it so it's 0xBAAD F00D just like spawned / mobbed potions
	};

	int numItemSpells = templeFuncs.Obj_Get_IdxField_NumItems(objHndItem, obj_f_item_spell_idx);

	// loop thru the item's spells (can be more than one in principle, like Keoghthem's Ointment)

	// Current code - change this at will...
	for (int n = 0; n < numItemSpells; n++){
		SpellStoredData spellData;
		templeFuncs.Obj_Get_IdxField_256bit(objHndItem, obj_f_item_spell_idx, n, &spellData);

		// get data from caster - make this optional!

		uint32_t classCodes[SPELLENUMMAX] = { 0, };
		uint32_t spellLevels[SPELLENUMMAX] = { 0, };
		uint32_t spellFoundNum = 0;
		int casterKnowsSpell = templeFuncs.ObjSpellKnownQueryGetData(objHndCrafter, spellData.spellEnum, classCodes, spellLevels, &spellFoundNum);
		if (casterKnowsSpell){
			uint32_t spellClassFinal = classCodes[0];
			uint32_t spellLevelFinal = 0;
			uint32_t isClassSpell = classCodes[0] & (0x80);

			if (isClassSpell){
				spellLevelFinal = templeFuncs.ObjGetMaxSpellSlotLevel(objHndCrafter, classCodes[0] & (0x7F), 0);
			};
			if (spellFoundNum > 1){
				for (uint32_t i = 1; i < spellFoundNum; i++){
					if (spellLevels[i] > spellLevelFinal){
						spellData.classCode = classCodes[i];
						spellLevelFinal = spellLevels[i];
					};
				}
				spellData.spellLevel = spellLevelFinal;

			};
			spellData.spellLevel = spellLevelFinal;
			templeFuncs.Obj_Set_IdxField_byPtr(objHndItem, obj_f_item_spell_idx, n, &spellData);

		};

	};
};


void CreateItemDebitXPGP(objHndl objHndCrafter, objHndl objHndItem){
	uint32_t crafterXP = templeFuncs.Obj_Get_Field_32bit(objHndCrafter, obj_f_critter_experience);
	uint32_t craftingCostCP = 0;
	uint32_t craftingCostXP = 0;

	if (itemCreationType == ItemCreationType(8)){ // magic arms and armor
		craftingCostCP = templeFuncs.ItemWorthFromEnhancements(41);
		craftingCostXP = templeFuncs.CraftMagicArmsAndArmorSthg(41);
	}
	else
	{
		// TODO make crafting costs take applied caster level into account
		// currently this is what ToEE does	
		craftingCostCP = templeFuncs.Obj_Get_Field_32bit(objHndItem, obj_f_item_worth) / 2;
		craftingCostXP = templeFuncs.Obj_Get_Field_32bit(objHndItem, obj_f_item_worth) / 2500;

	};

	templeFuncs.DebitPartyMoney(0, 0, 0, craftingCostCP);
	templeFuncs.Obj_Set_Field_32bit(objHndCrafter, obj_f_critter_experience, crafterXP - craftingCostXP);
};

void __cdecl UiItemCreationCraftingCostTexts(objHndl objHndItem){
	// prolog
	int32_t widgetId;
	int32_t * globInsuffXP;
	int32_t * globInsuffFunds;
	int32_t *globSkillReqNotMet;
	int32_t *globB0;
	uint32_t craftingCostCP;
	uint32_t craftingCostXP;
	char text[128];
	RECT rect;
	char * prereqString;
	__asm{
		mov widgetId, ebx; // widgetId is passed in ebx
	};


	uint32_t slowLevelNew = -1; // h4x!
	if (itemCreationType == ItemCreationType(3)){
		// do wand specific stuff
		slowLevelNew = 5;
		//templeFuncs.GetGlo
	};
	

	rect.left = 212;
	rect.top = 157;
	rect.right= 159;
	rect.bottom = 10;

	globInsuffXP = craftInsufficientXP.ptr();
	globInsuffFunds = craftInsufficientFunds.ptr();
	globSkillReqNotMet = craftSkillReqNotMet.ptr();
	globB0 = dword_10BEE3B0.ptr();

	//old method
	/* 
	craftingCostCP = templeFuncs.Obj_Get_Field_32bit(objHndItem, obj_f_item_worth) / 2;
	craftingCostXP = templeFuncs.Obj_Get_Field_32bit(objHndItem, obj_f_item_worth) / 2500;
	*/

	craftingCostCP = ItemWorthAdjustedForCasterLevel(objHndItem, slowLevelNew) / 2;
	craftingCostXP = ItemWorthAdjustedForCasterLevel(objHndItem, slowLevelNew) / 2500;
	

	// "Item Cost: %d"
	if (*globInsuffXP || *globInsuffFunds || *globSkillReqNotMet || *globB0){
		
		//_snprintf(text, 128, "%s @%d%d", itemCreationUIStringItemCost.ptr(), globInsuffFunds+1, craftingCostCP / 100);
		templeFuncs.temple_snprintf(text, 128, "%s @%d%d", *(itemCreationUIStringItemCost.ptr() ), *(globInsuffFunds) + 1, craftingCostCP / 100);
	}
	else {
		//_snprintf(text, 128, "%s @3%d", itemCreationUIStringItemCost.ptr(), craftingCostCP / 100);
		templeFuncs.temple_snprintf(text, 128, "%s @3%d", *(itemCreationUIStringItemCost.ptr()), craftingCostCP / 100);
	};


	templeFuncs.FontDrawSthg_sub_101F87C0(widgetId, text, &rect, itemCreationTextStyle.ptr());
	rect.top += 11;



	// "Experience Cost: %d"  (or "Skill Req: " for alchemy - WIP)

	if (itemCreationType == ItemCreationType(0)){ // alchemy
		// placeholder - they do similar bullshit in the code :P but I guess it can be modified easily enough!
		if (*globInsuffXP || *globInsuffFunds || *globSkillReqNotMet || *globB0){
			_snprintf(text, 128, "%s @%d%d", * (itemCreationUIStringSkillRequired.ptr() ), *(globSkillReqNotMet) + 1, craftingCostXP);
		}
		else {
			_snprintf(text, 128, "%s @3%d", *(itemCreationUIStringSkillRequired.ptr()), craftingCostXP);
		};
	}
	else
	{
		if (*globInsuffXP || *globInsuffFunds || *globSkillReqNotMet || *globB0){
			_snprintf(text, 128, "%s @%d%d", *(itemCreationUIStringXPCost.ptr()), *(globInsuffXP) + 1, craftingCostXP);
		}
		else {
			_snprintf(text, 128, "%s @3%d", *(itemCreationUIStringXPCost.ptr()), craftingCostXP);
		};
	};

	templeFuncs.FontDrawSthg_sub_101F87C0(widgetId, text, &rect, itemCreationTextStyle.ptr());
	rect.top += 11;

	// "Value: %d"
	//_snprintf(text, 128, "%s @1%d", * (itemCreationUIStringValue.ptr() ), templeFuncs.Obj_Get_Field_32bit(objHndItem, obj_f_item_worth) / 100);
	_snprintf(text, 128, "%s @1%d", *(itemCreationUIStringValue.ptr()), ItemWorthAdjustedForCasterLevel(objHndItem, slowLevelNew) / 100);

	templeFuncs.FontDrawSthg_sub_101F87C0(widgetId, text, &rect, itemCreationTextStyle2.ptr());




	// Prereq: %s
	;
	rect.left = 210;
	rect.top = 200;
	rect.right = 150;
	rect.bottom = 105;
	prereqString = templeFuncs.ItemCreationPrereqSthg_sub_101525B0(globObjHndCrafter, objHndItem);
	if (prereqString){
		templeFuncs.FontDrawSthg_sub_101F87C0(widgetId, prereqString , &rect, itemCreationTextStyle.ptr());
	}
	

	
};


uint32_t ItemWorthAdjustedForCasterLevel(objHndl objHndItem, uint32_t slotLevelNew){
	uint32_t numItemSpells = templeFuncs.Obj_Get_IdxField_NumItems(objHndItem, obj_f_item_spell_idx);
	uint32_t itemWorthBase = templeFuncs.Obj_Get_Field_32bit(objHndItem, obj_f_item_worth);
	if (slotLevelNew == -1){
		return itemWorthBase;
	}

	uint32_t itemSlotLevelBase = 0;

	// loop thru the item's spells (can be more than one in principle, like Keoghthem's Ointment)

	for (uint32_t n = 0; n < numItemSpells; n++){
		SpellStoredData spellData;
		templeFuncs.Obj_Get_IdxField_256bit(objHndItem, obj_f_item_spell_idx, n, &spellData);
		if (spellData.spellLevel > itemSlotLevelBase){
			itemSlotLevelBase = spellData.spellLevel;
		}
	};

	if (itemSlotLevelBase == 0 && slotLevelNew > itemSlotLevelBase){
		return itemWorthBase * slotLevelNew * 2;
	}
	else if (slotLevelNew > itemSlotLevelBase)
	{
		return itemWorthBase * slotLevelNew / itemSlotLevelBase;
	};
	return itemWorthBase;

	
}

static vector<uint64_t> craftingProtoHandles[8];

const char *getProtoName(uint64_t protoHandle) {
	/*
	 // gets item creation proto id
  if ( sub_1009C950((objHndl)protoHandle) )
    v1 = sub_100392E0(protoHandle);
  else
    v1 = sub_10039320((objHndl)protoHandle);

  line.key = v1;
  if ( tig_mes_get_line(ui_itemcreation_names, &line) )
    result = line.value;
  else
    result = ObjGetDisplayName((objHndl)protoHandle, (objHndl)protoHandle);
  return result;
  */

	return templeFuncs.ObjGetDisplayName(protoHandle, protoHandle);
}

static void loadProtoIds(MesHandle mesHandle) {

	for (uint32_t i = 0; i < 8; ++i) {
		auto protoLine = mesFuncs.GetLineById(mesHandle, i);
		if (!protoLine) {
			continue;
		}

		auto &protoHandles = craftingProtoHandles[i];

		StringTokenizer tokenizer(protoLine);
		while (tokenizer.next()) {
			auto handle = templeFuncs.GetProtoHandle(tokenizer.token().numberInt);
			protoHandles.push_back(handle);
		}

		// Sort by prototype name
		sort(protoHandles.begin(), protoHandles.end(), [](uint64_t a, uint64_t b)
		{
			auto nameA = getProtoName(a);
			auto nameB = getProtoName(b);
			return _strcmpi(nameA, nameB);
		});
		logger->info("Loaded {} prototypes for crafting type {}", craftingProtoHandles[i].size(), i);
	}
	
}

static int __cdecl systemInit(const GameSystemConf *conf) {

	mesFuncs.Open("mes\\item_creation.mes", &mesItemCreationText);
	mesFuncs.Open("mes\\item_creation_names.mes", &mesItemCreationNamesText);
	mesFuncs.Open("rules\\item_creation.mes", &mesItemCreationRules);
	loadProtoIds(mesItemCreationRules);

	acceptBtnTextures.loadAccept();
	declineBtnTextures.loadDecline();
	uiFuncs.GetAsset(UiAssetType::Generic, UiGenericAsset::DisabledNormal, &disabledBtnTexture);

	background = uiFuncs.LoadImg("art\\interface\\item_creation_ui\\item_creation.img");

	// TODO !sub_10150F00("rules\\item_creation.mes")

	/*
	tig_texture_register("art\\interface\\item_creation_ui\\craftarms_0.tga", &dword_10BEE38C)
    || tig_texture_register("art\\interface\\item_creation_ui\\craftarms_1.tga", &dword_10BECEE8)
    || tig_texture_register("art\\interface\\item_creation_ui\\craftarms_2.tga", &dword_10BED988)
    || tig_texture_register("art\\interface\\item_creation_ui\\craftarms_3.tga", &dword_10BECEEC)
    || tig_texture_register("art\\interface\\item_creation_ui\\invslot_selected.tga", &dword_10BECDAC)
    || tig_texture_register("art\\interface\\item_creation_ui\\invslot.tga", &dword_10BEE038)
    || tig_texture_register("art\\interface\\item_creation_ui\\add_button.tga", &dword_10BEE334)
    || tig_texture_register("art\\interface\\item_creation_ui\\add_button_grey.tga", &dword_10BED990)
    || tig_texture_register("art\\interface\\item_creation_ui\\add_button_hover.tga", &dword_10BEE2D8)
    || tig_texture_register("art\\interface\\item_creation_ui\\add_button_press.tga", &dword_10BED79C) )
	*/

	return 0;
}

static void __cdecl systemReset() {
}

static void __cdecl systemExit() {
}

class ItemCreation : public TempleFix {
public:
	const char* name() override {
		return "Item Creation UI";
	}
	void apply() override {
		// auto system = UiSystem::getUiSystem("ItemCreation-UI");		
		// system->init = systemInit;
		replaceFunction(0x10150DA0, CraftScrollWandPotionSetItemSpellData);
		replaceFunction(0x10152690, CreateItemResourceCheck);
		replaceFunction(0x10151F60, CreateItemDebitXPGP);
		replaceFunction(0x10152930, UiItemCreationCraftingCostTexts);

	}
} itemCreation;
