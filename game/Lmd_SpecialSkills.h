#ifndef LMD_SPECIAL_SKILLS_H
#define LMD_SPECIAL_SKILLS_H
enum skillIds_s
{
	SKILL_FORCEBODY = 0,
	SKILL_DESTRUCTION,
	SKILL_CLOAK,
	SKILL_DASH,
	SKILL_DISABLEJP,
	SKILL_SEVER,
	SKILL_SLOW,
	SKILL_FORCESTOMP,
	SKILL_REPULSE,
	SKILL_SLOWREGEN,
	SKILL_TIMEBOMB,
	SKILL_POISON,
	SKILL_ADRENALINE,
	SKILL_BACTAS,
	SKILL_FORCERESIST,
	SKILL_WPPROF,
	SKILL_DODGE,
	SKILL_BATTLEMED,
	SKILL_SCREAM,
	SKILL_INSANITY,
	SKILL_SHIELD,
	SKILL_ENERGY,
	SKILL_BLASTARMOR,
	SKILL_IMPLANTREGEN,
	SKILL_FIREWALL,
	SKILL_TRAINING,
	SKILL_POUR,
	SKILL_DRUG,
	SKILL_FJ,
	SKILL_AM,
	SKILL_HR,
	SKILL_INCINERATE,
	SKILL_BREACH,
	//SKILL_FORCEBLIND,
	SKILL_CF,
	SKILL_DEMO,
	SKILL_CS,
	SKILL_FIRESWORD,
	SKILL_SPARKSPAM,
	SKILL_LIGHTNINGSHIELD,
	SKILL_GRAPPLECHOKE,
	//SKILL_TECH,
	//SKILL_PI,
	NUM_SKILLS,
	LAST_SKILL = SKILL_HR,
};
void SSkills_Init(void);
void SSkills_Reset(gentity_t *ent, qboolean disconnect);
void SSkills_Think(gentity_t *ent);
void SSkills_Spawn(gentity_t *ent);
void SSkills_Command(gentity_t *ent, int iArg);
qboolean SSkills_Has(gentity_t *ent, int index);
qboolean SSkills_Buy(gentity_t *ent, int index);
void SSkills_Demolition(gentity_t *attacker, float *damage, float *radius, int mod);
int SSkills_Damage(gentity_t *target, gentity_t *attacker, gentity_t *inflictor, int damage, int flags, int mod);
qboolean SSkills_ForceAllowed(gentity_t *attacker, gentity_t *target, int power);
int SSkills_Regen(gentity_t *ent, int amount);
void lmd_skillshop(gentity_t *ent);

#endif
