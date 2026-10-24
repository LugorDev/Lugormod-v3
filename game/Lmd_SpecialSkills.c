// Native port of UFO's KEBAB skills.cpp. Original source: KEBAB/skills.cpp.
// Binary address hooks have been replaced with the game's public functions.
#include "g_local.h"
#include "Lmd_Accounts_Core.h"
#include "Lmd_Accounts_CustomSkills.h"
#include "Lmd_EntityCore.h"
#include "Lmd_Interact.h"
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
	int killCount;
	int lastKill;
	int conKillCount;
	int gotoClientCd;
	int lastCmd;
	int muteTime;
	int regenSlowed;
	int fastRegen;
	int fastRegenCheckTime;
	int cloaked;
	int cloakCheckTime;
	int poisoned;
	int poisonCheckTime;
	int poisonedBy;
	int adrenaline;
	int flocked;
	int hyperPushClient;
	int severed;
	int chatspy;
	int repulse;
	int repulseCheckTime;
	int highFallDmg;
	int namelock;
	char namel[36];
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
	int starghost;
	int dstcd;
	int stasis;
	int follower;

	int noisedelay;
	int invulMedTimer;
	char lastmsg[128];
	char othermsg[128];
	int msgtime;
	int msgcount;
	int eball;

	int ddmg;
	int dtime;
	int blinded;
	int breach;
	int incinerate;
	int incineratetime;
	int sparktime;
	int cloakdmg;
	int fpcap;
	int battlemedfx;
	int totalcr;
	int stompstr;

	int norus;
	float custFireRate;
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


starSkill_t starSkills[NUM_SKILLS];

int Kebab_EnableSkills()
{
	return lmd_specialSkills.integer;
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
		if (PLAYERISVALID(targ) && !targ->client->ps.duelInProgress)
		{
			G_Knockdown(targ, 5000);
			G_Damage(targ, ent, ent, NULL, NULL, str >= 15 ? 99999 : str * 5, DAMAGE_NO_PROTECTION | DAMAGE_NO_ARMOR, MOD_FALLING);
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
	if (!ent || !ent->client || !ent->inuse || ent->s.number >= MAX_CLIENTS)
		return 0;
	return (ent->client->pers.Lmd.persistantFlags & 8	
		|| ent->client->pers.Lmd.persistantFlags & 16	
		|| ent->client->pers.Lmd.persistantFlags & 128	
		|| ent->client->pers.Lmd.persistantFlags & 256		
		|| p_playerInfo[ent->s.number].forceshield >= level.time);	
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
	if (tr.fraction < 1.0 && tr.entityNum < 32)
	{
		gentity_t* to = SSkills_Entity(tr.entityNum);
		if (!to || !to->client || to->client->ps.duelInProgress || to->client->invulnerableTimer > level.time)
			return 0;
		if (Accounts_Prof_GetProfession(to->client->pers.Lmd.account) == 4)
		{
			trap_SendServerCommand( ent->s.number, "print \"^1This skill cannot be used on mercenaries.\n\"");
			return 0;
		}
		if (Kebab_HasImmunity(to))
		{
			trap_SendServerCommand( ent->s.number, "print \"^1Target is immune to this sskill.\n\"");
			return 0;
		}
		if (Kebab_CanEvade(to))
		{
			trap_SendServerCommand( ent->s.number, "print \"^3Target has managed to evade your skill!\n\"");
			Kebab_EvadeAnimation(to);
			return 1;
		}
		NPC_SetAnim(ent, SETANIM_BOTH, 1335, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		
		int duration = Kebab_SkillDuration(to, &p_playerInfo[tr.entityNum].regenSlowed, 10000);
		for (int i = 0; i < NUM_FORCE_POWERS; i++)
			ForcePowerStop(to, i);
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
		if (Kebab_HasImmunity(to))
		{
			trap_SendServerCommand( ent->s.number, "print \"^1Target is immune to this sskill.\n\"");
			return 0;
		}
		if (Kebab_CanEvade(to))
		{
			trap_SendServerCommand( ent->s.number, "print \"^3Target has managed to evade your skill!\n\"");
			Kebab_EvadeAnimation(to);
			return 1;
		}
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
	if (fabs(ent->client->ps.velocity[0]) > 50.0f || fabs(ent->client->ps.velocity[1]) > 50.0f || fabs(ent->client->ps.velocity[2]) > 50.0f)
	{
		trap_SendServerCommand( ent->s.number, "print \"^1You have to stand still when using this skill.\n\"");
		return 0;
	}
	trace_t tr;
	gentity_t* to;
	SSkills_AimTrace(&tr, ent, 256, NULL, NULL);
	if (tr.fraction < 1.0 && tr.entityNum < 32)
	{
		to = SSkills_Entity(tr.entityNum);
		if (!to || !to->client || to->client->ps.duelInProgress || to->client->invulnerableTimer > level.time)
			return 0;
		if (Accounts_Prof_GetProfession(to->client->pers.Lmd.account) == 4)
		{
			trap_SendServerCommand( ent->s.number, "print \"^1This skill cannot be used on mercenaries.\n\"");
			return 0;
		}
		if (Kebab_HasImmunity(to))
		{
			trap_SendServerCommand( ent->s.number, "print \"^1Target is immune to this sskill.\n\"");
			return 0;
		}
		if (Kebab_CanEvade(to))
		{
			trap_SendServerCommand( ent->s.number, "print \"^3Target has managed to evade your skill!\n\"");
			Kebab_EvadeAnimation(to);
			return 1;
		}
		NPC_SetAnim(ent, SETANIM_BOTH, 1351, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		int duration = Kebab_SkillDuration(to, &p_playerInfo[tr.entityNum].severed, 10000);
		trap_SendServerCommand( ent->s.number, va("print \"^3The target has been severed from the force for %d seconds.\n\"", duration/1000));
		trap_SendServerCommand( tr.entityNum, va("print \"^4You have been severed from the force for %d seconds.\n\"", duration/1000));
		
		to->client->ps.electrifyTime = level.time + 10000;
		to->client->ps.weaponTime = 10;
		to->client->ps.quickerGetup = qtrue;
		to->client->ps.forceHandExtendTime = level.time + 10;
		for (int i = 0; i < NUM_FORCE_POWERS; i++)
			ForcePowerStop(to, i);
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
	if (!to || !to->client)
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
	vec3_t  angles, dir;
	VectorSet( angles, 0, ent->client->ps.viewangles[YAW], 0 );
	if (ent->client->pers.cmd.rightmove > 0)
	{
		AngleVectors( angles, NULL, dir, NULL );
	}
	if (ent->client->pers.cmd.rightmove < 0)
	{
		AngleVectors( angles, NULL, dir, NULL );
		VectorScale( dir, -1, dir );
	}
	if (ent->client->pers.cmd.forwardmove > 0)
	{
		AngleVectors( angles, dir, NULL, NULL );
	}
	if (ent->client->pers.cmd.forwardmove < 0)
	{
		AngleVectors( angles, dir, NULL, NULL );
		VectorScale( dir, -1, dir );
	}
	if (ent->client->pers.cmd.forwardmove == 0 && ent->client->pers.cmd.rightmove == 0)
	{
		if (ent->client->ps.saberMove <= 3)
			AngleVectors( angles, NULL, NULL, dir );
		else
			AngleVectors( angles, dir, NULL, NULL );
	}
	G_Throw(ent, dir, 256);
	ent->client->ps.velocity[2] += 192;
	if (ent->client->ps.saberMove <= 3)
		NPC_SetAnim(ent, SETANIM_BOTH, 1151, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	trap_SendServerCommand( ent->s.number, "print \"^2You have dashed using the force.\n\"");
	return 1;
}

int Kebab_ForceSlow(gentity_t* ent)
{
	if (fabs(ent->client->ps.velocity[0]) > 50.0f || fabs(ent->client->ps.velocity[1]) > 50.0f || fabs(ent->client->ps.velocity[2]) > 50.0f)
	{
		trap_SendServerCommand( ent->s.number, "print \"^1You have to stand still when using this skill.\n\"");
		return 0;
	}
	trace_t tr;
	gentity_t* to;
	SSkills_AimTrace(&tr, ent, 256, NULL, NULL);
	if (tr.fraction < 1.0 && tr.entityNum < 32)
	{
		to = SSkills_Entity(tr.entityNum);
		if (!to || !to->client || to->client->ps.duelInProgress || to->client->invulnerableTimer > level.time)
			return 0;
		if (Kebab_HasImmunity(to))
		{
			trap_SendServerCommand( ent->s.number, "print \"^1Target is immune to this sskill.\n\"");
			return 0;
		}
		if (Kebab_CanEvade(to))
		{
			trap_SendServerCommand( ent->s.number, "print \"^3Target has managed to evade your skill!\n\"");
			Kebab_EvadeAnimation(to);
			return 1;
		}
		ForcePowerStop(to, FP_SPEED);
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
	if (fabs(ent->client->ps.velocity[0]) > 50.0f || fabs(ent->client->ps.velocity[1]) > 50.0f || fabs(ent->client->ps.velocity[2]) > 50.0f)
	{
		trap_SendServerCommand( ent->s.number, "print \"^1You have to stand still when using this skill.\n\"");
		return 0;
	}
	trace_t tr;
	SSkills_AimTrace(&tr, ent, 1024, NULL, NULL);
	if (tr.fraction < 1.0 && tr.entityNum < 32)
	{
		gentity_t* to = SSkills_Entity(tr.entityNum);
		if (!to || !to->client || to->client->ps.duelInProgress || to->client->invulnerableTimer > level.time || p_playerInfo[tr.entityNum].poisoned > level.time)
			return 0;
		if (Kebab_HasImmunity(to))
		{
			trap_SendServerCommand( ent->s.number, "print \"^1Target is immune to this sskill.\n\"");
			return 0;
		}
		if (Kebab_CanEvade(to))
		{
			trap_SendServerCommand( ent->s.number, "print \"^3Target has managed to evade your skill!\n\"");
			Kebab_EvadeAnimation(to);
			return 1;
		}
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
		ent->health = 0;
		ent->client->ps.stats[STAT_HEALTH] = 0;
		player_die(ent, ent, ent, 99999, MOD_SUICIDE);
		return;
	}
	for ( int i = 0; i < MAX_CLIENTS; i++ )
	{
		gentity_t* other = SSkills_Entity(i);
		if (ent->s.number == i || !other || !other->inuse || other->client->sess.spectatorState || !ForcePowerUsableOn(ent, other, pull ? FP_PULL : FP_PUSH) || !CanDamage(other, ent->client->ps.origin))
			continue;
		float dist = Distance(ent->client->ps.origin, other->client->ps.origin);
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
		ent->health = 0;
		ent->client->ps.stats[STAT_HEALTH] = 0;
		player_die(ent, ent, ent, 99999, MOD_SUICIDE);
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
		ent->health = 0;
		ent->client->ps.stats[STAT_HEALTH] = 0;
		player_die(ent, ent, ent, 99999, MOD_SUICIDE);
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
	if (fabs(ent->client->ps.velocity[0]) > 50.0f || fabs(ent->client->ps.velocity[1]) > 50.0f || fabs(ent->client->ps.velocity[2]) > 50.0f)
	{
		trap_SendServerCommand( ent->s.number, "print \"^1You have to stand still when using this skill.\n\"");
		return 0;
	}
	trace_t tr;
	gentity_t* to;
	SSkills_AimTrace(&tr, ent, 256, NULL, NULL);
	if (tr.fraction < 1.0 && tr.entityNum < MAX_CLIENTS)
	{
		to = SSkills_Entity(tr.entityNum);
		if (!to || !to->client || to->client->ps.duelInProgress || to->client->invulnerableTimer > level.time || to->client->sess.spectatorState)
			return 0;
		if (Kebab_HasImmunity(to))
		{
			trap_SendServerCommand( ent->s.number, "print \"^1Target is immune to this sskill.\n\"");
			return 0;
		}
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
	
	ent->client->ps.powerups[5] = level.time + 30000;
	ent->client->ps.powerups[7] = level.time + 30000;
	ent->client->ps.powerups[12] = level.time + 30000;
	ent->client->ps.powerups[13] = level.time + 30000;
	
	
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
	missile->classname = "demp2_proj";
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
	if (fabs(ent->client->ps.velocity[0]) > 50.0f || fabs(ent->client->ps.velocity[1]) > 50.0f || fabs(ent->client->ps.velocity[2]) > 50.0f)
	{
		trap_SendServerCommand( ent->s.number, "print \"^1You have to stand still when using this skill.\n\"");
		return 0;
	}
	NPC_SetAnim(ent, SETANIM_BOTH, BOTH_VICTORY_FAST, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_RESTART | SETANIM_FLAG_HOLD);
	p_playerInfo[ent->s.number].wallcharge = level.time + 2000;
	ent->client->Lmd.flags |= 4;
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
		if (Kebab_HasImmunity(to))
		{
			trap_SendServerCommand( ent->s.number, "print \"^1Target is immune to this sskill.\n\"");
			return 0;
		}
		if (Kebab_CanEvade(to))
		{
			trap_SendServerCommand( ent->s.number, "print \"^3Target has managed to evade your skill!\n\"");
			Kebab_EvadeAnimation(to);
			return 1;
		}
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
		if (Kebab_HasImmunity(to))
		{
			trap_SendServerCommand( ent->s.number, "print \"^1Target is immune to this sskill.\n\"");
			return 0;
		}
		
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
	vec3_t  angles, dir;
	VectorSet( angles, 0, ent->client->ps.viewangles[YAW], 0 );
	if (ent->client->pers.cmd.rightmove > 0)
	{
		AngleVectors( angles, NULL, dir, NULL );
	}
	if (ent->client->pers.cmd.rightmove < 0)
	{
		AngleVectors( angles, NULL, dir, NULL );
		VectorScale( dir, -1, dir );
	}
	if (ent->client->pers.cmd.forwardmove > 0)
	{
		AngleVectors( angles, dir, NULL, NULL );
	}
	if (ent->client->pers.cmd.forwardmove < 0)
	{
		AngleVectors( angles, dir, NULL, NULL );
		VectorScale( dir, -1, dir );
	}
	if (ent->client->pers.cmd.forwardmove == 0 && ent->client->pers.cmd.rightmove == 0)
	{
		if (ent->client->ps.saberMove <= 3)
			AngleVectors( angles, NULL, NULL, dir );
		else
			AngleVectors( angles, dir, NULL, NULL );
	}
	G_Throw(ent, dir, 256);
	ent->client->ps.velocity[2] += 192;
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
		if (Kebab_HasImmunity(to))
		{
			trap_SendServerCommand( ent->s.number, "print \"^1Target is immune to this sskill.\n\"");
			return 0;
		}
		NPC_SetAnim(ent, SETANIM_BOTH, 1335, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		int duration = Kebab_SkillDuration(to, &p_playerInfo[tr.entityNum].breach, 5000);
		vec3_t entDir;
		VectorSubtract(to->client->ps.origin, ent->client->ps.origin, entDir);
		entDir[2] += 0.1f;
		VectorNormalize(entDir);
		G_Throw(to, entDir, 50);
		G_Knockdown(to, 100);
		p_playerInfo[tr.entityNum].adrenaline = 0;
		p_playerInfo[tr.entityNum].forceshield = 0;
		p_playerInfo[tr.entityNum].seeing = 0;
		p_playerInfo[tr.entityNum].fastRegen = 0;
		p_playerInfo[tr.entityNum].screamtimer = 0;
		Kebab_CloakStop(to);
		for (int i = 0; i < NUM_FORCE_POWERS; i++)
		{
			if (i < MAX_POWERUPS)
			{
				to->client->ps.powerups[i] = 0;
			}
			ForcePowerStop(to, i);
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
	if (fabs(ent->client->ps.velocity[0]) > 50.0f || fabs(ent->client->ps.velocity[1]) > 50.0f || fabs(ent->client->ps.velocity[2]) > 50.0f)
	{
		trap_SendServerCommand( ent->s.number, "print \"^1You have to stand still when using this skill.\n\"");
		return 0;
	}
	trace_t tr;
	gentity_t* to;
	SSkills_AimTrace(&tr, ent, 256, NULL, NULL);
	if (tr.fraction < 1.0 && tr.entityNum < MAX_CLIENTS)
	{
		to = SSkills_Entity(tr.entityNum);
		if (!to || !to->client || to->client->ps.duelInProgress || to->client->invulnerableTimer > level.time || to->client->sess.spectatorState)
			return 0;
		if (Kebab_HasImmunity(to))
		{
			trap_SendServerCommand( ent->s.number, "print \"^1Target is immune to this sskill.\n\"");
			return 0;
		}
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
		damage = 1000 / sqrt(Distance(ent->client->ps.origin, to->client->ps.origin));
		G_Damage(to, ent, ent, NULL, NULL, damage, DAMAGE_NO_PROTECTION, MOD_LAVA);
		G_PlayEffectID(G_EffectIndex("effects/explosions/droidexplosion1"), tr.endpos, ent->client->ps.viewangles);
	}
}

int Kebab_ForceCombustion(gentity_t* ent)
{
	if (fabs(ent->client->ps.velocity[0]) > 50.0f || fabs(ent->client->ps.velocity[1]) > 50.0f || fabs(ent->client->ps.velocity[2]) > 50.0f)
	{
		trap_SendServerCommand( ent->s.number, "print \"^1You have to stand still when using this skill.\n\"");
		return 0;
	}
	p_playerInfo[ent->s.number].sparktime = level.time + 10*1000; 
	for (int i = 0; i < NUM_FORCE_POWERS; i++)
		ForcePowerStop(ent, i);
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
	if (!acc || p_playerInfo[ent->s.number].breach + 5000 >= level.time || 
		((index == SKILL_BLASTARMOR && ent->client->ps.stats[STAT_ARMOR] < ent->client->ps.stats[STAT_MAX_HEALTH]) * 0.7) || 
		(ent->client->ps.duelInProgress && Accounts_Prof_GetProfession(ent->client->pers.Lmd.account) != MERC) || 
		(index == SKILL_LIGHTNINGSHIELD && ent->client->ps.stats[STAT_ARMOR] < 1))
	{
		return 0;
	}
	return Kebab_HasSkill(acc, index, qtrue);
}



void Kebab_InitSkills()
{
	
	starSkills[SKILL_BACTAS].cd = 40*1000;
	starSkills[SKILL_BACTAS].prof = MERC;
	starSkills[SKILL_BACTAS].function = Kebab_SummonBacta;
	starSkills[SKILL_BACTAS].pts = 1;
	starSkills[SKILL_BACTAS].fp = 0;
	starSkills[SKILL_BACTAS].name = "Summoning Bactas";
	starSkills[SKILL_BACTAS].cmdName = "summonbacta";
	starSkills[SKILL_BACTAS].desc = "The mercenary summons big and medium bacta cannisters that restore HP per usage.";
	starSkills[SKILL_BACTAS].isPassive = qfalse;

	starSkills[SKILL_FJ].cd = 20*1000;
	starSkills[SKILL_FJ].prof = MERC;
	starSkills[SKILL_FJ].pts = 1;
	starSkills[SKILL_FJ].fp = 20;
	starSkills[SKILL_FJ].function = Kebab_FuelJump;
	starSkills[SKILL_FJ].name = "Fuel Jump";
	starSkills[SKILL_FJ].cmdName = "fueljump";
	starSkills[SKILL_FJ].desc = "Releases extra flame from the jetpack, causing the user to dash in the selected direction.";
	starSkills[SKILL_FJ].isPassive = qfalse;

	starSkills[SKILL_ADRENALINE].cd = 60*1000;
	starSkills[SKILL_ADRENALINE].prof = MERC;
	starSkills[SKILL_ADRENALINE].pts = 2;
	starSkills[SKILL_ADRENALINE].fp = 50;
	starSkills[SKILL_ADRENALINE].function = Kebab_MercSprint;
	starSkills[SKILL_ADRENALINE].name = "Adrenaline Stimulation";
	starSkills[SKILL_ADRENALINE].cmdName = "adrenaline";
	starSkills[SKILL_ADRENALINE].desc = "The mercenary uses biofeedback techniques to unleash more adrenaline to his blood stream, increasing his overall agility for 10 seconds.";
	starSkills[SKILL_ADRENALINE].isPassive = qfalse;

	starSkills[SKILL_CF].prof = MERC;
	starSkills[SKILL_CF].pts = 2;
	starSkills[SKILL_CF].name = "Cortosis Fist";
	starSkills[SKILL_CF].cmdName = "cortosisfist";
	starSkills[SKILL_CF].desc = "When in melee, the user is given two small cortosis daggers which double the melee damage and turn off victim's lightsabers for 2 seconds.";
	starSkills[SKILL_CF].isPassive = qtrue;

	
	starSkills[SKILL_SLOWREGEN].cd = 80*1000;
	starSkills[SKILL_SLOWREGEN].prof = MERC;
	starSkills[SKILL_SLOWREGEN].function = Kebab_SlowRegen;
	starSkills[SKILL_SLOWREGEN].pts = 3;
	starSkills[SKILL_SLOWREGEN].fp = 0;
	starSkills[SKILL_SLOWREGEN].name = "Slow Regeneration";
	starSkills[SKILL_SLOWREGEN].cmdName = "slowregen";
	starSkills[SKILL_SLOWREGEN].desc = "The mercenary makes use of crystals that weaken one's connection to the force, slowing the regeneration of their enemy down for 10 seconds.";
	starSkills[SKILL_SLOWREGEN].isPassive = qfalse;

	
	starSkills[SKILL_FIREWALL].cd = 80*1000;
	starSkills[SKILL_FIREWALL].prof = MERC;
	starSkills[SKILL_FIREWALL].pts = 3;
	starSkills[SKILL_FIREWALL].fp = 80;
	starSkills[SKILL_FIREWALL].function = Kebab_FireWall;
	starSkills[SKILL_FIREWALL].name = "Fire Wall";
	starSkills[SKILL_FIREWALL].cmdName = "firewall";
	starSkills[SKILL_FIREWALL].desc = "The mercenary defuels his jetpack completely in order to conjure a deadly fire wave.";
	starSkills[SKILL_FIREWALL].isPassive = qfalse;

	
	starSkills[SKILL_DRUG].cd = 80*1000;
	starSkills[SKILL_DRUG].prof = MERC;
	starSkills[SKILL_DRUG].pts = 3;
	starSkills[SKILL_DRUG].fp = 20;
	starSkills[SKILL_DRUG].function = Kebab_Drug;
	starSkills[SKILL_DRUG].name = "Kebab_Drug Injection";
	starSkills[SKILL_DRUG].cmdName = "drug";
	starSkills[SKILL_DRUG].desc = "Drugs the target for 10 seconds, making him more vulnerable to any kind of damage.";
	starSkills[SKILL_DRUG].isPassive = qfalse;

	
	starSkills[SKILL_AM].cd = 30*1000;
	starSkills[SKILL_AM].prof = MERC;
	starSkills[SKILL_AM].pts = 3;
	starSkills[SKILL_AM].fp = 0;
	starSkills[SKILL_AM].function = Kebab_Wallhack;
	starSkills[SKILL_AM].name = "Seeing";
	starSkills[SKILL_AM].cmdName = "seeing";
	starSkills[SKILL_AM].desc = "Mercenary uses a device, which allows him to see through walls for a short period of time.";
	starSkills[SKILL_AM].isPassive = qfalse;

	
	starSkills[SKILL_LIGHTNINGSHIELD].prof = MERC;
	starSkills[SKILL_LIGHTNINGSHIELD].pts = 3;
	starSkills[SKILL_LIGHTNINGSHIELD].name = "Lightning Shield";
	starSkills[SKILL_LIGHTNINGSHIELD].cmdName = "lshield";
	starSkills[SKILL_LIGHTNINGSHIELD].desc = "The user is able to redirect some of lightsaber damage at the attacker, as long as he has any shield left.";
	starSkills[SKILL_LIGHTNINGSHIELD].isPassive = qtrue;

	
	starSkills[SKILL_DEMO].prof = MERC;
	starSkills[SKILL_DEMO].pts = 4;
	starSkills[SKILL_DEMO].name = "Demolition";
	starSkills[SKILL_DEMO].cmdName = "demolition";
	starSkills[SKILL_DEMO].desc = "Extensively educates the user in the art of crafting explosives, thereby increasing the range and damage of their thermal detonators, detonation packs and trip mines. Using any of aforementioned weapons will now consume fuel.";
	starSkills[SKILL_DEMO].isPassive = qtrue;

	
	starSkills[SKILL_POISON].cd = 40*1000;
	starSkills[SKILL_POISON].prof = MERC;
	starSkills[SKILL_POISON].pts = 4;
	starSkills[SKILL_POISON].fp = 20;
	starSkills[SKILL_POISON].function = Kebab_PoisonDart;
	starSkills[SKILL_POISON].name = "Poison Dart";
	starSkills[SKILL_POISON].cmdName = "poisondart";
	starSkills[SKILL_POISON].desc = "The mercenary makes use of a highly toxic dart to shoot the enemy, dealing damage over the course of 10 seconds.";
	starSkills[SKILL_POISON].isPassive = qfalse;

	
	starSkills[SKILL_WPPROF].prof = MERC;
	starSkills[SKILL_WPPROF].pts = 4;
	starSkills[SKILL_WPPROF].name = "Weapon Proficiency";
	starSkills[SKILL_WPPROF].cmdName = "weaponprof";
	starSkills[SKILL_WPPROF].desc = "The mercenary shall spawn with every advanced weaponry existing in the Universe.";
	starSkills[SKILL_WPPROF].isPassive = qtrue;

	
	starSkills[SKILL_IMPLANTREGEN].prof = MERC;
	starSkills[SKILL_IMPLANTREGEN].pts = 4;
	starSkills[SKILL_IMPLANTREGEN].name = "Implant Regeneration";
	starSkills[SKILL_IMPLANTREGEN].cmdName = "implant";
	starSkills[SKILL_IMPLANTREGEN].desc = "Through special implants, user is able to regenerate his health, armor and ammo by meditating with weapon melee equipped.";
	starSkills[SKILL_IMPLANTREGEN].isPassive = qtrue;

	
	starSkills[SKILL_POUR].cd = 100*1000;
	starSkills[SKILL_POUR].prof = MERC;
	starSkills[SKILL_POUR].pts = 4;
	starSkills[SKILL_POUR].fp = 50;
	starSkills[SKILL_POUR].function = Kebab_PourFuel;
	starSkills[SKILL_POUR].name = "Pour Fuel";
	starSkills[SKILL_POUR].cmdName = "pourfuel";
	starSkills[SKILL_POUR].desc = "User pours some of his fuel on a selected target, making him vulnerable to any fire attack.";
	starSkills[SKILL_POUR].isPassive = qfalse;

	
	starSkills[SKILL_TRAINING].prof = MERC;
	starSkills[SKILL_TRAINING].pts = 5;
	starSkills[SKILL_TRAINING].name = "Mobility";
	starSkills[SKILL_TRAINING].cmdName = "mobility";
	starSkills[SKILL_TRAINING].desc = "Thanks to the special training, user is able to reach Jedi-like speed and perform various acrobations. Increases the chance of evading skills.";
	starSkills[SKILL_TRAINING].isPassive = qtrue;

	
	starSkills[SKILL_TIMEBOMB].cd = 120*1000;
	starSkills[SKILL_TIMEBOMB].prof = MERC;
	starSkills[SKILL_TIMEBOMB].function = Kebab_TimeBomb;
	starSkills[SKILL_TIMEBOMB].pts = 5;
	starSkills[SKILL_TIMEBOMB].fp = 50;
	starSkills[SKILL_TIMEBOMB].name = "Time Bomb";
	starSkills[SKILL_TIMEBOMB].cmdName = "timebomb";
	starSkills[SKILL_TIMEBOMB].desc = "Using extensive electronic knowledge, the mercenary builds a devastating time bomb that goes off in 3 seconds.";
	starSkills[SKILL_TIMEBOMB].isPassive = qfalse;

	
	starSkills[SKILL_GRAPPLECHOKE].prof = MERC;
	starSkills[SKILL_GRAPPLECHOKE].pts = 5;
	starSkills[SKILL_GRAPPLECHOKE].name = "Grapple Choke";
	starSkills[SKILL_GRAPPLECHOKE].cmdName = "grapple";
	starSkills[SKILL_GRAPPLECHOKE].desc = "Mercenary is able to choke his opponents using Grapple Hook.";
	starSkills[SKILL_GRAPPLECHOKE].isPassive = qtrue;

	
	starSkills[SKILL_FORCERESIST].prof = MERC;
	starSkills[SKILL_FORCERESIST].pts = 5;
	starSkills[SKILL_FORCERESIST].name = "Force Resistance";
	starSkills[SKILL_FORCERESIST].cmdName = "forceresist";
	starSkills[SKILL_FORCERESIST].desc = "Increases mercenary's resilience to knockdowns caused by forces Push and Pull. Also bestows full immunity to Mind Trick and Lightning";
	starSkills[SKILL_FORCERESIST].isPassive = qtrue;

	starSkills[SKILL_CS].prof = MERC;
	starSkills[SKILL_CS].pts = 6;
	starSkills[SKILL_CS].name = "Critical Hit";
	starSkills[SKILL_CS].cmdName = "critical";
	starSkills[SKILL_CS].desc = "Installs a pure rylith power cell in all user's weapons, increasing the direct hit damage by 10-15 points and giving a random chance of knocking down the victim.";
	starSkills[SKILL_CS].isPassive = qtrue;

	starSkills[SKILL_BLASTARMOR].prof = MERC;
	starSkills[SKILL_BLASTARMOR].pts = 6;
	starSkills[SKILL_BLASTARMOR].name = "Blast Armor";
	starSkills[SKILL_BLASTARMOR].cmdName = "blastarmor";
	starSkills[SKILL_BLASTARMOR].desc = "Reduces splash damage dealt to the user and makes lightsaber hits bounce off, preventing the enemy saberist from chaining his attacks. Works with at least 70 percent of shield.";
	starSkills[SKILL_BLASTARMOR].isPassive = qtrue;

	

	
	starSkills[SKILL_CLOAK].cd = 40*1000;
	starSkills[SKILL_CLOAK].prof = JEDI_SITH;
	starSkills[SKILL_CLOAK].function = Kebab_CloakSkill;
	starSkills[SKILL_CLOAK].pts = 1;
	starSkills[SKILL_CLOAK].fp = 0;
	starSkills[SKILL_CLOAK].name = "Force Cloak";
	starSkills[SKILL_CLOAK].cmdName = "forcecloak";
	starSkills[SKILL_CLOAK].desc = "With Force Power consumption, a Force user can alter the path of photons such that he becomes invisible unless he reveals himself.";
	starSkills[SKILL_CLOAK].isPassive = qfalse;

	starSkills[SKILL_DASH].cd = 20*1000;
	starSkills[SKILL_DASH].prof = JEDI_SITH;
	starSkills[SKILL_DASH].function = Kebab_ForceDash;
	starSkills[SKILL_DASH].pts = 1;
	starSkills[SKILL_DASH].fp = 20;
	starSkills[SKILL_DASH].name = "Force Dash";
	starSkills[SKILL_DASH].cmdName = "forcedash";
	starSkills[SKILL_DASH].desc = "The force user uses the force to dash with great speed in the selected direction.";
	starSkills[SKILL_DASH].isPassive = qfalse;

	starSkills[SKILL_DESTRUCTION].cd = 40*1000;
	starSkills[SKILL_DESTRUCTION].prof = JEDI_SITH;
	starSkills[SKILL_DESTRUCTION].function = Kebab_ForceDestruction;
	starSkills[SKILL_DESTRUCTION].pts = 2;
	starSkills[SKILL_DESTRUCTION].fp = 50;
	starSkills[SKILL_DESTRUCTION].name = "Force Destruction";
	starSkills[SKILL_DESTRUCTION].cmdName = "destruction";
	starSkills[SKILL_DESTRUCTION].desc = "Creates and launches an energy projectile made out of Force.";
	starSkills[SKILL_DESTRUCTION].isPassive = qfalse;

	starSkills[SKILL_BATTLEMED].cd = 120*1000;
	starSkills[SKILL_BATTLEMED].prof = JEDI_SITH;
	starSkills[SKILL_BATTLEMED].function = Kebab_BattleMeditation;
	starSkills[SKILL_BATTLEMED].pts = 2;
	starSkills[SKILL_BATTLEMED].fp = 100;
	starSkills[SKILL_BATTLEMED].name = "Battle Meditation";
	starSkills[SKILL_BATTLEMED].cmdName = "battlemed";
	starSkills[SKILL_BATTLEMED].desc = "Heals user's buddies, improves their defense and gives them Force Boon or Ysalimari, depending on their profession.";
	starSkills[SKILL_BATTLEMED].isPassive = qfalse;

	
	starSkills[SKILL_DISABLEJP].cd = 60*1000;
	starSkills[SKILL_DISABLEJP].prof = JEDI_SITH;
	starSkills[SKILL_DISABLEJP].function = Kebab_DisableJP;
	starSkills[SKILL_DISABLEJP].name = "Jetpack Disabling";
	starSkills[SKILL_DISABLEJP].pts = 3;
	starSkills[SKILL_DISABLEJP].fp = 50;
	starSkills[SKILL_DISABLEJP].cmdName = "disablejp";
	starSkills[SKILL_DISABLEJP].desc = "Electronically scrambles the target's jetpack through the force, disabling it for 15 seconds.";
	starSkills[SKILL_DISABLEJP].isPassive = qfalse;

	
	starSkills[SKILL_SLOW].cd = 60*1000;
	starSkills[SKILL_SLOW].prof = JEDI_SITH;
	starSkills[SKILL_SLOW].pts = 3;
	starSkills[SKILL_SLOW].fp = 50;
	starSkills[SKILL_SLOW].function = Kebab_ForceSlow;
	starSkills[SKILL_SLOW].name = "Force Slow";
	starSkills[SKILL_SLOW].cmdName = "forceslow";
	starSkills[SKILL_SLOW].desc = "The force user uses telekinesis to slow down an enemy for 15 seconds, sacrificing his own Force energy in the process.";
	starSkills[SKILL_SLOW].isPassive = qfalse;

	
	starSkills[SKILL_SCREAM].cd = 100*1000;
	starSkills[SKILL_SCREAM].prof = JEDI_SITH;
	starSkills[SKILL_SCREAM].function = Kebab_ForceScream;
	starSkills[SKILL_SCREAM].pts = 3;
	starSkills[SKILL_SCREAM].fp = 50;
	starSkills[SKILL_SCREAM].name = "Force Scream";
	starSkills[SKILL_SCREAM].cmdName = "forcescream";
	starSkills[SKILL_SCREAM].desc = "Conjures a powerful shockwave, which slows down and lightly damages enemies nearby.";
	starSkills[SKILL_SCREAM].isPassive = qfalse;

	
	starSkills[SKILL_SHIELD].cd = 120*1000;
	starSkills[SKILL_SHIELD].prof = JEDI_SITH;
	starSkills[SKILL_SHIELD].function = Kebab_ForceShield;
	starSkills[SKILL_SHIELD].pts = 3;
	starSkills[SKILL_SHIELD].fp = 80;
	starSkills[SKILL_SHIELD].name = "Force Shield";
	starSkills[SKILL_SHIELD].cmdName = "forceshield";
	starSkills[SKILL_SHIELD].desc = "Creates a powerful shield made out of Force for 30 seconds, which is capable of deflecting light weapons, gives temporary immunity to Force Powers , sSkills, and improves their vitality. Reduces damage dealt by the user and weakens his Force Powers.";
	starSkills[SKILL_SHIELD].isPassive = qfalse;

	
	starSkills[SKILL_FORCEBODY].cd = 120*1000;
	starSkills[SKILL_FORCEBODY].prof = JEDI_SITH;
	starSkills[SKILL_FORCEBODY].function = Kebab_FastRegen;
	starSkills[SKILL_FORCEBODY].pts = 3;
	starSkills[SKILL_FORCEBODY].fp = 0;
	starSkills[SKILL_FORCEBODY].name = "Force Body";
	starSkills[SKILL_FORCEBODY].cmdName = "forcebody";
	starSkills[SKILL_FORCEBODY].desc = "The force user enacts a ritual using his own blood, quadrupling his rate of regeneration for 30 seconds but also dealing damage to him.";
	starSkills[SKILL_FORCEBODY].isPassive = qfalse;

	
	starSkills[SKILL_DODGE].prof = JEDI_SITH;
	starSkills[SKILL_DODGE].pts = 4;
	starSkills[SKILL_DODGE].name = "Force Reflex";
	starSkills[SKILL_DODGE].cmdName = "forcereflex";
	starSkills[SKILL_DODGE].desc = "Improves user's reflex and allows dodging Disruptor Rifle with 75 percent success rate. Increases the chance of evading skills.";
	starSkills[SKILL_DODGE].isPassive = qtrue;

	
	starSkills[SKILL_BREACH].cd = 100*1000;
	starSkills[SKILL_BREACH].prof = JEDI_SITH;
	starSkills[SKILL_BREACH].function = Kebab_ForceBreach;
	starSkills[SKILL_BREACH].pts = 4;
	starSkills[SKILL_BREACH].fp = 50;
	starSkills[SKILL_BREACH].name = "Force Breach";
	starSkills[SKILL_BREACH].cmdName = "forcebreach";
	starSkills[SKILL_BREACH].desc = "Turns the target into a wound in the Force, temporarily removing all buffing spells, including High Resistance and Blast Armor. This skill cannot be evaded.";
	starSkills[SKILL_BREACH].isPassive = qfalse;

	
	starSkills[SKILL_SEVER].cd = 80*1000;
	starSkills[SKILL_SEVER].prof = JEDI_SITH;
	starSkills[SKILL_SEVER].function = Kebab_ForceSever;
	starSkills[SKILL_SEVER].pts = 4;
	starSkills[SKILL_SEVER].fp = 80;
	starSkills[SKILL_SEVER].name = "Force Sever";
	starSkills[SKILL_SEVER].cmdName = "forcesever";
	starSkills[SKILL_SEVER].desc = "Strips the target from the force for 10 seconds.";
	starSkills[SKILL_SEVER].isPassive = qfalse;

	
	starSkills[SKILL_ENERGY].cd = 60*1000;
	starSkills[SKILL_ENERGY].prof = JEDI_SITH;
	starSkills[SKILL_ENERGY].function = Kebab_EnergyBall;
	starSkills[SKILL_ENERGY].pts = 4;
	starSkills[SKILL_ENERGY].fp = 50;
	starSkills[SKILL_ENERGY].name = "Energy Ball";
	starSkills[SKILL_ENERGY].cmdName = "energyball";
	starSkills[SKILL_ENERGY].desc = "Conjures a powerful Force Ball made out of concentrated energy which knocks down target on impact.";
	starSkills[SKILL_ENERGY].isPassive = qfalse;

	
	starSkills[SKILL_FORCESTOMP].prof = JEDI_SITH;
	starSkills[SKILL_FORCESTOMP].pts = 4;
	starSkills[SKILL_FORCESTOMP].name = "Force Stomping";
	starSkills[SKILL_FORCESTOMP].cmdName = "forcestomp";
	starSkills[SKILL_FORCESTOMP].desc = "The force user gains ability to knockdown and inflict damage to opponents by crouched stomping on their heads.";
	starSkills[SKILL_FORCESTOMP].isPassive = qtrue;

	
	starSkills[SKILL_REPULSE].cd = 80*1000;
	starSkills[SKILL_REPULSE].prof = JEDI_SITH;
	starSkills[SKILL_REPULSE].function = Kebab_RepulseBegin;
	starSkills[SKILL_REPULSE].pts = 5;
	starSkills[SKILL_REPULSE].fp = 50;
	starSkills[SKILL_REPULSE].name = "Force Kebab_Repulse";
	starSkills[SKILL_REPULSE].cmdName = "repulse";
	starSkills[SKILL_REPULSE].desc = "Concentrates energy to create a sudden burst that throws surrounding enemies into great distance.";
	starSkills[SKILL_REPULSE].isPassive = qfalse;

	
	starSkills[SKILL_INSANITY].cd = 100*1000;
	starSkills[SKILL_INSANITY].prof = JEDI_SITH;
	starSkills[SKILL_INSANITY].function = Kebab_ForceInsanity;
	starSkills[SKILL_INSANITY].pts = 5;
	starSkills[SKILL_INSANITY].fp = 80;
	starSkills[SKILL_INSANITY].name = "Force Insanity";
	starSkills[SKILL_INSANITY].cmdName = "insanity";
	starSkills[SKILL_INSANITY].desc = "Drives the target insane for 10 seconds. This skill cannot be evaded.";
	starSkills[SKILL_INSANITY].isPassive = qfalse;

	
	starSkills[SKILL_INCINERATE].cd = 120*1000;
	starSkills[SKILL_INCINERATE].prof = JEDI_SITH;
	starSkills[SKILL_INCINERATE].function = Kebab_Incinerate;
	starSkills[SKILL_INCINERATE].pts = 5;
	starSkills[SKILL_INCINERATE].fp = 70;
	starSkills[SKILL_INCINERATE].name = "Force Incineration";
	starSkills[SKILL_INCINERATE].cmdName = "incinerate";
	starSkills[SKILL_INCINERATE].desc = "Sets the target on fire through the Force. If the target fails to extinguish it in two seconds, they die.";
	starSkills[SKILL_INCINERATE].isPassive = qfalse;

	
	starSkills[SKILL_SPARKSPAM].cd = 120*1000;
	starSkills[SKILL_SPARKSPAM].prof = JEDI_SITH;
	starSkills[SKILL_SPARKSPAM].function = Kebab_ForceCombustion;
	starSkills[SKILL_SPARKSPAM].pts = 5;
	starSkills[SKILL_SPARKSPAM].fp = 70;
	starSkills[SKILL_SPARKSPAM].name = "Force Combustion";
	starSkills[SKILL_SPARKSPAM].cmdName = "combustion";
	starSkills[SKILL_SPARKSPAM].desc = "Heavily damages the target.";
	starSkills[SKILL_SPARKSPAM].isPassive = qfalse;

	starSkills[SKILL_HR].prof = JEDI_SITH;
	starSkills[SKILL_HR].pts = 6;
	starSkills[SKILL_HR].name = "High Resistance";
	starSkills[SKILL_HR].cmdName = "highresistance";
	starSkills[SKILL_HR].desc = "Provides high resistance to injury, improves the user's vitality and reduces the duration of the sskills casted on the user by 50 percent.";
	starSkills[SKILL_HR].isPassive = qtrue;

	starSkills[SKILL_FIRESWORD].prof = JEDI_SITH;
	starSkills[SKILL_FIRESWORD].pts = 6;
	starSkills[SKILL_FIRESWORD].name = "Fire Magic";
	starSkills[SKILL_FIRESWORD].cmdName = "firemagic";
	starSkills[SKILL_FIRESWORD].desc = "Thanks to an ancient gem, the user is granted knowledge about Fire Magic, making him immune to any fire attacks. Increases damage inflicted with a lightsaber by 10-15 points (doesn't apply to saber throw).";
	starSkills[SKILL_FIRESWORD].isPassive = qtrue;

	trap_Printf( "Skills initialized.\n");
}

static int Kebab_HasSkill(Account_t *acc, int index, qboolean check)
{
    if (!acc || index < 0 || index >= NUM_SKILLS || !lmd_specialSkills.integer)
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

qboolean SSkills_Buy(gentity_t *ent, int index, int price)
{
    if (!PLAYERISVALID(ent) || !lmd_specialSkills.integer || index < 0 || index >= NUM_SKILLS || price <= 0)
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
    if (!PLAYERISVALID(ent) || !lmd_specialSkills.integer) return;
    Account_t *acc = ent->client->pers.Lmd.account;
    char name[MAX_STRING_CHARS];
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
    sskillState_t *state = &p_playerInfo[ent->s.number];
    int cooldowns[NUM_SKILLS];
    memcpy(cooldowns, state->cds, sizeof(cooldowns));
    if (state->cloaked) Kebab_CloakStop(ent);
    if (state->forceshield) ent->flags &= ~FL_SHIELDED;
    if (state->fastRegen && state->fpcap > 0)
        ent->client->ps.fd.forcePowerMax = state->fpcap;
    memset(state, 0, sizeof(*state));
    if (!disconnect) memcpy(state->cds, cooldowns, sizeof(cooldowns));
    state->poisonedBy = state->fueledby = state->incinerate = -1;
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
