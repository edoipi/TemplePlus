from templeplus.pymod import PythonModifier
from toee import *
import tpdp
from utilities import *
print "Registering sp-Mind Blank"

def MindBlankImmunity(attachee, args, evt_obj):
	sp_pkt = evt_obj.spell_packet
	spell_enum = sp_pkt.spell_enum
	if (spell_enum == 0):
		return 0
	spell_entry = tpdp.SpellEntry(spell_enum)
	
	if spell_entry.descriptor & (1<<(D20STD_F_SPELL_DESCRIPTOR_MIND_AFFECTING-14)):
		attachee.float_text_line( "Mind Affecting Immunity", tf_red )
		evt_obj.return_val = 1
	return 0
	
def MindBlankTooltip(attachee, args, evt_obj):
	# Set the tooltip
	evt_obj.append("Mind Blank (" + str(args.get_arg(1)) + " rounds)")
	return 0
	
def MindBlankEffectTooltip(attachee, args, evt_obj):
	# Set the tooltip
	evt_obj.append(tpdp.hash("MIND_BLANK"), -2, " (" + str(args.get_arg(1)) + " rounds)")
	return 0
	
def MindBlankRemove(attachee, args, evt_obj):
	# Show the remove spell effect
	game.particles( 'sp-Mind Blank-END', attachee)
	return 0

mindBlank = PythonModifier("sp-Mind Blank", 4)
mindBlank.AddHook(ET_OnSpellImmunityCheck, EK_NONE, MindBlankImmunity, ()) # spell_id, duration, spare, spare
mindBlank.AddHook(ET_OnGetTooltip, EK_NONE, MindBlankTooltip, ())
mindBlank.AddHook(ET_OnGetEffectTooltip, EK_NONE, MindBlankEffectTooltip, ())
mindBlank.AddHook(ET_OnConditionRemove, EK_NONE, MindBlankRemove, ())
mindBlank.AddSpellCountdownStandardHook()
