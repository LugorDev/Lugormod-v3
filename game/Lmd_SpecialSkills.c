// Native port of UFO's KEBAB skills.cpp. Original source: KEBAB/skills.cpp.
// Binary address hooks have been replaced with the game's public functions.
#include "g_local.h"
#include "Lmd_Accounts_Core.h"
#include "Lmd_Accounts_CustomSkills.h"
#include "Lmd_EntityCore.h"
#include "Lmd_EntityUtil.h"
#include "Lmd_Interact.h"
#include "Lmd_Entities_Public.h"
#include "Lmd_SpecialSkills.h"
#include "Lmd_Professions.h"
#include <limits.h>

extern vmCvar_t lmd_specialSkills;
extern vmCvar_t lmd_skillCreditCost;
#define JEDI_SITH PROF_JEDI
#define MERC PROF_MERC
#define PROFESSION_BOTH (PROF_JEDI + PROF_MERC)
#define PLAYERISVALID(e) ((e) && (e)->inuse && (e)->client && (e)->s.number >= 0 && (e)->s.number < MAX_CLIENTS)
void NPC_SetAnim(gentity_t *ent, int parts, int anim, int flags);
void G_Knockdown(gentity_t *ent, int duration);
qboolean isBuddy(gentity_t *ent, gentity_t *other);
static int Kebab_HasSkill(Account_t *acc, int index, qboolean check);
static int Kebab_PassiveSkill(gentity_t *ent, int index);
static void SSkills_ShopClose(gentity_t *player);
void lmd_menu_enter(gentity_t *player, gentity_t *menu);
void lmd_menu_exit(gentity_t *player);
static gentity_t *SSkills_Entity(int number) {
    if (number < 0 || number >= level.num_entities) return NULL;
    return g_entities[number].inuse ? &g_entities[number] : NULL;
}
static void SSkills_AimTrace(trace_t *trace, gentity_t *ent, int distance, vec3_t mins, vec3_t maxs) {
    vec3_t start, end, forward;
    VectorCopy(ent->client->ps.origin, start);
    start[2] += ent->client->ps.viewheight;
    AngleVectors(ent->client->ps.viewangles, forward, NULL, NULL);
    VectorMA(start, distance, forward, end);
    trap_Trace(trace, start, mins, maxs, end, ent->s.number, MASK_SHOT);
}
typedef struct sskillState_s {
	// client data
	int thinkDelay;
	int regenSlowed;
	int fastRegen;
	int fastRegenCheckTime;
	int cloaked;
	int cloakCheckTime;
	int poisoned;
	int poisonCheckTime;
	int poisonedBy;
	int adrenaline;
	int severed;
	int repulse;
	int highFallDmg;
	int cds[NUM_SKILLS];
	int battlemedtimer;
	int battlemed;
	int screamtimer;
	int insanity;
	int forceshield;
	int screamedon;
	int wallcharge;
	int fuelpour;
	int fueledby;
	int burn;
	int drugged;
	int seeing;
	int breach;
	int incinerate;
	int incineratetime;
	int sparktime;
	int cloakdmg;
	int fpcap;
	int battlemedfx;
	int stompstr;
    int shopEntity;
    int shopProfession;
    Character_t *shopCharacter;
    int shopSelection;
    qboolean shopConfirm;
    int shopConfirmChoice;
    int shopQuotedPrice;
    qboolean shopHadGodmode;
    char shopNotice[96];
} sskillState_t;
typedef struct starSkill_s
{
	char * name;
	char * desc;
	char * cmdName;
	int pts;
	int (*function)(gentity_t*);
	int cd;
	int prof;
	int fp;
	qboolean isPassive;
} starSkill_t;
static sskillState_t p_playerInfo[MAX_CLIENTS];

static starSkill_t starSkills[NUM_SKILLS];

int Kebab_EnableSkills(void)
{
	return lmd_specialSkills.integer && g_gametype.integer == GT_FFA;
}

void Kebab_StompWave(gentity_t* ent)
{
	
	if (!PLAYERISVALID(ent))
		return; 
	int str = p_playerInfo[ent->s.number].stompstr, effectsMultiplier = str / 3, baseDmg = 5, baseKD = 10;
	qboolean gotatleastone = qfalse;
	if (effectsMultiplier > 5)
		effectsMultiplier = 5;
	char  *fx = NULL;
	gentity_t *targ = NULL, *groundEnt = NULL;
	vec3_t org, entDir, null = {90, 0, 0};
	VectorCopy(ent->client->ps.origin, org);
	org[2] -= 10;
	float maxDist = 256;
	if (str < 9)
		fx = "effects/repeater/concussion.efx";
	else
		fx = "effects/ships/proton_impact.efx";
	if (ent->client->ps.groundEntityNum < MAX_CLIENTS)
	{
		groundEnt = SSkills_Entity(ent->client->ps.groundEntityNum);
		if (PLAYERISVALID(groundEnt) && !groundEnt->client->ps.duelInProgress)
		{
			G_Knockdown(groundEnt, 5000);
			G_Damage(groundEnt, ent, ent, NULL, NULL, str >= 15 ? 99999 : str * 5, DAMAGE_NO_PROTECTION | DAMAGE_NO_ARMOR, MOD_FALLING);
		}
	}
	baseDmg *= effectsMultiplier;
	baseKD *= effectsMultiplier;
	G_RadiusDamage(ent->client->ps.origin, ent, baseDmg, maxDist * 2, ent, NULL, MOD_FALLING);
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		targ = SSkills_Entity(i);
		if (ent->s.number == i || !PLAYERISVALID(targ) || (PLAYERISVALID(groundEnt) && groundEnt->s.number == i) || targ->client->sess.spectatorState || !CanDamage(targ, ent->client->ps.origin) || targ->client->ps.duelInProgress || Distance(ent->client->ps.origin, targ->client->ps.origin) > maxDist)
			continue;
		VectorSubtract(targ->client->ps.origin, ent->client->ps.origin, entDir);
		entDir[2] += 0.1f;
		VectorNormalize(entDir);
		G_Throw(targ, entDir, baseKD);
		targ->client->ps.velocity[2] += baseKD * 32;
		G_Knockdown(targ, 3000);
		gotatleastone = qtrue;
	}
	if (gotatleastone)
	{
		while (effectsMultiplier-- > 0)
		{
			G_PlayEffectID(G_EffectIndex(fx), org, null);
			G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/effects/stone_explosion"));
		}
		ent->client->ps.fd.forceGripUseTime = level.time + 3000;
	}
	p_playerInfo[ent->s.number].stompstr = 0;
}

void Kebab_EvadeAnimation(gentity_t* ent)
{
	trap_SendServerCommand( ent->s.number, "print \"^2You have managed to evade a skill!\n\"");
	ent->client->ps.forceHandExtend = HANDEXTEND_DODGE;
	ent->client->ps.forceHandExtendTime = level.time + 300;
	ent->client->ps.forceDodgeAnim = Q_irand(1175, 1180);
	ent->client->ps.powerups[PW_SPEEDBURST] = level.time + 100;
}

int Kebab_HasImmunity(gentity_t* ent)
{
	if (!ent || !ent->client || !ent->inuse || ent->s.number < 0 || ent->s.number >= MAX_CLIENTS)
		return 0;
	if (ent->health <= 0 || ent->client->sess.sessionTeam == TEAM_SPECTATOR ||
		duelInProgress(&ent->client->ps) || ent->client->ps.m_iVehicleNum ||
		(ent->flags & (FL_GODMODE | FL_UNDYING)) || ent->client->invulnerableTimer > level.time ||
		(ent->client->Lmd.flags & SNF_FREEZE) || (ent->client->Lmd.lmd_restrict & 1)) return 1;
	return (ent->client->pers.Lmd.persistantFlags & 8	
		|| ent->client->pers.Lmd.persistantFlags & 16	
		|| ent->client->pers.Lmd.persistantFlags & 128	
		|| ent->client->pers.Lmd.persistantFlags & 256		
		|| p_playerInfo[ent->s.number].forceshield > level.time);
}

int Kebab_CanEvade(gentity_t* target)
{
	if (target->client->ps.forceHandExtend == HANDEXTEND_KNOCKDOWN || target->client->ps.groundEntityNum == 1023)
		return 0;
	Account_t* acc = target->client->pers.Lmd.account;
	int base = 9;
	if ((target->client->ps.fd.forcePowersActive & (1 << FP_SEE) && (!acc || Accounts_Prof_GetProfession(acc) == 3)) || p_playerInfo[target->s.number].adrenaline >= level.time || target->client->ps.pm_flags & PMF_DUCKED)
		base -= 2;
	if (Kebab_HasSkill(acc, SKILL_DODGE, qtrue) || Kebab_HasSkill(acc, SKILL_TRAINING, qtrue))
		base -= 7;
	return base > 0 ? !Q_irand(0, base) : 1;
}

static qboolean SSkills_Evaded(gentity_t *caster, gentity_t *target)
{
    if (!Kebab_CanEvade(target)) return qfalse;
    Disp(caster, "^3Target has managed to evade your skill!");
    Kebab_EvadeAnimation(target);
    return qtrue;
}

static qboolean SSkills_TargetImmune(gentity_t *caster, gentity_t *target)
{
    if (!Kebab_HasImmunity(target)) return qfalse;
    Disp(caster, "^1Target is immune to this special skill.");
    return qtrue;
}

static qboolean SSkills_Standing(gentity_t *ent)
{
    for (int axis = 0; axis < 3; axis++) {
        if (fabs(ent->client->ps.velocity[axis]) > 50.0f) {
            Disp(ent, "^1You have to stand still when using this skill.");
            return qfalse;
        }
    }
    return qtrue;
}

static void SSkills_Dash(gentity_t *ent)
{
    vec3_t angles = {0}, forward, right, up, direction;
    angles[YAW] = ent->client->ps.viewangles[YAW];
    AngleVectors(angles, forward, right, up);
    if (ent->client->pers.cmd.forwardmove)
        VectorScale(forward, ent->client->pers.cmd.forwardmove > 0 ? 1 : -1, direction);
    else if (ent->client->pers.cmd.rightmove)
        VectorScale(right, ent->client->pers.cmd.rightmove > 0 ? 1 : -1, direction);
    else if (ent->client->ps.saberMove <= 3) VectorCopy(up, direction);
    else VectorCopy(forward, direction);
    G_Throw(ent, direction, 256);
    ent->client->ps.velocity[2] += 192;
}

int Kebab_SkillDuration(gentity_t* ent, int* var, int duration)
{
	if (Kebab_HasSkill(ent->client->pers.Lmd.account, SKILL_HR, qtrue))
	{
		duration = floor(duration * 0.5);
	}
	if (var != NULL) *var = level.time + duration;
	return duration;
}

int Kebab_FastRegen(gentity_t* ent)
{
	NPC_SetAnim(ent, SETANIM_BOTH, 1366, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	p_playerInfo[ent->s.number].fastRegen = level.time + 30000;
	p_playerInfo[ent->s.number].fpcap = ent->client->ps.fd.forcePowerMax;
	ent->client->ps.fd.forcePowerMax += 100;
	G_PlayEffectID(G_EffectIndex("concussion/explosion"), ent->client->ps.origin, ent->client->ps.viewangles);
	trap_SendServerCommand( ent->s.number, "print \"^2Your force regeneration is quadrupled for 30 seconds.\n\"");
	return 1;
}

int Kebab_SummonBacta(gentity_t* ent)
{
	if (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_MEDPAC) && ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_MEDPAC_BIG))
	{
		trap_SendServerCommand( ent->s.number, "print \"^1You already have both bactas in your inventory.\n\"");
		return 0;
	}
	NPC_SetAnim(ent, SETANIM_BOTH, 1375, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	G_PlayEffectID(G_EffectIndex("ships/scrape_sparks"), ent->client->ps.origin, ent->client->ps.viewangles);
	G_PlayEffectID(G_EffectIndex("ships/scrape_sparks"), ent->client->ps.origin, ent->client->ps.viewangles);
	G_PlayEffectID(G_EffectIndex("ships/scrape_sparks"), ent->client->ps.origin, ent->client->ps.viewangles);
	G_PlayEffectID(G_EffectIndex("ships/scrape_sparks"), ent->client->ps.origin, ent->client->ps.viewangles);
	G_PlayEffectID(G_EffectIndex("ships/scrape_sparks"), ent->client->ps.origin, ent->client->ps.viewangles);
	ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_MEDPAC);
	ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_MEDPAC_BIG);
	trap_SendServerCommand( ent->s.number, "print \"^2Your bactas have been summoned.\n\"");
	return 1;
}

int Kebab_SlowRegen(gentity_t* ent)
{
	if (ent->client->ps.groundEntityNum == 1023)
	{
		trap_SendServerCommand( ent->s.number, "print \"^1You cannot use this skill while mid air.\n\"");
		return 0;
	}
	trace_t tr;
	SSkills_AimTrace(&tr, ent, 256, NULL, NULL);
	if (tr.fraction < 1.0 && tr.entityNum < MAX_CLIENTS)
	{
		gentity_t* to = SSkills_Entity(tr.entityNum);
		if (!to || !to->client || to->client->ps.duelInProgress || to->client->invulnerableTimer > level.time)
			return 0;
		if (Accounts_Prof_GetProfession(to->client->pers.Lmd.account) == 4)
		{
			trap_SendServerCommand( ent->s.number, "print \"^1This skill cannot be used on mercenaries.\n\"");
			return 0;
		}
		if (SSkills_TargetImmune(ent, to)) return 0;
		if (SSkills_Evaded(ent, to)) return 1;
		NPC_SetAnim(ent, SETANIM_BOTH, 1335, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		
		int duration = Kebab_SkillDuration(to, &p_playerInfo[tr.entityNum].regenSlowed, 10000);
		for (int i = 0; i < NUM_FORCE_POWERS; i++)
			WP_ForcePowerStop(to, i);
		to->client->ps.fd.forcePower = 0;
		trap_SendServerCommand( ent->s.number, va("print \"^3Force regeneration of the target slowed down for %d seconds.\n\"", duration/1000));
		trap_SendServerCommand( tr.entityNum, va("print \"^4Your force regeneration has been slowed down for %d seconds.\n\"", duration/1000));
		G_PlayEffectID(G_EffectIndex("repeater/concussion"), SSkills_Entity(tr.entityNum)->client->ps.origin, SSkills_Entity(tr.entityNum)->client->ps.viewangles);
		return 1;
	}
	else
	{
		trap_SendServerCommand( ent->s.number, "print \"^1Target is too far away!.\n\"");
		return 0;
	}
}

void Kebab_DestructionMissile(gentity_t* ent)
{
	vec3_t  start, angles, forward;
	gentity_t* missile;
	VectorCopy(ent->client->ps.viewangles, angles);
	AngleVectors( angles, forward, NULL, NULL );
	VectorMA(ent->r.currentOrigin, 32, forward, start);
	start[2] += ent->client->ps.viewheight;
	missile = CreateMissile(start, forward, 1000, 30000, ent, qfalse);
	missile->classname = "conc_proj";
	missile->s.weapon = WP_CONCUSSION;
	missile->mass = 10;
	VectorSet( missile->r.maxs, 3, 3, 3 );
	VectorScale( missile->r.maxs, -1, missile->r.mins );
	missile->damage = 80;
	missile->splashDamage = 100;
	missile->splashRadius = 80;
	missile->methodOfDeath = MOD_FORCE_DARK;
	missile->splashMethodOfDeath = MOD_FORCE_DARK;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->bounceCount = 0;
}

int Kebab_ForceDestruction(gentity_t* ent)
{
	NPC_SetAnim(ent, SETANIM_TORSO, BOTH_FORCEPUSH, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	Kebab_DestructionMissile(ent);
	return 1;
}
 
void Kebab_BombThink(gentity_t* self)
{
	gentity_t* ent = SSkills_Entity(self->genericValue15);
	G_PlayEffectID(G_EffectIndex("explosions/wedge_explosion1"), self->s.origin, self->s.angles);
	G_RadiusDamage(self->s.origin, ent, 100000, 512, ent, NULL, MOD_THERMAL);
	G_FreeEntity(self);
	return;
}

void Kebab_TimeBombThink(gentity_t* self)
{
	gentity_t* ent = SSkills_Entity(self->genericValue15);
	if (!PLAYERISVALID(ent) || ent->client->pers.enterTime != self->genericValue14 ||
		ent->health <= 0 || duelInProgress(&ent->client->ps)) {
		G_FreeEntity(self);
		return;
	}
	G_PlayEffectID(G_EffectIndex("explosions/wedge_explosion1"), self->s.origin, self->s.angles);
	G_RadiusDamage(self->s.origin, ent, 1000, 384, ent, NULL, MOD_THERMAL);
	G_FreeEntity(self);
	return;
}

int Kebab_TimeBomb(gentity_t* ent)
{
	int l1 = 0;
	int l2 = 0;
	int l3 = 0;
	vec3_t mins;
	vec3_t maxs;
	int i = 0;
	gentity_t* loop;
	trace_t tr;
	VectorSet(mins, -13, -13, 0);
	VectorSet(maxs, 13, 13, 33);
	SSkills_AimTrace(&tr, ent, 128, mins, maxs);
	if (tr.fraction < 1.0)
	{
		if (tr.contents & CONTENTS_BODY)
		{
			trap_SendServerCommand( ent->s.number, "print \"^1You cannot place a time bomb on a player.\n\"");
			return 0;
		}
		l1 = (int)tr.endpos[0];
		l2 = (int)tr.endpos[1];
		l3 = (int)tr.endpos[2];
		loop = trySpawn(va("classname,misc_model_breakable,spawnflags,1,model,map_objects/imperial/crate_banded,origin,%d %d %d", l1, l2, l3));
		if (!loop)
			return 0;
		NPC_SetAnim(ent, SETANIM_BOTH, 121, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		loop->think = Kebab_TimeBombThink;
		loop->nextthink = level.time + 3000;
		loop->genericValue15 = ent->s.number;
		loop->genericValue14 = ent->client->pers.enterTime;
		trap_SendServerCommand( ent->s.number, "print \"^3You have placed a time bomb that will go off in 3 seconds.\n\"");
		return 1;
	}
	else
	{
		trap_SendServerCommand( ent->s.number, "print \"^1Target is too far away!\n\"");
		return 0;
	}
}

int Kebab_DisableJP(gentity_t* ent)
{
	if (ent->client->ps.groundEntityNum == 1023)
	{
		trap_SendServerCommand( ent->s.number, "print \"^1You cannot use this skill while mid air.\n\"");
		return 0;
	}
	int num = ent->s.number;
	trace_t tr;
	gentity_t* to;
	SSkills_AimTrace(&tr, ent, 3072, NULL, NULL);
	if (tr.fraction < 1.0 && tr.entityNum < MAX_CLIENTS)
	{
		to = SSkills_Entity(tr.entityNum);
		if (!to || !to->client || to->client->ps.duelInProgress || to->client->invulnerableTimer > level.time)
			return 0;
		if (!(to->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK)))
		{
			trap_SendServerCommand( ent->s.number, "print \"^1This skill can only be used on players with a jetpack.\n\"");
			return 0;
		}
		if (SSkills_TargetImmune(ent, to)) return 0;
		if (SSkills_Evaded(ent, to)) return 1;
		NPC_SetAnim(ent, SETANIM_BOTH, 1374, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		if (to->client->jetPackOn == qtrue)
		{
			to->client->jetPackOn = qfalse;
			p_playerInfo[to->s.number].highFallDmg = level.time + 5000;
		}
		to->client->jetPackToggleTime = level.time + 15000;
		to->client->ps.otherKiller = ent->s.number;
		to->client->ps.otherKillerTime = level.time + 5000;
		to->client->ps.otherKillerDebounceTime = level.time + 100;
		trap_SendServerCommand( ent->s.number, "print \"^3Jetpack of the target successfully disabled for 15 seconds.\n\"");
		trap_SendServerCommand( tr.entityNum, "print \"^4Your jetpack has been disabled for 15 seconds.\n\"");
		G_PlayEffectID(G_EffectIndex("repeater/concussion"), SSkills_Entity(tr.entityNum)->client->ps.origin, SSkills_Entity(tr.entityNum)->client->ps.viewangles);
		return 1;
	}
	else
	{
		trap_SendServerCommand( ent->s.number, "print \"^1Target is too far away!\n\"");
		return 0;
	}
}

int Kebab_ForceSever(gentity_t* ent)
{
	if (!SSkills_Standing(ent)) return 0;
	trace_t tr;
	gentity_t* to;
	SSkills_AimTrace(&tr, ent, 256, NULL, NULL);
	if (tr.fraction < 1.0 && tr.entityNum < MAX_CLIENTS)
	{
		to = SSkills_Entity(tr.entityNum);
		if (!to || !to->client || to->client->ps.duelInProgress || to->client->invulnerableTimer > level.time)
			return 0;
		if (Accounts_Prof_GetProfession(to->client->pers.Lmd.account) == 4)
		{
			trap_SendServerCommand( ent->s.number, "print \"^1This skill cannot be used on mercenaries.\n\"");
			return 0;
		}
		if (SSkills_TargetImmune(ent, to)) return 0;
		if (SSkills_Evaded(ent, to)) return 1;
		NPC_SetAnim(ent, SETANIM_BOTH, 1351, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		int duration = Kebab_SkillDuration(to, &p_playerInfo[tr.entityNum].severed, 10000);
		trap_SendServerCommand( ent->s.number, va("print \"^3The target has been severed from the force for %d seconds.\n\"", duration/1000));
		trap_SendServerCommand( tr.entityNum, va("print \"^4You have been severed from the force for %d seconds.\n\"", duration/1000));
		
		to->client->ps.electrifyTime = level.time + 10000;
		to->client->ps.weaponTime = 10;
		to->client->ps.quickerGetup = qtrue;
		to->client->ps.forceHandExtendTime = level.time + 10;
		for (int i = 0; i < NUM_FORCE_POWERS; i++)
			WP_ForcePowerStop(to, i);
		to->client->ps.fd.forcePower = 0;
		G_PlayEffectID(G_EffectIndex("repeater/concussion"), to->client->ps.origin, to->client->ps.viewangles);
		return 1;
	}
	else
	{
		trap_SendServerCommand( ent->s.number, "print \"^1Target is too far away!\n\"");
		return 0;
	}
}

void Kebab_CloakStop(gentity_t* to)
{
	if (!to || !to->client || to->s.number < 0 || to->s.number >= MAX_CLIENTS)
		return;
	p_playerInfo[to->s.number].cloaked = 0;
	to->s.eFlags &= ~EF_NODRAW;
	to->client->ps.eFlags &= ~EF_NODRAW;
	to->r.svFlags &= ~SVF_NOCLIENT;
	to->flags &= ~FL_NOTARGET;
	G_PlayEffectID(G_EffectIndex("ships/heavydmg"), to->client->ps.origin, to->client->ps.viewangles);
	G_PlayEffectID(G_EffectIndex("ships/heavydmg"), to->client->ps.origin, to->client->ps.viewangles);
	G_PlayEffectID(G_EffectIndex("ships/heavydmg"), to->client->ps.origin, to->client->ps.viewangles);
	G_PlayEffectID(G_EffectIndex("ships/heavydmg"), to->client->ps.origin, to->client->ps.viewangles);
	G_PlayEffectID(G_EffectIndex("ships/heavydmg"), to->client->ps.origin, to->client->ps.viewangles);
}

int Kebab_CloakSkill(gentity_t* ent)
{
	ent->s.eFlags |= EF_NODRAW;
	ent->client->ps.eFlags |= EF_NODRAW;
	ent->r.svFlags |= SVF_NOCLIENT;
	ent->flags |= FL_NOTARGET;
	p_playerInfo[ent->s.number].cloaked = level.time + 15000;
	G_PlayEffectID(G_EffectIndex("ships/heavydmg"), ent->client->ps.origin, ent->client->ps.viewangles);
	G_PlayEffectID(G_EffectIndex("ships/heavydmg"), ent->client->ps.origin, ent->client->ps.viewangles);
	G_PlayEffectID(G_EffectIndex("ships/heavydmg"), ent->client->ps.origin, ent->client->ps.viewangles);
	G_PlayEffectID(G_EffectIndex("ships/heavydmg"), ent->client->ps.origin, ent->client->ps.viewangles);
	G_PlayEffectID(G_EffectIndex("ships/heavydmg"), ent->client->ps.origin, ent->client->ps.viewangles);
	trap_SendServerCommand( ent->s.number, "print \"^2You have been cloaked for 15 seconds.\n\"");
	return 1;
}

int Kebab_ForceDash(gentity_t* ent)
{
	SSkills_Dash(ent);
	if (ent->client->ps.saberMove <= 3)
		NPC_SetAnim(ent, SETANIM_BOTH, 1151, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	trap_SendServerCommand( ent->s.number, "print \"^2You have dashed using the force.\n\"");
	return 1;
}

int Kebab_ForceSlow(gentity_t* ent)
{
	if (!SSkills_Standing(ent)) return 0;
	trace_t tr;
	gentity_t* to;
	SSkills_AimTrace(&tr, ent, 256, NULL, NULL);
	if (tr.fraction < 1.0 && tr.entityNum < MAX_CLIENTS)
	{
		to = SSkills_Entity(tr.entityNum);
		if (!to || !to->client || to->client->ps.duelInProgress || to->client->invulnerableTimer > level.time)
			return 0;
		if (SSkills_TargetImmune(ent, to)) return 0;
		if (SSkills_Evaded(ent, to)) return 1;
		WP_ForcePowerStop(to, FP_SPEED);
		NPC_SetAnim(ent, SETANIM_BOTH, 1374, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		G_PlayEffectID(G_EffectIndex("repeater/concussion"), to->client->ps.origin, to->client->ps.viewangles);
		int duration = Kebab_SkillDuration(to, NULL, 15000);
		to->client->Lmd.customSpeed.value = (int)(trap_Cvar_VariableIntegerValue("g_speed") * 0.5);
		to->client->Lmd.customSpeed.time = level.time + duration;
		
		
		trap_SendServerCommand( ent->s.number, va("print \"^3You have slowed the enemy down for %d seconds.\n\"", duration/1000));
		trap_SendServerCommand( to->s.number, va("print \"^4You have been slowed down for %d seconds.\n\"", duration/1000));
		return 1;
	}
	else
	{
		trap_SendServerCommand( ent->s.number, "print \"^1Target is too far away!\n\"");
		return 0;
	}
}

int Kebab_MercSprint(gentity_t* ent)
{
	p_playerInfo[ent->s.number].adrenaline = level.time + 10000;
	ent->client->Lmd.customSpeed.value = trap_Cvar_VariableIntegerValue("g_speed") * 1.5;
	ent->client->Lmd.customSpeed.time = level.time + 10000;
	NPC_SetAnim(ent, SETANIM_BOTH, 1361, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	G_PlayEffectID(G_EffectIndex("concussion/explosion"), ent->client->ps.origin, ent->client->ps.viewangles);
	trap_SendServerCommand( ent->s.number, "print \"^2You have used a stimulant to increase your agility for 10 seconds.\n\"");
	return 1;
}

int Kebab_PoisonDart(gentity_t* ent)
{
	if (!SSkills_Standing(ent)) return 0;
	trace_t tr;
	SSkills_AimTrace(&tr, ent, 1024, NULL, NULL);
	if (tr.fraction < 1.0 && tr.entityNum < MAX_CLIENTS)
	{
		gentity_t* to = SSkills_Entity(tr.entityNum);
		if (!to || !to->client || to->client->ps.duelInProgress || to->client->invulnerableTimer > level.time || p_playerInfo[tr.entityNum].poisoned > level.time)
			return 0;
		if (SSkills_TargetImmune(ent, to)) return 0;
		if (SSkills_Evaded(ent, to)) return 1;
		int duration = Kebab_SkillDuration(to, &p_playerInfo[tr.entityNum].poisoned, 10000);
		NPC_SetAnim(ent, SETANIM_BOTH, 1335, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		
		p_playerInfo[tr.entityNum].poisonedBy = ent->s.number;
		G_PlayEffectID(G_EffectIndex("noghri_stick/gas_cloud"), to->client->ps.origin, to->client->ps.viewangles);
		G_PlayEffectID(G_EffectIndex("noghri_stick/muzzle_flash"), ent->client->ps.origin, ent->client->ps.viewangles);
		trap_SendServerCommand( tr.entityNum, va("print \"^4You have been poisoned for %d seconds.\n\"", duration/1000));
		return 1;
	}
	else
	{
		trap_SendServerCommand( ent->s.number, "print \"^1Target is too far away!\n\"");
		return 0;
	}
}

void Kebab_Repulse(gentity_t* ent, qboolean pull, float knockback, float distance)
{
	if (ent->client->pers.Lmd.persistantFlags & 16)
	{

		return;
	}
	for ( int i = 0; i < MAX_CLIENTS; i++ )
	{
		gentity_t* other = SSkills_Entity(i);
		if (ent->s.number == i || !other || !other->inuse || other->client->sess.spectatorState || !ForcePowerUsableOn(ent, other, pull ? FP_PULL : FP_PUSH) || !CanDamage(other, ent->client->ps.origin))
			continue;
		float dist = max(1.0f, Distance(ent->client->ps.origin, other->client->ps.origin));
		if (dist <= distance)
		{
			vec3_t entDir;
			VectorSubtract( other->client->ps.origin, ent->client->ps.origin, entDir );
			entDir[2] += 0.1f;
			VectorNormalize( entDir );
			if (pull)
			{
				VectorScale(entDir, -1, entDir);
				other->client->pushEffectTime = level.time + 1000;
				if (other->client->ps.torsoAnim != BOTH_PULLED_INAIR_B)
					NPC_SetAnim(other, SETANIM_BOTH, BOTH_PULLED_INAIR_B, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				if (other->client->ps.legsAnim != BOTH_PULLED_INAIR_B)
					other->client->ps.velocity[2] += 16;
				other->client->ps.weaponTime = 1000;
				knockback /= (distance / dist);
			}
			G_Throw( other, entDir, knockback );
			if (!pull)
			{
				G_Knockdown(other, 2000);
				other->client->ps.velocity[2] += 1024;
				other->client->ps.otherKiller = ent->s.number;
				other->client->ps.otherKillerTime = level.time + 5000;
				other->client->ps.otherKillerDebounceTime = level.time + 100;
				if (!Kebab_HasImmunity(other))
				{
					p_playerInfo[other->s.number].highFallDmg = level.time + 5000;
				}
				vec3_t pos;
				VectorCopy(ent->client->ps.origin, pos);
				pos[2] += 16;
				G_PlayEffectID(G_EffectIndex("concussion/explosion"), pos, ent->client->ps.viewangles);
			}
		}
	}
}

int Kebab_RepulseBegin(gentity_t* ent)
{
	if (ent->client->ps.groundEntityNum == 1023)
	{
		trap_SendServerCommand( ent->s.number, "print \"^1You cannot use this skill while mid air.\n\"");
		return 0;
	}
	NPC_SetAnim(ent, SETANIM_BOTH, BOTH_FORCE_RAGE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	p_playerInfo[ent->s.number].repulse = level.time + 1000;
	return 1;
}

void Kebab_BattleMedDo(gentity_t* ent, float distance)
{
	if (ent->client->pers.Lmd.persistantFlags & 16)
	{

		return;
	}
	vec3_t org;
	VectorCopy(ent->client->ps.origin, org);
	org[2] -= 10;
	G_PlayEffectID(G_EffectIndex("effects/misc/possession"), org, ent->client->ps.viewangles);
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		gentity_t* target = SSkills_Entity(i);
		if (!target || !target->client || !target->inuse || !ForcePowerUsableOn(ent, target, FP_TEAM_HEAL) || target == ent || target->health <= 0 || !isBuddy(ent, target) || target->client->sess.spectatorState)
			continue;
		float cur = Distance(ent->client->ps.origin, target->client->ps.origin);
		if (cur <= distance)
		{
			Account_t* acc = target->client->pers.Lmd.account;
			if (!acc || Accounts_Prof_GetProfession(acc) != 4)
			{
				target->client->ps.fd.forcePower = 100;
				target->client->ps.powerups[PW_FORCE_BOON] = level.time + 30000;
				target->client->ps.powerups[PW_YSALAMIRI] = 0;
			}
			else
			{
				Add_Ammo(target, AMMO_BLASTER, 600);
				Add_Ammo(target, AMMO_POWERCELL, 600);
				Add_Ammo(target, AMMO_METAL_BOLTS, 600);
				target->client->ps.powerups[PW_YSALAMIRI] = level.time + 30000;
				target->client->ps.jetpackFuel = 100;
			}
			p_playerInfo[i].battlemedfx = level.time + 30000;
			target->health = target->client->ps.stats[STAT_MAX_HEALTH];
			target->client->ps.stats[STAT_ARMOR] = target->client->ps.stats[STAT_MAX_HEALTH];
			target->client->ps.stats[STAT_HEALTH] = target->client->ps.stats[STAT_MAX_HEALTH];
			target->client->ps.powerups[PW_BATTLESUIT] = level.time + 30000;
			vec3_t pos;
			VectorCopy(target->client->ps.origin, pos);
			pos[2] -= 10;
			G_PlayEffectID(G_EffectIndex("effects/force/heal2"), pos, target->client->ps.viewangles);
			trap_SendServerCommand( target->s.number, va("print \"^7%s ^4has used Battle Meditation on you!\n\"", ent->client->pers.netname));
		}
	}
}

int Kebab_BattleMeditation(gentity_t* ent)
{
	if (ent->client->ps.legsAnim != BOTH_MEDITATE || ent->client->ps.torsoAnim != BOTH_MEDITATE) 
	{
		trap_SendServerCommand( ent->s.number, "print \"^1You must be meditating to use this skill.\n\"");	
		return 0;
	}
	trap_SendServerCommand( ent->s.number, "print \"^2Battle Meditation will be soon used on your friends, if you continue to meditate.\n\"");	
	p_playerInfo[ent->s.number].battlemed = 1;
	return 1;
}

void Kebab_ForceScreamDo(gentity_t* ent, float distance)
{
	if (ent->client->ps.groundEntityNum == 1023)
	{
		p_playerInfo[ent->s.number].screamtimer = 0;
		ent->client->Lmd.customSpeed.time = 0;
		ent->client->Lmd.customSpeed.value = 0;
		return;
	}
	if ((ent->client->pers.Lmd.persistantFlags & 16) || ent->client->isHacking)
	{

		return;
	}
	vec3_t org;
	VectorCopy(ent->client->ps.origin, org);
	org[2] += 10;
	G_Sound(ent, CHAN_VOICE, G_SoundIndex(va("sound/chars/howler/howl_yell%d", Q_irand(1,5))));
	G_PlayEffectID(G_EffectIndex("effects/howler/sonic"), org, ent->client->ps.viewangles);
	NPC_SetAnim(ent, SETANIM_TORSO, BOTH_FORCE_RAGE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART);
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		gentity_t* target = SSkills_Entity(i);
		if (!target || !target->client || !target->inuse || !ForcePowerUsableOn(ent, target, FP_PUSH) || target == ent || target->health <= 0 || isBuddy(ent, target) || target->client->sess.spectatorState || target->client->ps.duelInProgress || !CanDamage(target, ent->client->ps.origin))
			continue;
		float cur = Distance(ent->client->ps.origin, target->client->ps.origin);
		if (cur <= distance)
		{
			if (Kebab_CanEvade(target))
				continue;
			target->client->Lmd.customSpeed.value = trap_Cvar_VariableIntegerValue("g_speed") * 0.2;
			target->client->Lmd.customSpeed.time = level.time + 1000;
			target->client->ps.weaponTime = 1000;
			p_playerInfo[target->s.number].screamedon = level.time + 1000;
			if (p_playerInfo[target->s.number].sparktime >= level.time)
			{
				p_playerInfo[ent->s.number].sparktime = 0;
				ent->client->Lmd.customSpeed.time = 0;
				ent->client->Lmd.customSpeed.value = 0;
			}
			NPC_SetAnim(target, SETANIM_TORSO, 1380, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			if (!Kebab_HasImmunity(target))
			{
				G_Damage(target, ent, ent, NULL, NULL, 5, DAMAGE_NO_ARMOR , MOD_UNKNOWN);
			}
		}
	}
}

int Kebab_ForceScream(gentity_t* ent)
{
	if (ent->client->ps.groundEntityNum == 1023)
	{
		trap_SendServerCommand( ent->s.number, "print \"^1You cannot use this skill while mid air.\n\"");
		return 0;
	}
	p_playerInfo[ent->s.number].screamtimer = 15000 + level.time;
	ent->client->Lmd.customSpeed.value = trap_Cvar_VariableIntegerValue("g_speed") * 0.2;
	ent->client->Lmd.customSpeed.time = level.time + 15000;
	return 1;
}

void Kebab_ForceInsanityDo(gentity_t* target)
{
	vec3_t randAngle;
	randAngle[0] = crandom() * 87;
	randAngle[1] = crandom() * 180;
	randAngle[2] = 0;
	SetClientViewAngle(target, randAngle);
	vec3_t org;
	VectorCopy(target->client->ps.origin, org);
	org[2] += 25;
	G_PlayEffectID(G_EffectIndex("effects/force/confusion_old"), org, target->client->ps.viewangles);
	NPC_SetAnim(target, SETANIM_TORSO, 1380, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	target->client->ps.weaponTime = 1000;
}

int Kebab_ForceInsanity(gentity_t* ent)
{
	if (!SSkills_Standing(ent)) return 0;
	trace_t tr;
	gentity_t* to;
	SSkills_AimTrace(&tr, ent, 256, NULL, NULL);
	if (tr.fraction < 1.0 && tr.entityNum < MAX_CLIENTS)
	{
		to = SSkills_Entity(tr.entityNum);
		if (!to || !to->client || to->client->ps.duelInProgress || to->client->invulnerableTimer > level.time || to->client->sess.spectatorState)
			return 0;
		if (SSkills_TargetImmune(ent, to)) return 0;
		NPC_SetAnim(ent, SETANIM_BOTH, 1335, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		int duration = Kebab_SkillDuration(to, &p_playerInfo[tr.entityNum].insanity, 10000);
		trap_SendServerCommand( ent->s.number, va("print \"^3The target has been driven insane for %d seconds.\n\"", duration/1000));
		trap_SendServerCommand( tr.entityNum, va("print \"^4You have been driven insane for %d seconds.\n\"", duration/1000));
		
		
		
		
		
		p_playerInfo[tr.entityNum].wallcharge = 0;
		p_playerInfo[tr.entityNum].screamtimer = 0;
		G_PlayEffectID(G_EffectIndex("repeater/concussion"), to->client->ps.origin, to->client->ps.viewangles);
		to->client->ps.saberHolstered = 2;
		return 1;
	}
	else
	{
		trap_SendServerCommand( ent->s.number, "print \"^1Target is too far away!\n\"");
		return 0;
	}
}

int Kebab_ForceShield(gentity_t* ent)
{
	p_playerInfo[ent->s.number].forceshield = level.time + 30000;
	ent->flags |= FL_SHIELDED;
	
	ent->client->ps.powerups[PW_SHIELDHIT] = level.time + 30000;
	ent->client->ps.powerups[PW_BATTLESUIT] = level.time + 30000;
	
	
	p_playerInfo[ent->s.number].insanity = 0;
	p_playerInfo[ent->s.number].regenSlowed = 0;
	p_playerInfo[ent->s.number].poisoned = 0;
	p_playerInfo[ent->s.number].poisonedBy = -1;
	p_playerInfo[ent->s.number].fuelpour = 0;
	p_playerInfo[ent->s.number].fueledby = -1;
	p_playerInfo[ent->s.number].burn = 0;
	p_playerInfo[ent->s.number].drugged = 0;
	return 1;
}

void Kebab_EnergyMissile(gentity_t* ent)
{
	vec3_t  start, angles, forward;
	gentity_t* missile;
	VectorCopy(ent->client->ps.viewangles, angles);
	AngleVectors( angles, forward, NULL, NULL );
	VectorMA(ent->r.currentOrigin, 32, forward, start);
	start[2] += ent->client->ps.viewheight;
	missile = CreateMissile(start, forward, 750, 30000, ent, qfalse);
	missile->classname = "sskill_energyball";
	missile->s.weapon = WP_DEMP2;
	missile->mass = 10;
	VectorSet( missile->r.maxs, 5, 5, 5 );
	VectorScale( missile->r.maxs, -1, missile->r.mins );
	missile->damage = 40;
	missile->splashDamage = 40;
	missile->splashRadius = 64;
	missile->methodOfDeath = MOD_DEMP2_ALT;
	missile->splashMethodOfDeath = MOD_DEMP2_ALT;
	missile->clipmask = MASK_SHOT;
	missile->bounceCount = 0;
}

int Kebab_EnergyBall(gentity_t* ent)
{
	if (ent->client->ps.groundEntityNum == 1023)
	{
		trap_SendServerCommand( ent->s.number, "print \"^1You cannot use this skill while mid air.\n\"");
		return 0;
	}
	NPC_SetAnim(ent, SETANIM_TORSO, BOTH_FORCEPUSH, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	Kebab_EnergyMissile(ent);
	return 1;
}

void Kebab_FireThink(gentity_t* self)
{
	if (self->timestamp <= level.time) { G_FreeEntity(self); return; }
	G_PlayEffectID(G_EffectIndex("effects/env/fire_wall"), self->r.currentOrigin, self->r.currentAngles);
	self->nextthink = level.time + 100;
}

void Kebab_FireSingleWall(gentity_t* ent, vec3_t forward)
{
	vec3_t  start;
	gentity_t* missile;
	VectorMA(ent->r.currentOrigin, 32, forward, start);
	start[2] += ent->client->ps.viewheight;
	missile = CreateMissile(start, forward, 1000, 3000, ent, qfalse);
	missile->classname = "firewall";
	missile->s.weapon = WP_NONE;
	missile->mass = 10;
	VectorSet( missile->r.maxs, 10, 10, 10 );
	VectorScale( missile->r.maxs, -1, missile->r.mins );
	missile->damage = 1;
	missile->splashDamage = 1;
	missile->splashRadius = 1;
	missile->methodOfDeath = MOD_LAVA;
	missile->splashMethodOfDeath = MOD_LAVA;
	missile->clipmask = MASK_SHOT;
	missile->s.speed = 200;
	missile->bounceCount = 0;
	missile->think = Kebab_FireThink;
	missile->timestamp = level.time + 3000;
	missile->nextthink = level.time + 100;
	missile->genericValue1 = ent->s.number;
}

void Kebab_FireWallDo(gentity_t* ent)
{
	NPC_SetAnim(ent, SETANIM_BOTH, BOTH_VICTORY_STRONG, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_RESTART | SETANIM_FLAG_HOLD);
	vec3_t angles, forward, backwards, right, left;
	for (int i = 0; i < 3; i++)
	{
		VectorCopy(ent->client->ps.viewangles, angles);
		VectorSet(angles, 0, angles[YAW], 0);
		angles[YAW] = crandom() * 64;
		AngleVectors( angles, forward, right, NULL );
		VectorScale(right, -1, left);
		VectorScale(forward, -1, backwards);
		Kebab_FireSingleWall(ent, forward);
		Kebab_FireSingleWall(ent, backwards);
		Kebab_FireSingleWall(ent, right);
		Kebab_FireSingleWall(ent, left);
	}
	G_RadiusDamage(ent->client->ps.origin, ent, 500, 256, ent, NULL, MOD_LAVA);
}

int Kebab_FireWall(gentity_t* ent)
{
	if (!SSkills_Standing(ent)) return 0;
	NPC_SetAnim(ent, SETANIM_BOTH, BOTH_VICTORY_FAST, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_RESTART | SETANIM_FLAG_HOLD);
	p_playerInfo[ent->s.number].wallcharge = level.time + 2000;
	// Use weapon and movement locks; do not take ownership of an admin freeze.
	ent->client->ps.weaponTime = 2000;
	ent->client->Lmd.customSpeed.value = 0;
	ent->client->Lmd.customSpeed.time = level.time + 2000;
	return 1;
}

int Kebab_PourFuel(gentity_t* ent)
{
	if (ent->client->ps.groundEntityNum == 1023)
	{
		trap_SendServerCommand( ent->s.number, "print \"^1You cannot use this skill while mid air.\n\"");
		return 0;
	}
	trace_t tr;
	SSkills_AimTrace(&tr, ent, 192, NULL, NULL);
	if (tr.fraction < 1.0 && tr.entityNum < MAX_CLIENTS)
	{
		gentity_t* to = SSkills_Entity(tr.entityNum);
		if (!to || !to->client || to->client->ps.duelInProgress || to->client->invulnerableTimer > level.time)
			return 0;
		if (SSkills_TargetImmune(ent, to)) return 0;
		if (SSkills_Evaded(ent, to)) return 1;
		NPC_SetAnim(ent, SETANIM_TORSO, BOTH_FORCEPUSH, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		int duration = Kebab_SkillDuration(to, &p_playerInfo[tr.entityNum].fuelpour, 30000);
		p_playerInfo[tr.entityNum].fueledby = ent->s.number;
		trap_SendServerCommand( ent->s.number, va("print \"^3You have poured fuel on your target for %d seconds!\n\"", duration/1000));
		trap_SendServerCommand( tr.entityNum, va("print \"^4You have been poured with fuel for %d seconds!\n\"", duration/1000));
		G_PlayEffectID(G_EffectIndex("env/water_splash"), to->client->ps.origin, to->client->ps.viewangles);
		return 1;
	}
	else
	{
		trap_SendServerCommand( ent->s.number, "print \"^1Target is too far away!.\n\"");
		return 0;
	}
}

int Kebab_Drug(gentity_t* ent)
{
	if (ent->client->ps.groundEntityNum == 1023)
	{
		trap_SendServerCommand( ent->s.number, "print \"^1You cannot use this skill while mid air.\n\"");
		return 0;
	}
	trace_t tr;
	gentity_t* to;
	SSkills_AimTrace(&tr, ent, 192, NULL, NULL);
	if (tr.fraction < 1.0 && tr.entityNum < MAX_CLIENTS)
	{
		to = SSkills_Entity(tr.entityNum);
		if (!to || !to->client || to->client->ps.duelInProgress || to->client->invulnerableTimer > level.time || to->client->sess.spectatorState)
			return 0;
		if (SSkills_TargetImmune(ent, to)) return 0;
		
		NPC_SetAnim(ent, SETANIM_BOTH, 1335, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		int duration = Kebab_SkillDuration(to, &p_playerInfo[tr.entityNum].drugged, 10000);
		trap_SendServerCommand( ent->s.number, va("print \"^3The target has been drugged for %d seconds.\n\"", duration/1000));
		trap_SendServerCommand( tr.entityNum, va("print \"^4You have been drugged for %d seconds.\n\"", duration/1000));
		
		G_PlayEffectID(G_EffectIndex("repeater/concussion"), to->client->ps.origin, to->client->ps.viewangles);
		return 1;
	}
	else
	{
		trap_SendServerCommand( ent->s.number, "print \"^1Target is too far away!\n\"");
		return 0;
	}
}

int Kebab_FuelJump(gentity_t* ent)
{
	if (!(ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK)))
	{
		trap_SendServerCommand( ent->s.number, "print \"^1You cannot use this skill without a jetpack equipped.\n\"");
		return 0;
	}
	ent->client->jetPackOn = qtrue;
	SSkills_Dash(ent);
	NPC_SetAnim(ent, SETANIM_BOTH, 1151, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	trap_SendServerCommand( ent->s.number, "print \"^2You have dashed.\n\"");
	return 1;
}

int Kebab_Wallhack(gentity_t* ent)
{
	trap_SendServerCommand( ent->s.number, "print \"^2Seeing activated.\n\"");
	ent->client->ps.fd.forcePowersActive |= (1 << FP_SEE);
	p_playerInfo[ent->s.number].seeing = 0;
	return 1;
}

int Kebab_ForceBreach(gentity_t* ent)
{
	trace_t tr;
	gentity_t* to;
	SSkills_AimTrace(&tr, ent, 512, NULL, NULL);
	if (tr.fraction < 1.0 && tr.entityNum < MAX_CLIENTS)
	{
		to = SSkills_Entity(tr.entityNum);
		if (!to || !to->client || to->client->ps.duelInProgress || to->client->invulnerableTimer > level.time || to->client->sess.spectatorState)
			return 0;
		if (SSkills_TargetImmune(ent, to)) return 0;
		NPC_SetAnim(ent, SETANIM_BOTH, 1335, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		int duration = Kebab_SkillDuration(to, &p_playerInfo[tr.entityNum].breach, 5000);
		vec3_t entDir;
		VectorSubtract(to->client->ps.origin, ent->client->ps.origin, entDir);
		entDir[2] += 0.1f;
		VectorNormalize(entDir);
		G_Throw(to, entDir, 50);
		G_Knockdown(to, 100);
		p_playerInfo[tr.entityNum].adrenaline = 0;
		if (p_playerInfo[tr.entityNum].fastRegen && p_playerInfo[tr.entityNum].fpcap > 0)
			to->client->ps.fd.forcePowerMax = p_playerInfo[tr.entityNum].fpcap;
		to->flags &= ~FL_SHIELDED;
		p_playerInfo[tr.entityNum].forceshield = 0;
		p_playerInfo[tr.entityNum].seeing = 0;
		p_playerInfo[tr.entityNum].fastRegen = 0;
		p_playerInfo[tr.entityNum].screamtimer = 0;
		if (p_playerInfo[to->s.number].cloaked) Kebab_CloakStop(to);
		for (int i = 0; i < NUM_FORCE_POWERS; i++)
		{
			if (i < MAX_POWERUPS)
			{
				to->client->ps.powerups[i] = 0;
			}
			WP_ForcePowerStop(to, i);
		}
		trap_SendServerCommand( ent->s.number, va("print \"^3The target has become a wound in the Force for %d seconds. Target's passive sskills have been disabled for %d seconds.\n\"", duration/1000, duration/1000 + 5));
		trap_SendServerCommand( tr.entityNum, va("print \"^4You have become a wound in the Force for %d seconds. Your passive sskills have been disabled for %d seconds.\n\"", duration/1000, duration/1000 + 5));
		G_PlayEffectID(G_EffectIndex("repeater/concussion"), to->client->ps.origin, to->client->ps.viewangles);
		return 1;
	}
	else
	{
		trap_SendServerCommand( ent->s.number, "print \"^1Target is too far away!\n\"");
		return 0;
	}
	return 1;
}

int Kebab_Incinerate(gentity_t* ent)
{
	if (!SSkills_Standing(ent)) return 0;
	trace_t tr;
	gentity_t* to;
	SSkills_AimTrace(&tr, ent, 256, NULL, NULL);
	if (tr.fraction < 1.0 && tr.entityNum < MAX_CLIENTS)
	{
		to = SSkills_Entity(tr.entityNum);
		if (!to || !to->client || to->client->ps.duelInProgress || to->client->invulnerableTimer > level.time || to->client->sess.spectatorState)
			return 0;
		if (SSkills_TargetImmune(ent, to)) return 0;
		trap_SendServerCommand( tr.entityNum, "print \"^4Extinguish the fire in 2 seconds or die!\n\"");
		NPC_SetAnim(ent, SETANIM_BOTH, 1335, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		p_playerInfo[tr.entityNum].incineratetime = level.time + 2000;
		p_playerInfo[tr.entityNum].incinerate = ent->s.number;
		return 1;
	}
	else
	{
		trap_SendServerCommand( ent->s.number, "print \"^1Target is too far away!\n\"");
		return 0;
	}
	return 1;
}

void Kebab_ForceCombustionDo(gentity_t* ent)
{
	trace_t tr;
	gentity_t* to;
	int damage;
	if (ent->client->ps.groundEntityNum == 1023)
	{
		p_playerInfo[ent->s.number].sparktime = 0;
		ent->client->Lmd.customSpeed.time = 0;
		ent->client->Lmd.customSpeed.value = 0;
		return;
	}
	ent->client->ps.weaponTime = 300;
	NPC_SetAnim(ent, SETANIM_BOTH, 1335, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART);
	SSkills_AimTrace(&tr, ent, 8192, NULL, NULL);
	if (tr.fraction < 1.0 && tr.entityNum < MAX_CLIENTS)
	{
		to = SSkills_Entity(tr.entityNum);
		if (!to || !to->client || to->client->ps.duelInProgress || to->client->invulnerableTimer > level.time || to->client->sess.spectatorState)
			return;
		if (Kebab_HasImmunity(to))
			return;
		damage = 1000 / sqrt(max(1.0f, Distance(ent->client->ps.origin, to->client->ps.origin)));
		G_Damage(to, ent, ent, NULL, NULL, damage, DAMAGE_NO_PROTECTION, MOD_LAVA);
		G_PlayEffectID(G_EffectIndex("effects/explosions/droidexplosion1"), tr.endpos, ent->client->ps.viewangles);
	}
}

int Kebab_ForceCombustion(gentity_t* ent)
{
	if (!SSkills_Standing(ent)) return 0;
	p_playerInfo[ent->s.number].sparktime = level.time + 10*1000; 
	for (int i = 0; i < NUM_FORCE_POWERS; i++)
		WP_ForcePowerStop(ent, i);
	ent->client->Lmd.customSpeed.value = trap_Cvar_VariableIntegerValue("g_speed") * 0.1;
	ent->client->Lmd.customSpeed.time = level.time + 10000;
	return 1;
}

int Kebab_PassiveSkill(gentity_t* ent, int index)
{
	if (!Kebab_EnableSkills())
		return 0;
	if (Kebab_HasImmunity(ent) || ent->client->Lmd.lmd_restrict & 32 || ent->client->invulnerableTimer >= level.time)
		return 0;
	Account_t* acc = ent->client->pers.Lmd.account;
	if (!acc || (p_playerInfo[ent->s.number].breach > 0 && p_playerInfo[ent->s.number].breach + 5000 >= level.time) ||
		(index == SKILL_BLASTARMOR && ent->client->ps.stats[STAT_ARMOR] < ent->client->ps.stats[STAT_MAX_HEALTH] * 0.7) ||
		(ent->client->ps.duelInProgress && Accounts_Prof_GetProfession(ent->client->pers.Lmd.account) != MERC) || 
		(index == SKILL_LIGHTNINGSHIELD && ent->client->ps.stats[STAT_ARMOR] < 1))
	{
		return 0;
	}
	return Kebab_HasSkill(acc, index, qtrue);
}

void Kebab_InitSkills(void)
{
    static const starSkill_t catalogue[NUM_SKILLS] = {
        { "Force Body", "The force user enacts a ritual using his own blood, quadrupling his rate of regeneration for 30 seconds but also dealing damage to him.", "forcebody", 3, Kebab_FastRegen, 120*1000, JEDI_SITH, 0, qfalse }, // SKILL_FORCEBODY
        { "Force Destruction", "Creates and launches an energy projectile made out of Force.", "destruction", 2, Kebab_ForceDestruction, 40*1000, JEDI_SITH, 50, qfalse }, // SKILL_DESTRUCTION
        { "Force Cloak", "With Force Power consumption, a Force user can alter the path of photons such that he becomes invisible unless he reveals himself.", "forcecloak", 1, Kebab_CloakSkill, 40*1000, JEDI_SITH, 0, qfalse }, // SKILL_CLOAK
        { "Force Dash", "The force user uses the force to dash with great speed in the selected direction.", "forcedash", 1, Kebab_ForceDash, 20*1000, JEDI_SITH, 20, qfalse }, // SKILL_DASH
        { "Jetpack Disabling", "Electronically scrambles the target's jetpack through the force, disabling it for 15 seconds.", "disablejp", 3, Kebab_DisableJP, 60*1000, JEDI_SITH, 50, qfalse }, // SKILL_DISABLEJP
        { "Force Sever", "Strips the target from the force for 10 seconds.", "forcesever", 4, Kebab_ForceSever, 80*1000, JEDI_SITH, 80, qfalse }, // SKILL_SEVER
        { "Force Slow", "The force user uses telekinesis to slow down an enemy for 15 seconds, sacrificing his own Force energy in the process.", "forceslow", 3, Kebab_ForceSlow, 60*1000, JEDI_SITH, 50, qfalse }, // SKILL_SLOW
        { "Force Stomping", "The force user gains ability to knockdown and inflict damage to opponents by crouched stomping on their heads.", "forcestomp", 4, NULL, 0, JEDI_SITH, 0, qtrue }, // SKILL_FORCESTOMP
        { "Force Repulse", "Concentrates energy to create a sudden burst that throws surrounding enemies into great distance.", "repulse", 5, Kebab_RepulseBegin, 80*1000, JEDI_SITH, 50, qfalse }, // SKILL_REPULSE
        { "Slow Regeneration", "The mercenary makes use of crystals that weaken one's connection to the force, slowing the regeneration of their enemy down for 10 seconds.", "slowregen", 3, Kebab_SlowRegen, 80*1000, MERC, 0, qfalse }, // SKILL_SLOWREGEN
        { "Time Bomb", "Using extensive electronic knowledge, the mercenary builds a devastating time bomb that goes off in 3 seconds.", "timebomb", 5, Kebab_TimeBomb, 120*1000, MERC, 50, qfalse }, // SKILL_TIMEBOMB
        { "Poison Dart", "The mercenary makes use of a highly toxic dart to shoot the enemy, dealing damage over the course of 10 seconds.", "poisondart", 4, Kebab_PoisonDart, 40*1000, MERC, 20, qfalse }, // SKILL_POISON
        { "Adrenaline Stimulation", "The mercenary uses biofeedback techniques to unleash more adrenaline to his blood stream, increasing his overall agility for 10 seconds.", "adrenaline", 2, Kebab_MercSprint, 60*1000, MERC, 50, qfalse }, // SKILL_ADRENALINE
        { "Summoning Bactas", "The mercenary summons big and medium bacta cannisters that restore HP per usage.", "summonbacta", 1, Kebab_SummonBacta, 40*1000, MERC, 0, qfalse }, // SKILL_BACTAS
        { "Force Resistance", "Increases mercenary's resilience to knockdowns caused by forces Push and Pull. Also bestows full immunity to Mind Trick and Lightning", "forceresist", 5, NULL, 0, MERC, 0, qtrue }, // SKILL_FORCERESIST
        { "Weapon Proficiency", "The mercenary shall spawn with every advanced weaponry existing in the Universe.", "weaponprof", 4, NULL, 0, MERC, 0, qtrue }, // SKILL_WPPROF
        { "Force Reflex", "Improves user's reflex and allows dodging Disruptor Rifle with 75 percent success rate. Increases the chance of evading skills.", "forcereflex", 4, NULL, 0, JEDI_SITH, 0, qtrue }, // SKILL_DODGE
        { "Battle Meditation", "Heals user's buddies, improves their defense and gives them Force Boon or Ysalimari, depending on their profession.", "battlemed", 2, Kebab_BattleMeditation, 120*1000, JEDI_SITH, 100, qfalse }, // SKILL_BATTLEMED
        { "Force Scream", "Conjures a powerful shockwave, which slows down and lightly damages enemies nearby.", "forcescream", 3, Kebab_ForceScream, 100*1000, JEDI_SITH, 50, qfalse }, // SKILL_SCREAM
        { "Force Insanity", "Drives the target insane for 10 seconds. This skill cannot be evaded.", "insanity", 5, Kebab_ForceInsanity, 100*1000, JEDI_SITH, 80, qfalse }, // SKILL_INSANITY
        { "Force Shield", "Creates a powerful shield made out of Force for 30 seconds, which is capable of deflecting light weapons, gives temporary immunity to Force Powers , sSkills, and improves their vitality. Reduces damage dealt by the user and weakens his Force Powers.", "forceshield", 3, Kebab_ForceShield, 120*1000, JEDI_SITH, 80, qfalse }, // SKILL_SHIELD
        { "Energy Ball", "Conjures a powerful Force Ball made out of concentrated energy which knocks down target on impact.", "energyball", 4, Kebab_EnergyBall, 60*1000, JEDI_SITH, 50, qfalse }, // SKILL_ENERGY
        { "Blast Armor", "Reduces splash damage dealt to the user and makes lightsaber hits bounce off, preventing the enemy saberist from chaining his attacks. Works with at least 70 percent of shield.", "blastarmor", 6, NULL, 0, MERC, 0, qtrue }, // SKILL_BLASTARMOR
        { "Implant Regeneration", "Through special implants, user is able to regenerate his health, armor and ammo by meditating with weapon melee equipped.", "implant", 4, NULL, 0, MERC, 0, qtrue }, // SKILL_IMPLANTREGEN
        { "Fire Wall", "The mercenary defuels his jetpack completely in order to conjure a deadly fire wave.", "firewall", 3, Kebab_FireWall, 80*1000, MERC, 80, qfalse }, // SKILL_FIREWALL
        { "Mobility", "Thanks to the special training, user is able to reach Jedi-like speed and perform various acrobations. Increases the chance of evading skills.", "mobility", 5, NULL, 0, MERC, 0, qtrue }, // SKILL_TRAINING
        { "Pour Fuel", "User pours some of his fuel on a selected target, making him vulnerable to any fire attack.", "pourfuel", 4, Kebab_PourFuel, 100*1000, MERC, 50, qfalse }, // SKILL_POUR
        { "Drug Injection", "Drugs the target for 10 seconds, making him more vulnerable to any kind of damage.", "drug", 3, Kebab_Drug, 80*1000, MERC, 20, qfalse }, // SKILL_DRUG
        { "Fuel Jump", "Releases extra flame from the jetpack, causing the user to dash in the selected direction.", "fueljump", 1, Kebab_FuelJump, 20*1000, MERC, 20, qfalse }, // SKILL_FJ
        { "Seeing", "Mercenary uses a device, which allows him to see through walls for a short period of time.", "seeing", 3, Kebab_Wallhack, 30*1000, MERC, 0, qfalse }, // SKILL_AM
        { "High Resistance", "Provides high resistance to injury, improves the user's vitality and reduces the duration of the sskills casted on the user by 50 percent.", "highresistance", 6, NULL, 0, JEDI_SITH, 0, qtrue }, // SKILL_HR
        { "Force Incineration", "Sets the target on fire through the Force. If the target fails to extinguish it in two seconds, they die.", "incinerate", 5, Kebab_Incinerate, 120*1000, JEDI_SITH, 70, qfalse }, // SKILL_INCINERATE
        { "Force Breach", "Turns the target into a wound in the Force, temporarily removing all buffing spells, including High Resistance and Blast Armor. This skill cannot be evaded.", "forcebreach", 4, Kebab_ForceBreach, 100*1000, JEDI_SITH, 50, qfalse }, // SKILL_BREACH
        { "Cortosis Fist", "When in melee, the user is given two small cortosis daggers which double the melee damage and turn off victim's lightsabers for 2 seconds.", "cortosisfist", 2, NULL, 0, MERC, 0, qtrue }, // SKILL_CF
        { "Demolition", "Extensively educates the user in the art of crafting explosives, thereby increasing the range and damage of their thermal detonators, detonation packs and trip mines. Using any of aforementioned weapons will now consume fuel.", "demolition", 4, NULL, 0, MERC, 0, qtrue }, // SKILL_DEMO
        { "Critical Hit", "Installs a pure rylith power cell in all user's weapons, increasing the direct hit damage by 10-15 points and giving a random chance of knocking down the victim.", "critical", 6, NULL, 0, MERC, 0, qtrue }, // SKILL_CS
        { "Fire Magic", "Thanks to an ancient gem, the user is granted knowledge about Fire Magic, making him immune to any fire attacks. Increases damage inflicted with a lightsaber by 10-15 points (doesn't apply to saber throw).", "firemagic", 6, NULL, 0, JEDI_SITH, 0, qtrue }, // SKILL_FIRESWORD
        { "Force Combustion", "Heavily damages the target.", "combustion", 5, Kebab_ForceCombustion, 120*1000, JEDI_SITH, 70, qfalse }, // SKILL_SPARKSPAM
        { "Lightning Shield", "The user is able to redirect some of lightsaber damage at the attacker, as long as he has any shield left.", "lshield", 3, NULL, 0, MERC, 0, qtrue }, // SKILL_LIGHTNINGSHIELD
        { "Grapple Choke", "Mercenary is able to choke his opponents using Grapple Hook.", "grapple", 5, NULL, 0, MERC, 0, qtrue }, // SKILL_GRAPPLECHOKE
    };
    memcpy(starSkills, catalogue, sizeof(starSkills));
}

static int Kebab_HasSkill(Account_t *acc, int index, qboolean check)
{
    if (!acc || index < 0 || index >= NUM_SKILLS || !Kebab_EnableSkills())
        return 0;
    int prof = Accounts_Prof_GetProfession(acc);
    if (check && (Accounts_Prof_GetLevel(acc) < 40 ||
        (starSkills[index].prof != PROFESSION_BOTH && prof != starSkills[index].prof)))
        return 0;
    char *value = Accounts_Custom_GetValue(acc, va("sskill_%s", starSkills[index].cmdName));
    if (value && !strcmp(value, "1")) return 1;
    // Preserve unlocks imported from legacy UFO accounts.
    value = Accounts_Custom_GetValue(acc, index > LAST_SKILL ? "plugin_skillBitmask337_2" : "plugin_skillBitmask337");
    int bit = index > LAST_SKILL ? index - LAST_SKILL - 1 : index;
    return value && ((strtoul(value, NULL, 10) & (1UL << bit)) != 0);
}

qboolean SSkills_Has(gentity_t *ent, int index)
{
    return PLAYERISVALID(ent) && index >= 0 && index < NUM_SKILLS && Kebab_PassiveSkill(ent, index);
}

static int SSkills_Price(int index)
{
    int unit = max(1, lmd_skillCreditCost.integer);
    if (index < 0 || index >= NUM_SKILLS || starSkills[index].pts <= 0 ||
        unit > INT_MAX / starSkills[index].pts)
        return 0;
    return unit * starSkills[index].pts;
}

qboolean SSkills_Buy(gentity_t *ent, int index)
{
    int price = SSkills_Price(index);
    if (!PLAYERISVALID(ent) || !Kebab_EnableSkills() || index < 0 || index >= NUM_SKILLS || price <= 0)
        return qfalse;
    Account_t *acc = ent->client->pers.Lmd.account;
    if (!acc || !Account_GetActiveCharacter(acc) || Accounts_Prof_GetLevel(acc) < 40) {
        Disp(ent, "^3Log in with a level 40 character to purchase special skills.");
        return qfalse;
    }
    int prof = Accounts_Prof_GetProfession(acc);
    if (starSkills[index].prof != PROFESSION_BOTH && starSkills[index].prof != prof) {
        Disp(ent, "^3This skill belongs to a different profession.");
        return qfalse;
    }
    if (Kebab_HasSkill(acc, index, qfalse)) {
        Disp(ent, "^3You already own this skill.");
        return qfalse;
    }
    int balance = Accounts_GetCredits(acc);
    if (balance < price) {
        Disp(ent, "^3You do not have enough credits.");
        return qfalse;
    }
    char key[96];
    Q_strncpyz(key, va("sskill_%s", starSkills[index].cmdName), sizeof(key));
    char *old = Accounts_Custom_GetValue(acc, key);
    char previous[MAX_STRING_CHARS];
    Q_strncpyz(previous, old ? old : "", sizeof(previous));
    Accounts_SetCredits(acc, balance - price);
    Accounts_Custom_SetValue(acc, key, "1");
    if (!Accounts_SaveChecked(acc)) {
        Accounts_SetCredits(acc, balance);
        Accounts_Custom_SetValue(acc, key, previous);
        Disp(ent, "^1Purchase could not be saved. Your credits have been returned.");
        return qfalse;
    }
    Disp(ent, va("^2Purchased %s for %d credits. %s", starSkills[index].name, price,
        starSkills[index].isPassive ? "Passive benefits apply on your next spawn." : va("Use /sskills %s.", starSkills[index].cmdName)));
    return qtrue;
}

static int SSkills_Find(const char *name)
{
    for (int i = 0; i < NUM_SKILLS; i++)
        if (starSkills[i].cmdName && !Q_stricmp(name, starSkills[i].cmdName)) return i;
    return -1;
}

void SSkills_Command(gentity_t *ent, int iArg)
{
    if (!PLAYERISVALID(ent) || !Kebab_EnableSkills()) return;
    Account_t *acc = ent->client->pers.Lmd.account;
    char name[MAX_STRING_CHARS];
    if (iArg > 0 && iArg <= NUM_SKILLS)
        Q_strncpyz(name, starSkills[iArg - 1].cmdName, sizeof(name));
    else
        trap_Argv(1, name, sizeof(name));
    if (!name[0]) {
        Disp(ent, "^5Special skills: visit a skill shop to purchase. Use /sskills <name> to activate an owned skill.");
        for (int i = 0; i < NUM_SKILLS; i++) {
            if (starSkills[i].prof == Accounts_Prof_GetProfession(acc))
                Disp(ent, va("^3%s ^7- %s (%s, %d credits)%s", starSkills[i].cmdName,
                    starSkills[i].name, starSkills[i].isPassive ? "passive" : "active", SSkills_Price(i),
                    Kebab_HasSkill(acc, i, qfalse) ? " ^2OWNED" : ""));
        }
        return;
    }
    int index = SSkills_Find(name);
    if (index < 0 || !Kebab_HasSkill(acc, index, qtrue)) {
        Disp(ent, "^3You do not own that skill for your current profession, or are below level 40.");
        return;
    }
    starSkill_t *skill = &starSkills[index];
    if (skill->isPassive || !skill->function) {
        Disp(ent, va("^5%s: ^7%s", skill->name, skill->desc));
        return;
    }
    sskillState_t *state = &p_playerInfo[ent->s.number];
    if (ent->health <= 0 || ent->client->sess.sessionTeam == TEAM_SPECTATOR ||
        duelInProgress(&ent->client->ps) || ent->client->ps.m_iVehicleNum ||
        (ent->client->Lmd.flags & SNF_FREEZE) || ent->client->pers.Lmd.jailTime > level.time ||
        (ent->client->Lmd.lmd_restrict & 2) || Kebab_HasImmunity(ent) ||
        state->severed > level.time || state->breach > level.time ||
        ent->client->ps.forceHandExtend != HANDEXTEND_NONE || ent->client->ps.weaponTime > 0) {
        Disp(ent, "^3You cannot use special skills right now.");
        return;
    }
    if (state->cds[index] > level.time) {
        Disp(ent, va("^3Skill cooldown: %d seconds.", (state->cds[index] - level.time + 999) / 1000));
        return;
    }
    int *resource = Accounts_Prof_GetProfession(acc) == PROF_MERC ? &ent->client->ps.jetpackFuel : &ent->client->ps.fd.forcePower;
    if (*resource < skill->fp) {
        Disp(ent, "^3You do not have enough Force power or jetpack fuel.");
        return;
    }
    if (skill->function(ent)) {
        *resource = max(0, *resource - skill->fp);
        state->cds[index] = level.time + skill->cd;
        ent->client->invulnerableTimer = 0;
    }
}

void SSkills_Init(void)
{
    memset(p_playerInfo, 0, sizeof(p_playerInfo));
    memset(starSkills, 0, sizeof(starSkills));
    Kebab_InitSkills();
}

void SSkills_Reset(gentity_t *ent, qboolean disconnect)
{
    if (!ent || !ent->client || ent->s.number < 0 || ent->s.number >= MAX_CLIENTS) return;
    SSkills_ShopClose(ent);
    sskillState_t *state = &p_playerInfo[ent->s.number];
    int cooldowns[NUM_SKILLS];
    memcpy(cooldowns, state->cds, sizeof(cooldowns));
    if (state->cloaked) Kebab_CloakStop(ent);
    if (state->forceshield) {
        ent->flags &= ~FL_SHIELDED;
        if (ent->client->ps.powerups[PW_SHIELDHIT] == state->forceshield) ent->client->ps.powerups[PW_SHIELDHIT] = 0;
        if (ent->client->ps.powerups[PW_BATTLESUIT] == state->forceshield) ent->client->ps.powerups[PW_BATTLESUIT] = 0;
    }
    if (state->fastRegen && state->fpcap > 0)
        ent->client->ps.fd.forcePowerMax = state->fpcap;
    memset(state, 0, sizeof(*state));
    if (!disconnect) memcpy(state->cds, cooldowns, sizeof(cooldowns));
    state->poisonedBy = state->fueledby = state->incinerate = -1;
    if (disconnect) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sskillState_t *other = &p_playerInfo[i];
            if (other->poisonedBy == ent->s.number) other->poisoned = 0;
            if (other->fueledby == ent->s.number) other->fuelpour = other->burn = 0;
            if (other->incinerate == ent->s.number) {
                other->incineratetime = 0;
                other->incinerate = -1;
            }
        }
    }
}

void SSkills_Spawn(gentity_t *ent)
{
    if (!PLAYERISVALID(ent) || !lmd_specialSkills.integer) return;
    Account_t *acc = ent->client->pers.Lmd.account;
    if (Kebab_HasSkill(acc, SKILL_WPPROF, qtrue)) {
        int weapons[] = {WP_DEMP2, WP_CONCUSSION, WP_ROCKET_LAUNCHER, WP_TRIP_MINE, WP_DET_PACK};
        for (int i = 0; i < 5; i++) {
            ent->client->ps.stats[STAT_WEAPONS] |= 1 << weapons[i];
            ent->client->ps.ammo[weaponData[weapons[i]].ammoIndex] = ammoData[weaponData[weapons[i]].ammoIndex].max;
        }
    }
    if (Kebab_HasSkill(acc, SKILL_HR, qtrue)) ent->client->ps.stats[STAT_MAX_HEALTH] = 125;
    if (Kebab_HasSkill(acc, SKILL_TRAINING, qtrue)) {
        ent->client->ps.fd.forcePowersKnown |= 1 << FP_LEVITATION;
        ent->client->ps.fd.forcePowerLevel[FP_LEVITATION] = 2;
        ent->client->ps.trueNonJedi = qfalse;
    }
    if (Kebab_HasSkill(acc, SKILL_AM, qtrue)) ent->client->ps.fd.forcePowerLevel[FP_SEE] = 2;
    if (Kebab_HasSkill(acc, SKILL_GRAPPLECHOKE, qtrue)) {
        ent->client->ps.fd.forcePowersKnown |= 1 << FP_GRIP;
        ent->client->ps.fd.forcePowerLevel[FP_GRIP] = 3;
        ent->client->ps.trueNonJedi = qfalse;
    }
}

static void SSkills_Heal(gentity_t *ent, int health, int armor)
{
    int cap = ent->client->ps.stats[STAT_MAX_HEALTH];
    ent->health = min(cap, ent->health + health);
    ent->client->ps.stats[STAT_HEALTH] = ent->health;
    ent->client->ps.stats[STAT_ARMOR] = min(cap, ent->client->ps.stats[STAT_ARMOR] + armor);
}

void SSkills_Think(gentity_t *ent)
{
    if (!PLAYERISVALID(ent)) return;
    sskillState_t *s = &p_playerInfo[ent->s.number];
    if (s->shopEntity && ent->client->Lmd.lmdMenu.entityNum != s->shopEntity)
        SSkills_ShopClose(ent);
    if (!Kebab_EnableSkills() || ent->health <= 0 || ent->client->sess.sessionTeam == TEAM_SPECTATOR ||
        duelInProgress(&ent->client->ps) || !ent->client->pers.Lmd.account) {
        SSkills_Reset(ent, qfalse);
        return;
    }
    if (s->thinkDelay > level.time) return;
    s->thinkDelay = level.time + 200;
    qboolean immune = (ent->flags & (FL_GODMODE | FL_UNDYING)) ||
        (ent->client->Lmd.lmd_restrict & 1) || ent->client->invulnerableTimer > level.time;
    qboolean rolling = ent->client->ps.legsAnim >= BOTH_ROLL_F && ent->client->ps.legsAnim <= BOTH_ROLL_R;
    if (s->severed > level.time) ent->client->ps.fd.forcePower = 0;
    if (s->fastRegen && s->fastRegen <= level.time) {
        ent->client->ps.fd.forcePowerMax = s->fpcap;
        ent->client->ps.fd.forcePower = min(s->fpcap, ent->client->ps.fd.forcePower);
        s->fastRegen = s->fpcap = 0;
    }
    if (s->fastRegen > level.time && s->fastRegenCheckTime <= level.time && !immune) {
        G_Damage(ent, ent, ent, NULL, NULL, 2, DAMAGE_NO_ARMOR, MOD_FORCE_DARK);
        s->fastRegenCheckTime = level.time + 1000;
    }
    if (s->forceshield && s->forceshield <= level.time) {
        ent->flags &= ~FL_SHIELDED;
        s->forceshield = 0;
    }
    if (s->battlemedfx > level.time) SSkills_Heal(ent, 4, 2);
    if (SSkills_Has(ent, SKILL_IMPLANTREGEN) && ent->client->ps.weapon == WP_MELEE &&
        ent->client->ps.torsoAnim == BOTH_MEDITATE && ent->client->ps.legsAnim == BOTH_MEDITATE) {
        SSkills_Heal(ent, 2, 1);
        Add_Ammo(ent, AMMO_BLASTER, 5);
        Add_Ammo(ent, AMMO_POWERCELL, 5);
        Add_Ammo(ent, AMMO_METAL_BOLTS, 5);
    }
    if (s->cloaked) {
        if (s->cloaked <= level.time || ent->client->pers.cmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK)) {
            Kebab_CloakStop(ent);
        } else if (s->cloakCheckTime <= level.time) {
            if (ent->client->ps.fd.forcePower < 6) Kebab_CloakStop(ent);
            else ent->client->ps.fd.forcePower -= 6;
            s->cloakCheckTime = level.time + 1000;
        }
    }
    if (s->poisoned > level.time && s->poisonCheckTime <= level.time && !immune) {
        gentity_t *source = SSkills_Entity(s->poisonedBy);
        if (source && source->client && source->health > 0)
            G_Damage(ent, source, source, NULL, NULL, Q_irand(9, 10), DAMAGE_NO_ARMOR, MOD_SLIME);
        s->poisonCheckTime = level.time + 1000;
    }
    if (rolling) {
        s->burn = s->fuelpour = s->incineratetime = 0;
        s->incinerate = -1;
    }
    if (s->fuelpour > level.time && s->burn > level.time && !immune && !SSkills_Has(ent, SKILL_FIRESWORD)) {
        gentity_t *source = SSkills_Entity(s->fueledby);
        if (source && source->client) G_Damage(ent, source, source, NULL, NULL, 4, 0, MOD_LAVA);
    }
    if (s->incinerate >= 0 && s->incineratetime > 0 && !immune) {
        gentity_t *source = SSkills_Entity(s->incinerate);
        if (source && source->client && !SSkills_Has(ent, SKILL_FIRESWORD)) {
            int damage = s->incineratetime <= level.time ? 99999 : 5;
            G_Damage(ent, source, source, NULL, NULL, damage, DAMAGE_NO_ARMOR, MOD_LAVA);
        }
        if (s->incineratetime <= level.time) {
            s->incineratetime = 0;
            s->incinerate = -1;
        }
    }
    if (s->wallcharge && s->wallcharge <= level.time) {
        s->wallcharge = 0;
        if (ent->health > 0) Kebab_FireWallDo(ent);
    }
    if (s->sparktime > level.time) Kebab_ForceCombustionDo(ent);
    if (s->screamtimer > level.time) Kebab_ForceScreamDo(ent, 256);
    if (s->insanity > level.time && !immune && !Kebab_CanEvade(ent)) Kebab_ForceInsanityDo(ent);
    if (s->repulse) {
        if (s->repulse > level.time) Kebab_Repulse(ent, qtrue, 64, 256);
        else {
            s->repulse = 0;
            Kebab_Repulse(ent, qfalse, 512, 128);
        }
    }
    if (s->battlemed) {
        if (ent->client->ps.torsoAnim != BOTH_MEDITATE || ent->client->ps.legsAnim != BOTH_MEDITATE)
            s->battlemed = s->battlemedtimer = 0;
        else if ((s->battlemedtimer += 200) >= 3000) {
            s->battlemed = s->battlemedtimer = 0;
            Kebab_BattleMedDo(ent, 512);
        }
    }
    if (Kebab_HasSkill(ent->client->pers.Lmd.account, SKILL_AM, qtrue) &&
        (ent->client->ps.fd.forcePowersActive & (1 << FP_SEE))) {
        if ((s->seeing += 200) >= 20000) WP_ForcePowerStop(ent, FP_SEE);
    }
    if (SSkills_Has(ent, SKILL_FORCESTOMP)) {
        if (ent->client->ps.groundEntityNum == ENTITYNUM_NONE && ent->client->ps.velocity[2] < 0 &&
            (ent->client->ps.pm_flags & PMF_DUCKED)) s->stompstr++;
        else {
            if (s->stompstr > 4 && (ent->client->ps.pm_flags & PMF_DUCKED)) Kebab_StompWave(ent);
            s->stompstr = 0;
        }
    }
}

int SSkills_Regen(gentity_t *ent, int amount)
{
    if (!PLAYERISVALID(ent) || !lmd_specialSkills.integer) return max(1, amount);
    sskillState_t *s = &p_playerInfo[ent->s.number];
    if (s->severed > level.time || s->sparktime > level.time) return 0;
    if (s->regenSlowed > level.time && Q_irand(0, 4)) return 0;
    amount = max(1, amount);
    return s->fastRegen > level.time ? amount * 4 : amount;
}

qboolean SSkills_ForceAllowed(gentity_t *attacker, gentity_t *target, int power)
{
    if (!lmd_specialSkills.integer) return qtrue;
    if (PLAYERISVALID(attacker) && (p_playerInfo[attacker->s.number].severed > level.time ||
        p_playerInfo[attacker->s.number].breach > level.time)) return qfalse;
    if (!PLAYERISVALID(target) || power == FP_TEAM_HEAL || power == FP_TEAM_FORCE) return qtrue;
    if (p_playerInfo[target->s.number].forceshield > level.time) return qfalse;
    if (SSkills_Has(target, SKILL_FORCERESIST) &&
        (power == FP_LIGHTNING || power == FP_TELEPATHY ||
         ((power == FP_PUSH || power == FP_PULL) && !Q_irand(0, 2)))) return qfalse;
    return qtrue;
}

int SSkills_Damage(gentity_t *target, gentity_t *attacker, gentity_t *inflictor, int damage, int flags, int mod)
{
    if (!lmd_specialSkills.integer || damage <= 0) return damage;
    if (PLAYERISVALID(target)) {
        sskillState_t *s = &p_playerInfo[target->s.number];
        if (mod == MOD_FALLING && s->highFallDmg > level.time) damage = min(INT_MAX / 24, damage) * 24;
        if (s->cloaked && (s->cloakdmg += damage) >= 25) Kebab_CloakStop(target);
        if (!(flags & DAMAGE_NO_PROTECTION)) {
            if (mod == MOD_LAVA && SSkills_Has(target, SKILL_FIRESWORD)) return 0;
            if (SSkills_Has(target, SKILL_HR)) damage = max(1, (int)(damage * 0.8f));
            if ((flags & DAMAGE_RADIUS) && SSkills_Has(target, SKILL_BLASTARMOR)) damage = max(1, damage / 2);
            if ((mod == MOD_DISRUPTOR || mod == MOD_DISRUPTOR_SNIPER) && SSkills_Has(target, SKILL_DODGE) && Q_irand(0, 3))
                return 0;
        }
        if (s->drugged > level.time) damage = min(INT_MAX / 2, damage) * 2;
        if (s->fuelpour > level.time && mod == MOD_LAVA) s->burn = level.time + 10000;
        if (inflictor && inflictor->classname && !Q_stricmp(inflictor->classname, "sskill_energyball") && PLAYERISVALID(attacker) &&
            Kebab_HasSkill(attacker->client->pers.Lmd.account, SKILL_ENERGY, qtrue) && !Kebab_HasImmunity(target))
            G_Knockdown(target, 1000);
    }
    if (PLAYERISVALID(attacker) && attacker != target) {
        sskillState_t *s = &p_playerInfo[attacker->s.number];
        if (s->cloaked) Kebab_CloakStop(attacker);
        if (s->forceshield > level.time) damage = max(1, damage / 2);
        if (PLAYERISVALID(target) && !Kebab_HasImmunity(target)) {
            if (mod == MOD_SABER && SSkills_Has(target, SKILL_BLASTARMOR) && !attacker->client->ps.saberInFlight)
                attacker->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;
            if (SSkills_Has(attacker, SKILL_CF) && mod == MOD_MELEE) {
                damage = min(INT_MAX / 2, damage) * 2;
                if (target->client->ps.weapon == WP_SABER) {
                    target->client->ps.saberHolstered = 2;
                    target->client->ps.weaponTime = max(2000, target->client->ps.weaponTime);
                }
            }
            if (SSkills_Has(attacker, SKILL_CS) && mod >= MOD_BRYAR_PISTOL && mod <= MOD_CONC_ALT) {
                damage += Q_irand(5, 10);
                if (!Q_irand(0, 9)) G_Knockdown(target, 500);
            }
            if (SSkills_Has(attacker, SKILL_FIRESWORD) && mod == MOD_SABER && !attacker->client->ps.saberInFlight &&
                s->cds[SKILL_FIRESWORD] <= level.time) {
                damage += Q_irand(10, 15);
                s->cds[SKILL_FIRESWORD] = level.time + 300;
            }
        }
        if (PLAYERISVALID(target) && (mod == MOD_SABER || mod == MOD_MELEE) &&
            SSkills_Has(target, SKILL_LIGHTNINGSHIELD) && !Kebab_HasImmunity(attacker))
            G_Damage(attacker, target, target, NULL, NULL, max(1, damage * 3 / 4), DAMAGE_NO_ARMOR, MOD_DEMP2);
    }
    return damage;
}

void SSkills_Demolition(gentity_t *attacker, float *damage, float *radius, int mod)
{
    if (!PLAYERISVALID(attacker) || !SSkills_Has(attacker, SKILL_DEMO) || attacker->client->ps.jetpackFuel < 10)
        return;
    if (mod != MOD_THERMAL && mod != MOD_THERMAL_SPLASH && mod != MOD_TRIP_MINE_SPLASH &&
        mod != MOD_TIMED_MINE_SPLASH && mod != MOD_DET_PACK_SPLASH) return;
    *damage *= 1.5f;
    *radius *= 1.5f;
    attacker->client->ps.jetpackFuel -= 10;
}

static qboolean SSkills_ShopAccess(gentity_t *shop, gentity_t *player)
{
    extern qboolean PlayerUseableCheck(gentity_t *, gentity_t *);
    return shop && PLAYERISVALID(player) && Kebab_EnableSkills() && !(shop->flags & FL_INACTIVE) &&
        PlayerUseableCheck(shop, player) && DistanceSquared(shop->r.currentOrigin, player->client->ps.origin) <= shop->radius * shop->radius;
}

#define SSKILLS_SHOP_PAGE_SIZE 5

static void SSkills_ShopClose(gentity_t *player)
{
    sskillState_t *s = &p_playerInfo[player->s.number];
    if (!s->shopEntity) return;
    int number = s->shopEntity;
    qboolean restoreGodmode = s->shopHadGodmode;
    s->shopEntity = 0;
    s->shopCharacter = NULL;
    s->shopConfirm = qfalse;
    if (player->client->Lmd.lmdMenu.entityNum == number) {
        lmd_menu_exit(player);
        trap_SendServerCommand(player->s.number, "cp \" \"");
    }
    if (restoreGodmode) player->flags |= FL_GODMODE;
    if (number > 0 && number < MAX_GENTITIES && player->client->Lmd.interact.targ == &g_entities[number])
        Interact_Clear(player);
}

static int SSkills_ShopCatalogue(int profession, int skills[NUM_SKILLS])
{
    int count = 0;
    for (int i = 0; i < NUM_SKILLS; i++) {
        if (starSkills[i].prof == profession || starSkills[i].prof == PROFESSION_BOTH)
            skills[count++] = i;
    }
    return count;
}

static void SSkills_ShopDescription(char *message, int size, const char *description)
{
    int column = 0;
    for (const char *p = description; *p; p++) {
        char text[2] = {*p, 0};
        if (*p == ' ' && column >= 54) {
            text[0] = '\n';
            column = 0;
        } else column++;
        Q_strcat(message, size, text);
    }
    Q_strcat(message, size, "\n\n");
}

static void SSkills_ShopDraw(gentity_t *player)
{
    sskillState_t *s = &p_playerInfo[player->s.number];
    Account_t *account = player->client->pers.Lmd.account;
    int skills[NUM_SKILLS];
    int count = SSkills_ShopCatalogue(s->shopProfession, skills);
    char message[MAX_STRING_CHARS];
    Q_strncpyz(message, va("^5Special Skill Shop\n^7Credits: ^2%d\n\n", Accounts_GetCredits(account)), sizeof(message));
    if (s->shopConfirm && s->shopSelection < count) {
        int index = skills[s->shopSelection];
        qboolean owned = Kebab_HasSkill(account, index, qfalse);
        Q_strcat(message, sizeof(message), va("^3%s ^7(%s)\n", starSkills[index].name,
            starSkills[index].isPassive ? "Passive" : "Active"));
        SSkills_ShopDescription(message, sizeof(message), starSkills[index].desc);
        Q_strcat(message, sizeof(message), va("^7Price: ^2%d credits\n", SSkills_Price(index)));
        if (owned) Q_strcat(message, sizeof(message), "^2Already owned\n\n^3[ Back ]\n");
        else {
            Q_strcat(message, sizeof(message), s->shopConfirmChoice == 0 ? "\n^3[ Buy skill ]\n^7  Cancel\n" : "\n^7  Buy skill\n^3[ Cancel ]\n");
        }
    } else {
        int page = min(s->shopSelection / SSKILLS_SHOP_PAGE_SIZE, max(0, (count - 1) / SSKILLS_SHOP_PAGE_SIZE));
        Q_strcat(message, sizeof(message), va("^7Page %d/%d\n", page + 1, max(1, (count + SSKILLS_SHOP_PAGE_SIZE - 1) / SSKILLS_SHOP_PAGE_SIZE)));
        int end = min(count, (page + 1) * SSKILLS_SHOP_PAGE_SIZE);
        for (int row = page * SSKILLS_SHOP_PAGE_SIZE; row < end; row++) {
            int index = skills[row];
            Q_strcat(message, sizeof(message), va("%s%s%s - %d CR%s\n",
                row == s->shopSelection ? "^3[ " : "^7  ", starSkills[index].name,
                row == s->shopSelection ? " ]" : "", SSkills_Price(index),
                Kebab_HasSkill(account, index, qfalse) ? " ^2Owned" : ""));
        }
        Q_strcat(message, sizeof(message), s->shopSelection == count ? "\n^3[ Exit ]\n" : "\n^7  Exit\n");
    }
    if (s->shopNotice[0]) Q_strcat(message, sizeof(message), va("\n^2%s\n", s->shopNotice));
    Q_strcat(message, sizeof(message), "\n^7Forward/Back: select | Use/Attack: confirm\nAlt Attack: back / close");
    trap_SendServerCommand(player->s.number, va("cp \"%s\"", message));
    player->client->Lmd.lmdMenu.nextUpdateTime = level.time + 1000;
}

static qboolean SSkills_ShopSessionValid(gentity_t *player, gentity_t *shop)
{
    sskillState_t *s = &p_playerInfo[player->s.number];
    return shop->inuse && shop->classname && !Q_stricmp(shop->classname, "lmd_skillshop") &&
        s->shopEntity == shop->s.number && player->client->Lmd.lmdMenu.entityNum == s->shopEntity &&
        player->client->pers.Lmd.character == s->shopCharacter &&
        Account_GetActiveCharacter(player->client->pers.Lmd.account) == s->shopCharacter &&
        PlayerAcc_Prof_GetProfession(player) == s->shopProfession && PlayerAcc_Prof_GetLevel(player) >= 40 &&
        player->health > 0 && !player->client->ps.m_iVehicleNum && player->client->sess.sessionTeam != TEAM_SPECTATOR &&
        !duelInProgress(&player->client->ps) && !(player->client->Lmd.flags & SNF_FREEZE) &&
        player->client->pers.Lmd.jailTime <= level.time && SSkills_ShopAccess(shop, player);
}

void SSkills_ShopOpen(gentity_t *shop, gentity_t *player)
{
    if (!SSkills_ShopAccess(shop, player) || shop->s.number <= 0 || shop->s.number >= MAX_GENTITIES) return;
    int profession = PlayerAcc_Prof_GetProfession(player);
    if (!player->client->pers.Lmd.character || PlayerAcc_Prof_GetLevel(player) < 40 ||
        (profession != PROF_JEDI && profession != PROF_MERC) || player->health <= 0 ||
        player->client->sess.sessionTeam == TEAM_SPECTATOR || player->client->ps.m_iVehicleNum || duelInProgress(&player->client->ps) ||
        (player->client->Lmd.flags & SNF_FREEZE) || player->client->pers.Lmd.jailTime > level.time) {
        Disp(player, "^3Use the skill shop with an active level 40 Jedi or Mercenary.");
        return;
    }
    SSkills_ShopClose(player);
    if (player->client->Lmd.lmdMenu.entityNum) lmd_menu_exit(player);
    sskillState_t *s = &p_playerInfo[player->s.number];
    s->shopHadGodmode = (player->flags & FL_GODMODE) != 0;
    s->shopEntity = shop->s.number;
    s->shopCharacter = player->client->pers.Lmd.character;
    s->shopProfession = profession;
    s->shopSelection = 0;
    s->shopConfirm = qfalse;
    s->shopNotice[0] = 0;
    Interact_Set(player, shop, shop->r.currentOrigin, shop->radius, NULL);
    lmd_menu_enter(player, shop);
    SSkills_ShopDraw(player);
}

static void SSkills_ShopSelect(gentity_t *player)
{
    sskillState_t *s = &p_playerInfo[player->s.number];
    int skills[NUM_SKILLS];
    int count = SSkills_ShopCatalogue(s->shopProfession, skills);
    if (s->shopSelection >= count) { SSkills_ShopClose(player); return; }
    int index = skills[s->shopSelection];
    if (!s->shopConfirm) {
        s->shopConfirm = qtrue;
        s->shopConfirmChoice = 0;
        s->shopQuotedPrice = SSkills_Price(index);
        s->shopNotice[0] = 0;
        return;
    }
    if (s->shopConfirmChoice || Kebab_HasSkill(player->client->pers.Lmd.account, index, qfalse)) {
        s->shopConfirm = qfalse;
        return;
    }
    if (s->shopQuotedPrice != SSkills_Price(index)) {
        s->shopQuotedPrice = SSkills_Price(index);
        Q_strncpyz(s->shopNotice, "Price changed. Please confirm again.", sizeof(s->shopNotice));
        return;
    }
    qboolean bought = SSkills_Buy(player, index);
    Q_strncpyz(s->shopNotice, bought ? "Skill purchased!" : "Purchase failed. No credits charged.", sizeof(s->shopNotice));
    s->shopConfirm = qfalse;
}

void SSkills_ShopMenu(gentity_t *player, gentity_t *shop, usercmd_t *cmd)
{
    if (!PLAYERISVALID(player)) return;
    if (!shop || !cmd || !SSkills_ShopSessionValid(player, shop)) { SSkills_ShopClose(player); return; }
    sskillState_t *s = &p_playerInfo[player->s.number];
    int skills[NUM_SKILLS];
    int count = SSkills_ShopCatalogue(s->shopProfession, skills);
    qboolean up = cmd->forwardmove > 0, down = cmd->forwardmove < 0;
    qboolean select = (cmd->buttons & (BUTTON_USE | BUTTON_ATTACK)) != 0;
    qboolean back = (cmd->buttons & BUTTON_ALT_ATTACK) != 0;
    qboolean changed = qfalse;
    if ((up && player->client->Lmd.lmdMenu.stoppedPressingForward) ||
        (down && player->client->Lmd.lmdMenu.stoppedPressingBackward)) {
        if (s->shopConfirm) s->shopConfirmChoice = !s->shopConfirmChoice;
        else s->shopSelection = (s->shopSelection + (up ? count : 1)) % (count + 1);
        changed = qtrue;
    }
    if (back && player->client->Lmd.lmdMenu.stoppedPressingAltAttack) {
        if (s->shopConfirm) { s->shopConfirm = qfalse; changed = qtrue; }
        else { SSkills_ShopClose(player); return; }
    } else if (select && player->client->Lmd.lmdMenu.stoppedPressingUsing) {
        SSkills_ShopSelect(player);
        if (!s->shopEntity) return;
        changed = qtrue;
    }
    player->client->Lmd.lmdMenu.stoppedPressingForward = !up;
    player->client->Lmd.lmdMenu.stoppedPressingBackward = !down;
    player->client->Lmd.lmdMenu.stoppedPressingUsing = !select;
    player->client->Lmd.lmdMenu.stoppedPressingAltAttack = !back;
    if (changed || level.time >= player->client->Lmd.lmdMenu.nextUpdateTime) SSkills_ShopDraw(player);
}

static void SSkills_ShopUse(gentity_t *self, gentity_t *other, gentity_t *player)
{
    SSkills_ShopOpen(self, player);
}

static void SSkills_ShopInteract(gentity_t *self, gentity_t *player)
{
    if (!SSkills_ShopAccess(self, player)) return;
    char name[MAX_STRING_CHARS], action[32];
    trap_Argv(1, name, sizeof(name));
    trap_Argv(2, action, sizeof(action));
    if (!name[0]) { SSkills_ShopUse(self, player, player); return; }
    int index = SSkills_Find(name);
    if (index < 0) { Disp(player, "^3Unknown special skill."); return; }
    if (Q_stricmp(action, "buy")) {
        Disp(player, va("^5%s ^7(%d CR): %s\n^3Use /interact %s buy to purchase.",
            starSkills[index].name, SSkills_Price(index), starSkills[index].desc, starSkills[index].cmdName));
        return;
    }
    int price = SSkills_Price(index);
    if (price <= 0) { Disp(player, "^1The server's skill price configuration is invalid."); return; }
    SSkills_Buy(player, index);
}

const entityInfoData_t lmd_skillshop_keys[] = {
    {"#UKEYS", NULL}, {"#MODEL", NULL}, {"#HITBOX", NULL},
    {"radius", "Maximum distance for purchases, default 128 units."},
    {"model", "Defaults to models/map_objects/factory/f_con1.md3."},
    {"anim", "Menu animation: 0 console (default), 1 talk, 2 none."},
    {NULL, NULL}
};
entityInfo_t lmd_skillshop_info = {
    "Level 40 special skill vendor with an on-screen catalogue and purchase confirmation.",
    NULL, lmd_skillshop_keys
};
void lmd_skillshop(gentity_t *ent)
{
    extern void PlayerUsableGetKeys(gentity_t *);
    PlayerUsableGetKeys(ent);
    G_SpawnFloat("radius", "128", &ent->radius);
    G_SpawnInt("anim", "0", &ent->Lmd.customIndex);
    ent->radius = max(32, min(512, ent->radius));
    if (!ent->model || !ent->model[0]) ent->model = "models/map_objects/factory/f_con1.md3";
    SpawnEntModel(ent, qtrue, qfalse);
    if (ent->r.mins[0] > ent->r.maxs[0] || ent->r.mins[1] > ent->r.maxs[1] || ent->r.mins[2] > ent->r.maxs[2]) {
        VectorSet(ent->r.mins, -16, -16, 0);
        VectorSet(ent->r.maxs, 16, 16, 40);
        ent->r.contents = CONTENTS_SOLID;
    }
    ent->use = SSkills_ShopUse;
    ent->interact = SSkills_ShopInteract;
    ent->r.svFlags |= SVF_PLAYER_USABLE;
    ent->Lmd.crosshairText = "Special Skill Shop";
    ent->Lmd.crosshairTextRange = 256;
    trap_LinkEntity(ent);
}
