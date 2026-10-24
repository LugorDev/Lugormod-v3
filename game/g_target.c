// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"

#include "Lmd_Entities_Public.h"
#include "Lmd_Accounts_Core.h"

//==========================================================

/*QUAKED target_give (1 0 0) (-8 -8 -8) (8 8 8)
Gives the activator all the items pointed to.
*/
const entityInfoData_t target_give_spawnflags[] = {
    // {"", ""},
    {NULL, NULL}
};
const entityInfoData_t target_give_keys[] = {
    {"target", "the item to give a player"},
    {"targetname", "make the trigger target this value for the entity to be used"},
    {NULL, NULL}
};
const entityInfo_t target_give_info = {
    "Gives the activator all the items pointed to.",
    target_give_spawnflags,
    target_give_keys
};

void Use_Target_Give(gentity_t* ent, gentity_t* other, gentity_t* activator)
{
    gentity_t* t;
    trace_t trace;

    if (!activator->client)
    {
        return;
    }

    if (!ent->target)
    {
        return;
    }

    memset(&trace, 0, sizeof(trace));
    t = NULL;
    while ((t = G_Find(t, FOFS(targetname), ent->target)) != NULL)
    {
        if (!t->item)
        {
            continue;
        }
        Touch_Item(t, activator, &trace);

        // make sure it isn't going to respawn or show any events
        t->nextthink = 0;
        trap_UnlinkEntity(t);
    }
}

void SP_target_give(gentity_t* ent)
{
    ent->use = Use_Target_Give;
}


//==========================================================

/*QUAKED target_remove_powerups (1 0 0) (-8 -8 -8) (8 8 8)
takes away all the activators powerups.
Used to drop flight powerups into death puts.
*/
const entityInfoData_t target_remove_powerups_spawnflags[] = {
    // {"", ""},
    {NULL, NULL}
};
const entityInfoData_t target_remove_powerups_keys[] = {
    {"targetname", "make the trigger target this value for the entity to be used"},
    {NULL, NULL}
};
const entityInfo_t target_remove_powerups_info = {
    "Gives the activator all the items pointed to.",
    target_remove_powerups_spawnflags,
    target_remove_powerups_keys
};

void Use_target_remove_powerups(gentity_t* ent, gentity_t* other, gentity_t* activator)
{
    if (!activator->client)
    {
        return;
    }

    if (activator->client->ps.powerups[PW_REDFLAG])
    {
        Team_ReturnFlag(TEAM_RED);
    }
    else if (activator->client->ps.powerups[PW_BLUEFLAG])
    {
        Team_ReturnFlag(TEAM_BLUE);
    }
    else if (activator->client->ps.powerups[PW_NEUTRALFLAG])
    {
        Team_ReturnFlag(TEAM_FREE);
    }

    //RoboPhred
    activator->client->pushEffectTime = 0;

    memset(activator->client->ps.powerups, 0, sizeof(activator->client->ps.powerups));
}

void SP_target_remove_powerups(gentity_t* ent)
{
    ent->use = Use_target_remove_powerups;
}

//==========================================================

const entityInfoData_t target_powerup_spawnflags[] = {
    {NULL, NULL}
};
const entityInfoData_t target_powerup_keys[] = {
    {"powerup", "powerup to give to the player (number 1-15)"},
    {"wait", "duration in seconds"},
    {"targetname", "make the trigger target this value for the entity to be used"},
    {NULL, NULL}
};
const entityInfo_t target_powerup_info = {
    "Sets a powerup to the player.",
    target_powerup_spawnflags,
    target_powerup_keys
};

void Use_target_powerup(gentity_t* ent, gentity_t* other, gentity_t* activator)
{
    if (!activator->client)
    {
        return;
    }
    //RoboPhred
    if (ent->genericValue10 == PW_PULL)
    {
        activator->client->pushEffectTime = level.time + ent->wait * 1000;
    }
    else
        activator->client->ps.powerups[ent->genericValue10] = level.time + ent->wait * 1000;
}

void SP_target_powerup(gentity_t* ent)
{
    int pw;

    G_SpawnInt("powerup", "1", &pw);
    if (pw < 0 || pw > PW_NUM_POWERUPS)
    {
        EntitySpawnError("Invalid powerup.");
        G_Free(ent);
        return;
    }
    ent->genericValue10 = pw;
    ent->use = Use_target_powerup;
    if (!ent->wait)
    {
        ent->wait = 30;
    }
}


//==========================================================

/*QUAKED target_delay (1 0 0) (-8 -8 -8) (8 8 8) NO_RETRIGGER

NO_RETRIGGER - Keeps the delay from resetting the time if it is
activated again while it is counting down to an event.

"wait" seconds to pause before firing targets.
"random" delay variance, total delay = delay +/- random seconds
*/
const entityInfoData_t target_delay_spawnflags[] = {
    {"1", "Keeps the delay from resetting the time if it is activated again while it is counting down to an event."},
    {NULL, NULL}
};
const entityInfoData_t target_delay_keys[] = {
    {"wait", "seconds to pause before firing targets"},
    {"random", "delay variance, total delay = delay +/- random seconds"},
    {"targetname", "make the trigger target this value for the entity to be used"},
    {NULL, NULL}
};
const entityInfo_t target_delay_info = {
    "This is a great entity for making an entity not fire its target right away, instead it waits the amount of time you set with the \'wait\' key to fire its target.",
    target_delay_spawnflags,
    target_delay_keys
};

//Ufo: multithread option
void Think_Target_Delay(gentity_t* ent)
{
    if (ent->spawnflags & 2)
    {
        int* ptr = &ent->health;
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (ptr[i] == 0)
                continue;
            if (ptr[i] <= level.time)
            {
                ptr[i] = 0;
                G_UseTargets2(ent, GetEnt(i), ent->target);
                continue;
            }
            if (ent->nextthink == 0 || ptr[i] < ent->nextthink)
                ent->nextthink = ptr[i];
        }
    }
    else
        G_UseTargets(ent, ent->activator);
}

void Use_Target_Delay(gentity_t* ent, gentity_t* other, gentity_t* activator)
{
    if (ent->spawnflags & 2 && activator->s.number < MAX_CLIENTS)
    {
        int* ptr = &ent->health + activator->s.number;
        if (*ptr && ent->spawnflags & 1)
            return;
        *ptr = level.time + (ent->wait + ent->random * crandom()) * 1000;
        ent->think = Think_Target_Delay;
        if (ent->nextthink == 0 || *ptr < ent->nextthink)
            ent->nextthink = *ptr;
        return;
    }
    else if (ent->nextthink > level.time && (ent->spawnflags & 1))
    {
        //Leave me alone, I am thinking.
        return;
    }
    G_ActivateBehavior(ent, BSET_USE);
    ent->nextthink = level.time + (ent->wait + ent->random * crandom()) * 1000;
    ent->think = Think_Target_Delay;
    ent->activator = activator;
}

void SP_target_delay(gentity_t* ent)
{
    // check delay for backwards compatability
    if (!G_SpawnFloat("delay", "0", &ent->wait))
    {
        G_SpawnFloat("wait", "1", &ent->wait);
    }

    if (!ent->wait)
    {
        ent->wait = 1;
    }
    ent->use = Use_Target_Delay;
}


//==========================================================

/*QUAKED target_score (1 0 0) (-8 -8 -8) (8 8 8)
"count" number of points to add, default 1

The activator is given this many points.
*/
const entityInfoData_t target_score_spawnflags[] = {
    {NULL, NULL}
};
const entityInfoData_t target_score_keys[] = {
    {"count", "number of points to add, default 1"},
    {"targetname", "make the trigger target this value for the entity to be used"},
    {NULL, NULL}
};
const entityInfo_t target_score_info = {
    "The activator is given this many points.",
    target_score_spawnflags,
    target_score_keys
};

void Use_Target_Score(gentity_t* ent, gentity_t* other, gentity_t* activator)
{
    //RoboPhred
    if (ent->spawnflags & 1)
        AddScore(activator, ent->r.currentOrigin, ent->count - activator->client->ps.persistant[PERS_SCORE]);
    else
        AddScore(activator, ent->r.currentOrigin, ent->count);
}

void SP_target_score(gentity_t* ent)
{
    if (!ent->count)
    {
        ent->count = 1;
    }
    ent->use = Use_Target_Score;
}


// lumaya: target_weapons

typedef struct
{
    const char* name;
    int id;
} targetWeaponsName_t;

targetWeaponsName_t targetWeaponsTable[] = {
    {"none", WP_NONE},
    {"stun_baton", WP_STUN_BATON},
    {"melee", WP_MELEE},
    {"single", WP_SABER},
    {"duals", WP_SABER},
    {"staff", WP_SABER},
    {"bryar_pistol", WP_BRYAR_PISTOL},
    {"blaster", WP_BLASTER},
    {"disruptor", WP_DISRUPTOR},
    {"bowcaster", WP_BOWCASTER},
    {"repeater", WP_REPEATER},
    {"demp2", WP_DEMP2},
    {"flechette", WP_FLECHETTE},
    {"rocket_launcher", WP_ROCKET_LAUNCHER},
    {"thermal", WP_THERMAL},
    {"tripmine", WP_TRIP_MINE},
    {"detpack", WP_DET_PACK},
    {"concussion", WP_CONCUSSION},
    //	{ "bryar_old",       WP_BRYAR_OLD },
    //	{ "emplaced_gun",    WP_EMPLACED_GUN },
    //	{ "turret",          WP_TURRET },
    {NULL, WP_NUM_WEAPONS}
};


const entityInfoData_t target_weapons_spawnflags[] = {
    {"1", "Don't lock the saber, meaning the given saber can be switched from duals to staff for instance."},
    {"2", "Just allow the weapons but don't give them."},
    {"4", "Add the ammo instead of setting it."},
    {NULL, NULL}
};
const entityInfoData_t target_weapons_keys[] = {
    {
        "weapons",
        "Given Weapons. Example: weapons,detpack5.concussion5000.single - this would give the activator detpack with 5 ammo concussion with 5000 ammo. if no number is given then its infinite. staff|duals|single take no ammo. if 0 is given then it removes the weapon\n"
    },
    {"targetname", "make the trigger target this value for the entity to be used"},
    {NULL, NULL}
};
const entityInfo_t target_weapons_info = {
    "The activator is given or allowed these weapons: stun_baton, melee, single | duals | staff, bryar_pistol, blaster, disruptor, bowcaster, repeater, demp2, flechette, rocket_launcher, thermal"
    ",tripmine, detpack, concussion.",
    target_score_spawnflags,
    target_score_keys
};

static int TargetWeapons_FindID(const char* name)
{
    for (int i = 0; targetWeaponsTable[i].name; i++)
    {
        if (!Q_stricmp(targetWeaponsTable[i].name, name))
        {
            return targetWeaponsTable[i].id;
        }
    }
    return -1;
}

extern qboolean G_SetSaber(gentity_t* ent, int saberNum, char* saberName, qboolean siegeOverride);
extern qboolean G_SaberModelSetup(gentity_t* ent);
extern qboolean WP_SaberStyleValidForSaber(saberInfo_t* saber1, saberInfo_t* saber2, int saberHolstered,
                                           int saberAnimLevel);
extern qboolean WP_UseFirstValidSaberStyle(saberInfo_t* saber1, saberInfo_t* saber2, int saberHolstered,
                                           int* saberAnimLevel);

static void TargetWeapons_ParseAndGive(gentity_t* activator, const char* input, qboolean forceGive, qboolean justAllow, qboolean addAmmo)
{
    char buffer[1024];
    Q_strncpyz(buffer, input, sizeof(buffer));

    int forcedWeapon = -1;

    char* token = strtok(buffer, ".");
    while (token)
    {
        char weaponName[64];
        int ammoAmount = 0;

        // Extract number from token if present (e.g. "detpack5")
        int len = 0;
        while (token[len] && !isdigit(token[len])) len++;

        Q_strncpyz(weaponName, token, len + 1);
        if (isdigit(token[len]))
        {
            ammoAmount = atoi(&token[len]);
        }
        else
        {
            ammoAmount = SHRT_MAX;
        }

        int weaponID = TargetWeapons_FindID(weaponName);
        if (weaponID != -1)
        {
            if (ammoAmount == 0)
            {
                if (justAllow)
                {
                    activator->client->Lmd.canPickUpWeapons &= ~(1 << weaponID);
                    activator->client->ps.stats[STAT_WEAPONS] &= ~(1 << weaponID);
                }
                else
                {
                    activator->client->ps.stats[STAT_WEAPONS] &= ~(1 << weaponID);
                }
            }
            else
            {
                if (justAllow)
                    activator->client->Lmd.canPickUpWeapons |= (1 << weaponID);
                else
                    activator->client->ps.stats[STAT_WEAPONS] |= (1 << weaponID);
            }

            if (weaponID != WP_SABER && weaponID != WP_BRYAR_PISTOL)
            {
                activator->client->ps.ammo[weaponData[weaponID].ammoIndex] = addAmmo ? activator->client->ps.ammo[weaponData[weaponID].ammoIndex] + ammoAmount : ammoAmount;
                if (activator->client->ps.ammo[weaponData[weaponID].ammoIndex] > SHRT_MAX)
                {
                    activator->client->ps.ammo[weaponData[weaponID].ammoIndex] = SHRT_MAX;
                }
            }

            if (weaponID == WP_SABER && !justAllow)
            {
                activator->client->Lmd.lockSaber = forceGive;
                char userinfo[MAX_INFO_STRING];
                const char* saber1;
                const char* saber2;

                // Choose saber models based on type
                if (!Q_stricmp(weaponName, "duals"))
                {
                    saber1 = "single_2";
                    saber2 = "single_2";
                }
                else if (!Q_stricmp(weaponName, "staff"))
                {
                    saber1 = "dual_2";
                    saber2 = "none";
                }
                else // single
                {
                    saber1 = "single_2";
                    saber2 = "none";
                }

                trap_GetUserinfo(activator->s.number, userinfo, sizeof(userinfo));
                Info_SetValueForKey(userinfo, "saber1", saber1);
                G_SetSaber(activator, 0, saber1, qfalse);
                Info_SetValueForKey(userinfo, "saber2", saber2);
                G_SetSaber(activator, 1, saber2, qfalse);
                trap_SetUserinfo(activator->s.number, userinfo);
                ClientUserinfoChanged(activator->s.number);

                G_SaberModelSetup(activator);

                // update saber anim levels
                if (activator->client->saber[0].model[0] && activator->client->saber[1].model[0]) // dual
                {
                    activator->client->ps.fd.saberAnimLevelBase =
                        activator->client->ps.fd.saberAnimLevel =
                        activator->client->ps.fd.saberDrawAnimLevel = SS_DUAL;
                }
                else if (activator->client->saber[0].saberFlags & SFL_TWO_HANDED) // staff
                {
                    activator->client->ps.fd.saberAnimLevel =
                        activator->client->ps.fd.saberDrawAnimLevel = SS_STAFF;
                }
                else // single
                {
                    if (activator->client->sess.saberLevel < SS_FAST)
                    {
                        activator->client->sess.saberLevel = SS_FAST;
                    }
                    else if (activator->client->sess.saberLevel > SS_STRONG)
                    {
                        activator->client->sess.saberLevel = SS_STRONG;
                    }
                    activator->client->ps.fd.saberAnimLevelBase =
                        activator->client->ps.fd.saberAnimLevel =
                        activator->client->ps.fd.saberDrawAnimLevel =
                        activator->client->sess.saberLevel;

                    if (activator->client->ps.fd.saberAnimLevel >
                        activator->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE])
                    {
                        activator->client->ps.fd.saberAnimLevelBase =
                            activator->client->ps.fd.saberAnimLevel =
                            activator->client->ps.fd.saberDrawAnimLevel =
                            activator->client->sess.saberLevel =
                            activator->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE];
                    }
                }

                if (!WP_SaberStyleValidForSaber(&activator->client->saber[0], &activator->client->saber[1],
                                                2, activator->client->ps.fd.saberAnimLevel))
                {
                    WP_UseFirstValidSaberStyle(&activator->client->saber[0], &activator->client->saber[1],
                                               2,
                                               &activator->client->ps.fd.saberAnimLevel);

                    activator->client->ps.fd.saberAnimLevelBase =
                        activator->client->saberCycleQueue =
                        activator->client->ps.fd.saberAnimLevel;
                }
            }

            if (forceGive && !justAllow && ammoAmount != 0)
            {
                forcedWeapon = weaponID;
            }
        }

        token = strtok(NULL, ".");
    }

    if (forcedWeapon != -1)
    {
        activator->client->ps.weapon = forcedWeapon;
        activator->client->ps.weaponstate = WEAPON_READY;
        activator->client->ps.weaponTime = 0;
        activator->s.weapon = forcedWeapon;
    }
}

void Use_Target_Weapons(gentity_t* ent, gentity_t* other, gentity_t* activator)
{
    if (!activator || !activator->client)
        return;

    if (ent->target2 && ent->target2[0])
    {
        TargetWeapons_ParseAndGive(activator, ent->target2, !(ent->spawnflags & 1), ent->spawnflags & 2, ent->spawnflags & 4);
    }
}

void SP_target_weapons(gentity_t* ent)
{
    G_SpawnString("weapons", "", &ent->target2);
    if (!ent->target2 || !ent->target2[0])
    {
        EntitySpawnError("Invalid weapons key-value.");
        G_FreeEntity(ent);
        return;
    }

    ent->use = Use_Target_Weapons;
}


//==========================================================

/*QUAKED target_print (1 0 0) (-8 -8 -8) (8 8 8) redteam blueteam private
"message"	text to print
"wait"		don't fire off again if triggered within this many milliseconds ago
If "private", only the activator gets the message.  If no checks, all clients get the message.
*/

const entityInfoData_t target_print_spawnflags[] = {
    {"1", "Red team only"},
    {"2", "Blue team only"},
    {"4", "Only the person who activated the entity can see the message"},
    {"8", "Message will appear at top left corner of the screen"},
    {"16", "Message will appear in area where chat normally appears"},
    {NULL, NULL}
};
const entityInfoData_t target_print_keys[] = {
    {"message", "text to print"},
    {"wait", "don\'t fire off again if triggered within this many milliseconds ago"},
    {"targetname", "make the trigger target this value for the entity to be used"},
    {NULL, NULL}
};
const entityInfo_t target_print_info = {
    "This will print a message across the screen. Basically the same as the announce command. If no spawnflag is set, it will make the print global.",
    target_print_spawnflags,
    target_print_keys
};

//RoboPhred
//Ufo: Added data tags support

char* GetPasswordByIndex(const char* index)
{
    if (!index || !index[0])
        return NULL;
    gentity_t* ent = NULL;
    while (ent = IterateEnts(ent))
    {
        if (Q_stricmp(ent->fullName, index) != 0)
            continue;
        if (Q_stricmp(ent->classname, "lmd_pwterminal") != 0)
            continue;
        return ent->target3;
    }
    return "";
}

extern char* lmd_processMessagePlaceholders(gentity_t* entity, char* message, char* target2);
char* Accounts_Custom_GetValue(Account_t* acc, char* key);

void Send_Target_Print(gentity_t* ent, int targ)
{
    char buf[MAX_STRING_CHARS];
    strncpy(buf, ent->message, MAX_STRING_CHARS);

    gentity_t* activator = ent->activator;
    if (activator && activator->m_pVehicle && activator->m_pVehicle->m_pPilot)
    {
        activator = (gentity_t*)activator->m_pVehicle->m_pPilot;
    }

    char* processedMsg = lmd_processMessagePlaceholders(activator, buf, ent->target2);
    strncpy(buf, processedMsg, MAX_STRING_CHARS);

    if (buf[0] == '@' && buf[1] != '@')
    {
        trap_SendServerCommand(targ, va("cps \"%s\"", buf));
    }
    else if (ent->spawnflags & 8)
    {
        trap_SendServerCommand(targ, va("print \"%s\n\"", buf));
    }
    else if (ent->spawnflags & 16)
    {
        trap_SendServerCommand(targ, va("chat \"%s\"", buf));
    }
    else
    {
        trap_SendServerCommand(targ, va("cp \"%s\n\"", buf));
    }
}

void Use_Target_Print_Go(gentity_t* ent)
{
    //RoboPhred
    int i;

    if (!ent || !ent->inuse)
    {
        Com_Printf("ERROR: Bad ent in Use_Target_Print");
        return;
    }

    if (ent->wait)
    {
        if (ent->genericValue14 >= level.time)
        {
            return;
        }
        ent->genericValue14 = level.time + ent->wait;
    }

    /*
    //RoboPhred
    if(ent->message[0] == '@' && ent->message[1] != '@')
        cmd = "cps";
    else if(ent->spawnflags & 8)
        cmd = "print";
    else if(ent->spawnflags & 16)
        cmd = "chat";
    else
        cmd = "cp";
    */
    G_ActivateBehavior(ent, BSET_USE);

    if (ent->spawnflags & 4)
        Send_Target_Print(ent, ent->activator->s.number);
    else if (ent->spawnflags & 3)
    {
        for (i = 0; i < level.maxclients; i++)
        {
            if (level.clients[i].pers.connected != CON_CONNECTED)
                continue;
            if (ent->spawnflags & 1 && level.clients[i].sess.sessionTeam != TEAM_RED)
                continue;
            if (ent->spawnflags & 2 && level.clients[i].sess.sessionTeam != TEAM_BLUE)
                continue;
            Send_Target_Print(ent, i);
        }
    }
    else
        Send_Target_Print(ent, -1);
    /*
    if ( ( ent->spawnflags & 4 ) ) 
    {//private, to one client only
        if (!activator || !activator->inuse)
        {
            Com_Printf("ERROR: Bad activator in Use_Target_Print");
        }
        if ( activator && activator->client )
        {//make sure there's a valid client ent to send it to
            trap_SendServerCommand( activator->s.number, va("%s \"%s\"", cmd, ent->message ));
        }
        //NOTE: change in functionality - if there *is* no valid client ent, it won't send it to anyone at all
        return;
    }

    if ( ent->spawnflags & 3 ) {
        if ( ent->spawnflags & 1 ) {
            G_TeamCommand( TEAM_RED, va("%s \"%s\"", cmd, ent->message) );
        }
        if ( ent->spawnflags & 2 ) {
            G_TeamCommand( TEAM_BLUE, va("%s \"%s\"", cmd, ent->message) );
            if(isChat)
                target_print_sendechos(activator, ent->message);
        }
        return;
    }
    trap_SendServerCommand( -1, va("%s \"%s\"", cmd, ent->message ));
    */
    G_UseTargets(ent, ent->activator);
}

//Ufo: redesigned a bit to make "delay" working
void Think_Target_Print(gentity_t* ent)
{
    Use_Target_Print_Go(ent);
}

void Use_Target_Print(gentity_t* ent, gentity_t* other, gentity_t* activator)
{
    ent->activator = activator;
    if (ent->delay <= 0)
    {
        Use_Target_Print_Go(ent);
    }
    else
    {
        ent->think = Think_Target_Print;
        //Ufo:
        ent->nextthink = level.time + ent->delay;
    }
}

void SP_target_print(gentity_t* ent)
{
    //RoboPhred
    if (!ent->message || !ent->message[0])
    {
        G_FreeEntity(ent);
        return;
    }
    ent->use = Use_Target_Print;
    //RoboPhred: force a delay
    if (ent->wait <= 0)
        ent->wait = 700;
    //Ufo:
    if (!ent->target2 || !ent->target2[0])
        G_SpawnString("arg", "", &ent->target2);
}

const entityInfoData_t target_fp_spawnflags[] = {
    {"1", "Modify only if FP is known"},
    {NULL, NULL}
};

const entityInfoData_t target_fp_keys[] = {
	{"targetname", "make the trigger target this value for the entity to be used"},
	{"ResetPowers", "Resets the users original force powers. 0-1 (Default 0)"},
	{"ModifyPowers", "E.g. ModifyPowers,heal2.rage2 -> would set force heal and rage to level 2. Available keys are: jump, push, pull, speed, seeing, heal, protect, absorb"
				  "mindtrick, theal, grip, lightning, rage, drain, tforce, sattack, sdefend, sthrow."},
	{"ForceRegenSpeedMultiplier", "Multiply the existing force regen speed by the given amount. E.g: 1.25. Must be greater than 0."},
	{NULL, NULL}
};

const entityInfo_t target_fp_info = {
	"The activator is given the forcepower + levels given in the ModifyPowers key. ForcePowers not given in ModifyPowers key will remain unchanged. Only death can reset this.",
	target_fp_spawnflags,
	target_fp_keys
};

typedef struct {
	const char *name;
	int id;
} forcePowerMap_t;

static const forcePowerMap_t forcePowerMap[] = {
	{"jump", FP_LEVITATION},
	{"push", FP_PUSH},
	{"pull", FP_PULL},
	{"speed", FP_SPEED},
	{"seeing", FP_SEE},
    
	{"heal", FP_HEAL},
	{"protect", FP_PROTECT},
	{"absorb", FP_ABSORB},
	{"mindtrick", FP_TELEPATHY},
	{"theal", FP_TEAM_HEAL},
    
	{"grip", FP_GRIP},
	{"lightning", FP_LIGHTNING},
	{"rage", FP_RAGE},
	{"drain", FP_DRAIN},
	{"tforce", FP_TEAM_FORCE},
    
	{"sattack", FP_SABER_OFFENSE},
	{"sdefend", FP_SABER_DEFENSE},
	{"sthrow", FP_SABERTHROW},
	{NULL, -1}
};

int lmd_get_forcePowerMapIndex(const char *token) {
	for (int i = 0; forcePowerMap[i].name; i++) {
		if (!Q_stricmp(forcePowerMap[i].name, token)) {
			return forcePowerMap[i].id;
		}
	}
	return -1;
}
qboolean PlayerUseableCheck(gentity_t *self, gentity_t *activator);
void Use_Target_Fp (gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	if (!activator || !activator->client)
		return;
		
	if (PlayerUseableCheck(ent, activator) == qfalse)
		return;

    if (!PlayerUseableCheck(ent, activator))
        return;

	activator->client->Lmd.customForceRegenSpeedMultiplier = ent->modelScale[0];

	if (ent->Lmd.customIndex == 1)
	{
		activator->client->Lmd.targetFpUsed = qfalse;
		WP_InitForcePowers(activator);
		return;
	}

	activator->client->Lmd.targetFpUsed = qtrue;
	
	char *token = strtok(ent->target2, ".");
	while (token)
	{
		char powerName[32];
		int level = -1;
            
		if (sscanf(token, "%31[a-zA-Z]%d", powerName, &level) == 2)
		{
			int fpIndex = lmd_get_forcePowerMapIndex(powerName);
			if (fpIndex >= 0 && level >= 0 && level <= FORCE_LEVEL_5)
			{
			    if (ent->spawnflags & 1 && !(activator->client->ps.fd.forcePowersKnown & (1 << fpIndex)))
			    {
			        //modify only if fp is known
			    }
                else
                {
				    activator->client->ps.fd.forcePowerLevel[fpIndex] = level;
				    if (level > 0)
					    activator->client->ps.fd.forcePowersKnown |= (1 << fpIndex);
				    else
					    activator->client->ps.fd.forcePowersKnown &= ~(1 << fpIndex);
                }
			}
		}

		token = strtok(NULL, ".");
	}
}

extern void PlayerUsableGetKeys(gentity_t *ent);

void SP_target_fp( gentity_t *ent )
{
    PlayerUsableGetKeys(ent);
	G_SpawnString("ModifyPowers", "", &ent->target2);
	G_SpawnFloat("ForceRegenSpeedMultiplier", "0.0", &ent->modelScale[0]);
	G_SpawnInt("ResetPowers", "0", &ent->Lmd.customIndex);
	
	ent->use = Use_Target_Fp;
}




//==========================================================


/*QUAKED target_speaker (1 0 0) (-8 -8 -8) (8 8 8) looped-on looped-off global activator
"noise"		wav file to play

A global sound will play full volume throughout the level.
Activator sounds will play on the player that activated the target.
Global and activator sounds can't be combined with looping.
Normal sounds play each time the target is used.
Looped sounds will be toggled by use functions.
Multiple identical looping sounds will just increase volume without any speed cost.
"wait" : Seconds between auto triggerings, 0 = don't auto trigger
"random"	wait variance, default is 0
*/

const entityInfoData_t target_speaker_spawnflags[] = {
    {"1", "Makes the sound start on, loops the sound when complete"},
    {"2", "Makes the sound start off"},
    {"4", "Everybody on the entire map can hear the sound"},
    {"8", "Only the activator can hear the sound"},
    {NULL, NULL}
};
const entityInfoData_t target_speaker_keys[] = {
    {"noise", "wav file to play. ex: sound/ambience/narshaddaa/cantina_1.mp3"},
    {"wait", "seconds between auto triggerings, 0 = don't auto trigger"},
    {"random", "wait variance, default is 0"},
    {"targetname", "make the trigger target this value for the entity to be used"},
    {NULL, NULL}
};
const entityInfo_t target_speaker_info = {
    "This entity will play a sound that you specify in a certain radius.",
    target_speaker_spawnflags,
    target_speaker_keys
};

void Use_Target_Speaker(gentity_t* ent, gentity_t* other, gentity_t* activator)
{
    G_ActivateBehavior(ent, BSET_USE);

    if (ent->spawnflags & 3)
    {
        // looping sound toggles
        if (ent->s.loopSound)
        {
            ent->s.loopSound = 0; // turn it off
            ent->s.loopIsSoundset = qfalse;
            ent->s.trickedentindex = 1;
        }
        else
        {
            ent->s.loopSound = ent->noise_index; // start it
            ent->s.loopIsSoundset = qfalse;
            ent->s.trickedentindex = 0;
        }
    }
    else
    {
        // normal sound
        if (ent->spawnflags & 8)
        {
            G_AddEvent(activator, EV_GENERAL_SOUND, ent->noise_index);
        }
        else if (ent->spawnflags & 4)
        {
            G_AddEvent(ent, EV_GLOBAL_SOUND, ent->noise_index);
        }
        else
        {
            G_AddEvent(ent, EV_GENERAL_SOUND, ent->noise_index);
        }
    }
}

//RoboPhred
qboolean target_speaker_allowlogical()
{
    int spawnflags;
    G_SpawnInt("spawnflags", "0", &spawnflags);
    if (spawnflags & 8 && !(spawnflags & 1))
        return qtrue;
    return qfalse;
}

void SP_target_speaker(gentity_t* ent)
{
    char buffer[MAX_QPATH];
    char* s = NULL;

    G_SpawnFloat("wait", "0", &ent->wait);
    G_SpawnFloat("random", "0", &ent->random);

    if (G_SpawnString("soundSet", "", &s))
    {
        // this is a sound set
        ent->s.soundSetIndex = G_SoundSetIndex(s);
        ent->s.eFlags = EF_PERMANENT;
        VectorCopy(ent->s.origin, ent->s.pos.trBase);
        trap_LinkEntity(ent);
        return;
    }

    if (!G_SpawnString("noise", "NOSOUND", &s))
    {
        //RoboPhred: just delete outselvs
        Com_Printf("target_speaker without a noise key at %s", vtos(ent->s.origin));
        G_FreeEntity(ent);
        return;
    }

    // force all client reletive sounds to be "activator" speakers that
    // play on the entity that activates it
    if (s[0] == '*')
    {
        ent->spawnflags |= 8;
    }

    Q_strncpyz(buffer, s, sizeof(buffer));

    ent->noise_index = G_SoundIndex(buffer);

    // a repeating speaker can be done completely client side
    ent->s.eType = ET_SPEAKER;
    ent->s.eventParm = ent->noise_index;
    ent->s.frame = ent->wait * 10;
    ent->s.clientNum = ent->random * 10;


    // check for prestarted looping sound
    if (ent->spawnflags & 1)
    {
        ent->s.loopSound = ent->noise_index;
        ent->s.loopIsSoundset = qfalse;
    }

    ent->use = Use_Target_Speaker;

    if (ent->spawnflags & 4)
    {
        ent->r.svFlags |= SVF_BROADCAST;
    }

    VectorCopy(ent->s.origin, ent->s.pos.trBase);

    // must link the entity so we get areas and clusters so
    // the server can determine who to send updates to
    trap_LinkEntity(ent);
}


//==========================================================

/*QUAKED target_laser (0 .5 .8) (-8 -8 -8) (8 8 8) START_ON
When triggered, fires a laser.  You can either set a target or a direction.
*/
const entityInfoData_t target_laser_spawnflags[] = {
    {NULL, NULL}
};
const entityInfoData_t target_laser_keys[] = {
    {"target", "make this target a target_position. you don't need to do this you can just set angles"},
    {"targetname", "make the trigger target this value for the entity to be used"},
    {NULL, NULL}
};
const entityInfo_t target_laser_info = {
    "When triggered, fires a laser. You can either set a target or a direction. Starts in the off state so you have to use its targetname for it to turn on",
    target_laser_spawnflags,
    target_laser_keys
};

void target_laser_think(gentity_t* self)
{
    vec3_t end;
    trace_t tr;
    vec3_t point;

    // if pointed at another entity, set movedir to point at it
    if (self->enemy)
    {
        VectorMA(self->enemy->s.origin, 0.5, self->enemy->r.mins, point);
        VectorMA(point, 0.5, self->enemy->r.maxs, point);
        VectorSubtract(point, self->s.origin, self->movedir);
        VectorNormalize(self->movedir);
    }

    // fire forward and see what we hit
    VectorMA(self->s.origin, 2048, self->movedir, end);

    trap_Trace(&tr, self->s.origin, NULL, NULL, end, self->s.number, CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE);

    if (tr.entityNum)
    {
        // hurt it if we can
        G_Damage(&g_entities[tr.entityNum], self, self->activator, self->movedir,
                 tr.endpos, self->damage, DAMAGE_NO_KNOCKBACK, MOD_TARGET_LASER);
    }

    VectorCopy(tr.endpos, self->s.origin2);

    trap_LinkEntity(self);
    self->nextthink = level.time + FRAMETIME;
}

void target_laser_on(gentity_t* self)
{
    if (!self->activator)
        self->activator = self;
    target_laser_think(self);
}

void target_laser_off(gentity_t* self)
{
    trap_UnlinkEntity(self);
    self->nextthink = 0;
}

void target_laser_use(gentity_t* self, gentity_t* other, gentity_t* activator)
{
    self->activator = activator;
    if (self->nextthink > 0)
        target_laser_off(self);
    else
        target_laser_on(self);
}

void target_laser_start(gentity_t* self)
{
    gentity_t* ent;

    self->s.eType = ET_BEAM;

    if (self->target)
    {
        ent = G_Find(NULL, FOFS(targetname), self->target);
        if (!ent)
        {
            G_Printf("%s at %s: %s is a bad target\n", self->classname, vtos(self->s.origin), self->target);
        }
        self->enemy = ent;
    }
    else
    {
        G_SetMovedir(self->s.angles, self->movedir);
    }

    self->use = target_laser_use;
    self->think = target_laser_think;

    if (!self->damage)
    {
        self->damage = 1;
    }

    if (self->spawnflags & 1)
        target_laser_on(self);
    else
        target_laser_off(self);
}

void SP_target_laser(gentity_t* self)
{
    // let everything else get spawned before we start firing
    self->think = target_laser_start;
    self->nextthink = level.time + FRAMETIME;
}


//==========================================================

void target_teleporter_use(gentity_t* self, gentity_t* other, gentity_t* activator)
{
    gentity_t* dest;

    if (!activator || !activator->client || (activator->s.eType != ET_PLAYER && activator->s.eType != ET_NPC))
    {
        return;
    }

    G_ActivateBehavior(self, BSET_USE);

    //RoboPhred
    if (!(self->spawnflags & 1) && duelInProgress(&activator->client->ps))
        return;

    //RoboPhred
    if (!self->target || !self->target[0])
        dest = self;
    else
    {
        dest = G_PickTarget(self->target);
        if (!dest)
        {
            G_Printf("Couldn't find teleporter destination\n");
            return;
        }
    }

    //RoboPhred: spawnflag 2 to restrict effect
    TeleportPlayer(activator, dest->s.origin, dest->s.angles, self->spawnflags & 2);
}

/*QUAKED target_teleporter (1 0 0) (-8 -8 -8) (8 8 8)
The activator will be teleported away.
*/
const entityInfoData_t target_teleporter_spawnflags[] = {
    {"1", "Do not teleport player if they are in a duel."},
    {NULL, NULL}
};
const entityInfoData_t target_teleporter_keys[] = {
    {"targetname", "make the trigger target this value for the entity to be used"},
    {NULL, NULL}
};
const entityInfo_t target_teleporter_info = {
    "The activator will be teleported away... Use angle or angles keys to change the direction that you teleport in.",
    target_teleporter_spawnflags,
    target_teleporter_keys
};

void SP_target_teleporter(gentity_t* self)
{
    //RoboPhred
    /*Their target is themselvs if neccessary.
    if (!self->targetname)
        G_Printf("untargeted %s at %s\n", self->classname, vtos(self->s.origin));
    */
    self->use = target_teleporter_use;
}

//==========================================================


/*QUAKED target_relay (.5 .5 .5) (-8 -8 -8) (8 8 8) RED_ONLY BLUE_ONLY RANDOM x x x x INACTIVE
This doesn't perform any actions except fire its targets.
The activator can be forced to be from a certain team.
if RANDOM is checked, only one of the targets will be fired, not all of them

INACTIVE  Can't be used until activated

wait - set to -1 to use it only once
*/

const entityInfoData_t target_relay_spawnflags[] = {
    {"1", "only red team can use this"},
    {"", "only blue team can use this"},
    {"4", "makes the target get fired off randomly, currently this only works for target1"},
    {"128", "makes the entity start deactivated"},
    {NULL, NULL}
};
const entityInfoData_t target_relay_keys[] = {
    {"wait", "set to -1 to use it only once"},
    {"target", "the first target to fire"},
    {"target2", "the second target to fire"},
    {"target3", "the third target to fire"},
    {"target4", "the fourth target to fire"},
    {"target5", "the fifth target to fire"},
    {"target6", "the sixth target to fire"},
    {"targetname", "make the trigger target this value for the entity to be used"},
    {NULL, NULL}
};
const entityInfo_t target_relay_info = {
    "Does nothing but fire at its targets, can fire at up to 6 targets it will fire them all at the same time. If random checked, only one will be fired.",
    target_relay_spawnflags,
    target_relay_keys
};

void target_relay_use(gentity_t* self, gentity_t* other, gentity_t* activator)
{
    qboolean ranscript = qfalse;
    if ((self->spawnflags & 1) && activator->client
        && activator->client->sess.sessionTeam != TEAM_RED)
    {
        return;
    }
    if ((self->spawnflags & 2) && activator->client
        && activator->client->sess.sessionTeam != TEAM_BLUE)
    {
        return;
    }

    if (self->flags & FL_INACTIVE)
    {
        //set by target_deactivate
        return;
    }

    ranscript = G_ActivateBehavior(self, BSET_USE);
    if (self->wait == -1)
    {
        //never use again
        if (ranscript)
        {
            //crap, can't remove!
            self->use = NULL;
        }
        else
        {
            //remove
            //RoboPhred: always stay I guess
            //if (!self->spawnString) {//Lugormod
            //	self->think = G_FreeEntity;
            //	self->nextthink = level.time + FRAMETIME;
            //} else {
            self->use = NULL;
            //}
        }
    }
    if (self->spawnflags & 4)
    {
        gentity_t* ent;

        ent = G_PickTarget(self->target);
        if (ent && ent->use)
        {
            GlobalUse(ent, self, activator);
        }
        return;
    }
    G_UseTargets(self, activator);


    //RoboPhred
    if (self->target2)
        G_UseTargets2(self, activator, self->target2);
    if (self->target3)
        G_UseTargets2(self, activator, self->target3);
    if (self->target4)
        G_UseTargets2(self, activator, self->target4);
    if (self->target5)
        G_UseTargets2(self, activator, self->target5);
    if (self->target6)
        G_UseTargets2(self, activator, self->target6);
}

void SP_target_relay(gentity_t* self)
{
    self->use = target_relay_use;
    if (self->spawnflags & 128)
    {
        self->flags |= FL_INACTIVE;
    }
}


//==========================================================

/*QUAKED target_kill (.5 .5 .5) (-8 -8 -8) (8 8 8)
Kills the activator.
*/

const entityInfoData_t target_kill_spawnflags[] = {
    {NULL, NULL}
};
const entityInfoData_t target_kill_keys[] = {
    {"target", "target a player to kill them"},
    {"targetname", "make the trigger target this value for the entity to be used"},
    {NULL, NULL}
};
const entityInfo_t target_kill_info = {
    "Kills the activator.",
    target_kill_spawnflags,
    target_kill_keys
};

void target_kill_use(gentity_t* self, gentity_t* other, gentity_t* activator)
{
    G_ActivateBehavior(self, BSET_USE);
    G_Damage(activator, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);
}

void SP_target_kill(gentity_t* self)
{
    self->use = target_kill_use;
}

/*QUAKED target_position (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for in-game calculation, like jumppad targets.
*/
const entityInfoData_t target_position_spawnflags[] = {
    {NULL, NULL}
};
const entityInfoData_t target_position_keys[] = {
    {"targetname", "make the trigger target this value for the entity to be used"},
    {NULL, NULL}
};
const entityInfo_t target_position_info = {
    "Used as a positional target for in-game calculation, like jumppad targets.",
    target_position_spawnflags,
    target_position_keys
};

void SP_target_position(gentity_t* self)
{
    G_SetOrigin(self, self->s.origin);
    /*
    G_SetAngles( self, self->s.angles );
    self->s.eType = ET_INVISIBLE;
    */
}

static void target_location_linkup(gentity_t* ent)
{
    int i;
    int n;

    if (level.locationLinked)
        return;

    level.locationLinked = qtrue;

    level.locationHead = NULL;

    trap_SetConfigstring(CS_LOCATIONS, "unknown");

    for (i = 0, ent = g_entities, n = 1;
         i < level.num_entities;
         i++, ent++)
    {
        if (ent->classname && !Q_stricmp(ent->classname, "target_location"))
        {
            // lets overload some variables!
            ent->health = n; // use for location marking
            trap_SetConfigstring(CS_LOCATIONS + n, ent->message);
            n++;
            ent->nextTrain = level.locationHead;
            level.locationHead = ent;
        }
    }

    // All linked together now
}

/*QUAKED target_location (0 0.5 0) (-8 -8 -8) (8 8 8)
Set "message" to the name of this location.
Set "count" to 0-7 for color.
0:white 1:red 2:green 3:yellow 4:blue 5:cyan 6:magenta 7:white

Closest target_location in sight used for the location, if none
in site, closest in distance
*/
const entityInfoData_t target_location_spawnflags[] = {
    {NULL, NULL}
};
const entityInfoData_t target_location_keys[] = {
    {"message", "the name of this location"},
    {"count", "0-7 for color. 0 - white, 1 - red, 2 - green, etc."},
    {"targetname", "make the trigger target this value for the entity to be used"},
    {NULL, NULL}
};
const entityInfo_t target_location_info = {
    "Closest target_location in sight used for the location, if none in site, closest in distance",
    target_location_spawnflags,
    target_location_keys
};

void SP_target_location(gentity_t* self)
{
    self->think = target_location_linkup;
    self->nextthink = level.time + 200; // Let them all spawn first

    G_SetOrigin(self, self->s.origin);
}

/*QUAKED target_counter (1.0 0 0) (-4 -4 -4) (4 4 4) x x x x x x x INACTIVE
Acts as an intermediary for an action that takes multiple inputs.

INACTIVE cannot be used until used by a target_activate

target2 - what the counter should fire each time it's incremented and does NOT reach it's count

After the counter has been triggered "count" times (default 2), it will fire all of it's targets and remove itself.

bounceCount - number of times the counter should reset to it's full count when it's done
*/
const entityInfoData_t target_counter_spawnflags[] = {
    {"1", "Start Inactive, must be hit by a target_activate to be able to be used."},
    {NULL, NULL}
};
const entityInfoData_t target_counter_keys[] = {
    {"target", "what to first fire at when the entity as been hit a certain amount of times."},
    {"target2", "fires this every time you target the entity and it isnt at its count"},
    {"count", "the amount of times to be hit for firing its target (default 2)"},
    {
        "bounceCount",
        "set this at -1 so the entity will never stop working, otherwise after the first use it will stop working. if you set bouncecount to 5 then after the fifth use it will stop working."
    },
    {"targetname", "make the trigger target this value for the entity to be used"},
    {NULL, NULL}
};
const entityInfo_t target_counter_info = {
    "You have to use this entity a specified amount of times for it to fire at its target, the default is 2 times ravensoft made it so it will deactivate after it gets hit a certain amount of times",
    target_counter_spawnflags,
    target_counter_keys
};
extern void G_DebugPrint(int level, const char* format, ...);

void target_counter_use(gentity_t* self, gentity_t* other, gentity_t* activator)
{
    if (self->count == 0)
    {
        return;
    }

    //gi.Printf("target_counter %s used by %s, entnum %d\n", self->targetname, activator->targetname, activator->s.number );
    self->count--;

    if (activator)
    {
        G_DebugPrint(WL_VERBOSE, "target_counter %s used by %s (%d/%d)\n", self->targetname, activator->targetname,
                     (self->genericValue1 - self->count), self->genericValue1);
    }

    if (self->count)
    {
        if (self->target2)
        {
            //gi.Printf("target_counter %s firing target2 from %s, entnum %d\n", self->targetname, activator->targetname, activator->s.number );
            G_UseTargets2(self, activator, self->target2);
        }
        return;
    }

    G_ActivateBehavior(self, BSET_USE);

    if (self->spawnflags & 128)
    {
        self->flags |= FL_INACTIVE;
    }

    self->activator = activator;
    G_UseTargets(self, activator);

    if (self->count == 0)
    {
        if (self->bounceCount == 0)
        {
            return;
        }
        self->count = self->genericValue1;
        if (self->bounceCount > 0)
        {
            //-1 means bounce back forever
            self->bounceCount--;
        }
    }
}

void SP_target_counter(gentity_t* self)
{
    self->wait = -1;
    if (!self->count)
    {
        self->count = 2;
    }
    //if ( self->bounceCount > 0 )//let's always set this anyway
    {
        //we will reset when we use up our count, remember our initial count
        self->genericValue1 = self->count;
    }

    //RoboPhred: damn you ravensoft.  You add an entity value specificly for this then forget to hook it to the key...
    G_SpawnInt("bounceCount", "0", &self->bounceCount);

    self->use = target_counter_use;
}

#define TARGET_DOORSTATE_MAX_DOORS 6

static qboolean target_doorstate_is_door(gentity_t* ent)
{
    return ent && ent->classname && (!Q_stricmp(ent->classname, "func_door") || !Q_stricmp(ent->classname, "lmd_door"));
}

static qboolean target_doorstate_matches(gentity_t* door)
{
    const char* want = door->GenericStrings[15];

    if (!want || !want[0])
    {
        return qfalse;
    }

    if (!Q_stricmp(want, "open"))
    {
        return (door->moverState == MOVER_POS2);
    }

    if (!Q_stricmp(want, "close") || !Q_stricmp(want, "closed"))
    {
        return (door->moverState == MOVER_POS1);
    }

    return qfalse;
}

const entityInfoData_t target_doorstate_keys[] = {
    {"target", "Fired when every referenced door matches its wantPosition."},
    {"target2", "Fired when any referenced door is missing or does not match its wantPosition."},
    {"door1", "Targetname of the first door to check."},
    {"door2", "Targetname of the second door to check."},
    {"door3", "Targetname of the third door to check."},
    {"door4", "Targetname of the fourth door to check."},
    {"door5", "Targetname of the fifth door to check."},
    {"door6", "Targetname of the sixth door to check."},
    {NULL, NULL},
};

const entityInfo_t target_doorstate_info = {
    "Checks a list of doors to ensure each is in its configured wantPosition. Fires target when all match, otherwise fires target2.",
    NULL,
    target_doorstate_keys
};

static void target_doorstate_use(gentity_t* self, gentity_t* other, gentity_t* activator)
{
    qboolean allMatch = qtrue;
    qboolean hasDoor = qfalse;
    int i;

    if (!activator)
    {
        activator = self;
    }

    for (i = 0; i < TARGET_DOORSTATE_MAX_DOORS && allMatch; ++i)
    {
        const char* doorTarget = self->GenericStrings[i];
        gentity_t* door = NULL;
        qboolean foundDoor = qfalse;

        if (!doorTarget || !doorTarget[0])
        {
            continue;
        }

        hasDoor = qtrue;

        while ((door = G_Find(door, FOFS(targetname), doorTarget)) != NULL)
        {
            if (!target_doorstate_is_door(door))
            {
                continue;
            }

            foundDoor = qtrue;

            if (!target_doorstate_matches(door))
            {
                allMatch = qfalse;
                break;
            }
        }

        if (!allMatch)
        {
            break;
        }

        if (!foundDoor)
        {
            allMatch = qfalse;
            break;
        }
    }

    if (!hasDoor)
    {
        allMatch = qfalse;
    }

    if (allMatch)
    {
        G_UseTargets(self, activator);
    }
    else if (self->target2 && self->target2[0])
    {
        G_UseTargets2(self, activator, self->target2);
    }
}

void SP_target_doorstate(gentity_t* self)
{
    int i;

    for (i = 0; i < TARGET_DOORSTATE_MAX_DOORS; ++i)
    {
        char keyName[8];

        Com_sprintf(keyName, sizeof(keyName), "door%d", i + 1);
        G_SpawnString(keyName, "", &self->GenericStrings[i]);
        if (self->GenericStrings[i] && !self->GenericStrings[i][0])
        {
            self->GenericStrings[i] = NULL;
        }
    }

    self->use = target_doorstate_use;
}

/*QUAKED target_random (.5 .5 .5) (-4 -4 -4) (4 4 4) USEONCE
Randomly fires off only one of it's targets each time used

USEONCE	set to never fire again
*/
const entityInfoData_t target_random_spawnflags[] = {
    {"1", "Fire once then disable itself"},
    {NULL, NULL}
};
const entityInfoData_t target_random_keys[] = {
    {"target", "should target more than one entity, it will only fire at one of its targets."},
    {"targetname", "make the trigger target this value for the entity to be used"},
    {NULL, NULL}
};
const entityInfo_t target_random_info = {
    "Randomly fires off only one of it\'s targets each time used",
    target_random_spawnflags,
    target_random_keys
};

void target_random_use(gentity_t* self, gentity_t* other, gentity_t* activator)
{
    int t_count = 0, pick;
    gentity_t* t = NULL;

    //gi.Printf("target_random %s used by %s (entnum %d)\n", self->targetname, activator->targetname, activator->s.number );
    G_ActivateBehavior(self, BSET_USE);

    if (self->spawnflags & 1)
    {
        self->use = 0;
    }

    while ((t = G_Find(t, FOFS(targetname), self->target)) != NULL)
    {
        if (t != self)
        {
            t_count++;
        }
    }

    if (!t_count)
    {
        return;
    }

    if (t_count == 1)
    {
        G_UseTargets(self, activator);
        return;
    }

    //FIXME: need a seed
    pick = Q_irand(1, t_count);
    t_count = 0;
    while ((t = G_Find(t, FOFS(targetname), self->target)) != NULL)
    {
        if (t != self)
        {
            t_count++;
        }
        else
        {
            continue;
        }

        if (t == self)
        {
            //				gi.Printf ("WARNING: Entity used itself.\n");
        }
        else if (t_count == pick)
        {
            if (t->use != NULL) // check can be omitted
            {
                GlobalUse(t, self, activator);
                return;
            }
        }

        if (!self->inuse)
        {
            Com_Printf("entity was removed while using targets\n");
            return;
        }
    }
}

void SP_target_random(gentity_t* self)
{
    self->use = target_random_use;
}

int numNewICARUSEnts = 0;

void scriptrunner_run(gentity_t* self)
{
    /*
    if (self->behaviorSet[BSET_USE])
    {	
    char	newname[MAX_FILENAME_LENGTH];

    sprintf((char *) &newname, "%s/%s", Q3_SCRIPT_DIR, self->behaviorSet[BSET_USE] );

    ICARUS_RunScript( self, newname );
    }
    */

    if (self->count != -1)
    {
        if (self->count <= 0)
        {
            self->use = 0;
            self->behaviorSet[BSET_USE] = NULL;
            return;
        }
        else
        {
            --self->count;
        }
    }

    if (self->behaviorSet[BSET_USE])
    {
        if (self->spawnflags & 1)
        {
            if (!self->activator)
            {
                if (g_developer.integer)
                {
                    Com_Printf("target_scriptrunner tried to run on invalid entity!\n");
                }
                return;
            }

            //if ( !self->activator->sequencer || !self->activator->taskManager )
            if (!trap_ICARUS_IsInitialized(self->s.number))
            {
                //Need to be initialized through ICARUS
                if (!self->activator->script_targetname || !self->activator->script_targetname[0])
                {
                    //We don't have a script_targetname, so create a new one
                    self->activator->script_targetname = va("newICARUSEnt%d", numNewICARUSEnts++);
                }

                if (trap_ICARUS_ValidEnt(self->activator))
                {
                    trap_ICARUS_InitEnt(self->activator);
                }
                else
                {
                    if (g_developer.integer)
                    {
                        Com_Printf("target_scriptrunner tried to run on invalid ICARUS activator!\n");
                    }
                    return;
                }
            }

            if (g_developer.integer)
            {
                Com_Printf("target_scriptrunner running %s on activator %s\n", self->behaviorSet[BSET_USE],
                           self->activator->targetname);
            }
            trap_ICARUS_RunScript(self->activator, va("%s/%s", Q3_SCRIPT_DIR, self->behaviorSet[BSET_USE]));
        }
        else
        {
            if (g_developer.integer && self->activator)
            {
                Com_Printf("target_scriptrunner %s used by %s\n", self->targetname, self->activator->targetname);
            }
            G_ActivateBehavior(self, BSET_USE);
        }
    }

    if (self->wait)
    {
        self->nextthink = level.time + self->wait;
    }
}

void target_scriptrunner_use(gentity_t* self, gentity_t* other, gentity_t* activator)
{
    if (self->nextthink > level.time)
    {
        return;
    }

    //Disp(activator, va("Debug: running script %s", 
    //                   self->targetname));
    //Com_Printf("running script %s\n", self->targetname);

    self->activator = activator;
    self->enemy = other;
    if (self->delay)
    {
        //delay before firing scriptrunner
        self->think = scriptrunner_run;
        self->nextthink = level.time + self->delay;
    }
    else
    {
        scriptrunner_run(self);
    }
}

/*QUAKED target_scriptrunner (1 0 0) (-4 -4 -4) (4 4 4) runonactivator x x x x x x INACTIVE
--- SPAWNFLAGS ---
runonactivator - Will run the script on the entity that used this or tripped the trigger that used this
INACTIVE - start off

----- KEYS ------
Usescript - Script to run when used
count - how many times to run, -1 = infinite.  Default is once
wait - can't be used again in this amount of seconds (Default is 1 second if it's multiple-use)
delay - how long to wait after use to run script

*/
const entityInfoData_t target_scriptrunner_spawnflags[] = {
    {"1", "Will run the script on the entity that used this or tripped the trigger that used this"},
    {"128", "Will start in the off state"},
    {NULL, NULL}
};
const entityInfoData_t target_scriptrunner_keys[] = {
    {"useScript", "script to run when used. ex: close_door_cinematic"},
    {"count", "how many times to run, -1 = infinite.  Default is once"},
    {"wait", "can\'t be used again in this amount of seconds (Default is 1 second if it\'s multiple-use)"},
    {"delay", "how long to wait after use to run script"},
    {"targetname", "make the trigger target this value for the entity to be used"},
    {NULL, NULL}
};
const entityInfo_t target_scriptrunner_info = {
    "Starts a script once used.",
    target_scriptrunner_spawnflags,
    target_scriptrunner_keys
};

void SP_target_scriptrunner(gentity_t* self)
{
    /*
    if (g_gametype.integer == GT_FFA
    || g_gametype.integer == GT_TEAM) {
    G_FreeEntity(self);
    return;
    }
    */
    float v;
    if (self->spawnflags & 128)
    {
        self->flags |= FL_INACTIVE;
    }

    if (g_gametype.integer != GT_SIEGE && g_dontLoadNPC.integer)
    {
        if (self->count == 1)
        {
            self->wait = 60; //not sure this is used
        }
        self->count = -1;
    }

    if (!self->count)
    {
        self->count = 1; //default 1 use only
    }
    /*
    else if ( !self->wait )
    {
    self->wait = 1;//default wait of 1 sec
    }
    */
    // FIXME: this is a hack... because delay is read in as an int, so I'm bypassing that because it's too late in the project to change it and I want to be able to set less than a second delays
    // no one should be setting a radius on a scriptrunner, if they are this would be bad, take this out for the next project
    v = 0.0f;
    G_SpawnFloat("delay", "0", &v);
    self->delay = v * 1000; //sec to ms
    self->wait *= 1000; //sec to ms

    G_SetOrigin(self, self->s.origin);
    self->use = target_scriptrunner_use;
}

/*
void G_ToggleActiveState(char *targetstring) //Lugormod
{
gentity_t	*target = NULL;
while( NULL != (target = G_Find(target, FOFS(targetname), targetstring)) )
{
target->flags ^= FL_INACTIVE;
}
}
*/
void G_SetActiveState(char* targetstring, qboolean actState)
{
    gentity_t* target = NULL;
    while (NULL != (target = G_Find(target, FOFS(targetname), targetstring)))
    {
        target->flags = actState ? (target->flags & ~FL_INACTIVE) : (target->flags | FL_INACTIVE);
        /*
        if (actState) {
        Com_Printf("info: %s (%i) activated.", 
        target->targetname, target->s.number);
        } else {
        Com_Printf("info: %s (%i) deactivated.", 
        target->targetname, target->s.number);
        }
        */
    }
}

#define ACT_ACTIVE		qtrue
#define ACT_INACTIVE	qfalse

void target_activate_use(gentity_t* self, gentity_t* other, gentity_t* activator)
{
    G_ActivateBehavior(self, BSET_USE);

    G_SetActiveState(self->target, ACT_ACTIVE);

    //RoboPhred
    if (self->target2)
        G_SetActiveState(self->target2, ACT_ACTIVE);
    if (self->target3)
        G_SetActiveState(self->target3, ACT_ACTIVE);
    if (self->target4)
        G_SetActiveState(self->target4, ACT_ACTIVE);
    if (self->target5)
        G_SetActiveState(self->target5, ACT_ACTIVE);
    if (self->target6)
        G_SetActiveState(self->target6, ACT_ACTIVE);
}

void target_deactivate_use(gentity_t* self, gentity_t* other, gentity_t* activator)
{
    G_ActivateBehavior(self, BSET_USE);

    G_SetActiveState(self->target, ACT_INACTIVE);
    /*
    if (g_dontLoadNPC.integer && //Lugormod indeed try to reset
    (g_gametype.integer == GT_FFA ||
    g_gametype.integer == GT_TEAM)){
    self->think = target_activate_think;
    self->nextthink = level.time + 60000;
    }
    */
    //RoboPhred
    if (self->target2)
        G_SetActiveState(self->target2, ACT_INACTIVE);
    if (self->target3)
        G_SetActiveState(self->target3, ACT_INACTIVE);
    if (self->target4)
        G_SetActiveState(self->target4, ACT_INACTIVE);
    if (self->target5)
        G_SetActiveState(self->target5, ACT_INACTIVE);
    if (self->target6)
        G_SetActiveState(self->target6, ACT_INACTIVE);
}

//FIXME: make these apply to doors, etc too?
/*QUAKED target_activate (1 0 0) (-4 -4 -4) (4 4 4)
Will set the target(s) to be usable/triggerable
*/
const entityInfoData_t target_activate_spawnflags[] = {
    {NULL, NULL}
};
const entityInfoData_t target_activate_keys[] = {
    {"target", "the target to make usable/triggerable"},
    {"target2", "a second target to make usable/triggerable"},
    {"target3", "a third target to make usable/triggerable"},
    {"target4", "a fourth target to make usable/triggerable"},
    {"target5", "a fifth target to make usable/triggerable"},
    {"target6", "a sixth target to make usable/triggerable"},
    {"targetname", "make the trigger target this value for the entity to be used"},
    {NULL, NULL}
};
const entityInfo_t target_activate_info = {
    "Will set the target(s) to be usable/triggerable. Accepts up to 6 targets.",
    target_activate_spawnflags,
    target_activate_keys
};

void SP_target_activate(gentity_t* self)
{
    G_SetOrigin(self, self->s.origin);
    self->use = target_activate_use;
}

/*QUAKED target_deactivate (1 0 0) (-4 -4 -4) (4 4 4)
Will set the target(s) to be non-usable/triggerable
*/
const entityInfoData_t target_deactivate_spawnflags[] = {
    {NULL, NULL}
};
const entityInfoData_t target_deactivate_keys[] = {
    {"target", "the target to make non-usable/triggerable"},
    {"target2", "a second target to make non-usable/triggerable"},
    {"target3", "a third target to make non-usable/triggerable"},
    {"target4", "a fourth target to make non-usable/triggerable"},
    {"target5", "a fifth target to make non-usable/triggerable"},
    {"target6", "a sixth target to make non-usable/triggerable"},
    {"targetname", "make the trigger target this value for the entity to be used"},
    {NULL, NULL}
};
const entityInfo_t target_deactivate_info = {
    "Will set the target(s) to be non-usable/triggerable. Accepts up to 6 targets.",
    target_deactivate_spawnflags,
    target_deactivate_keys
};

void SP_target_deactivate(gentity_t* self)
{
    G_SetOrigin(self, self->s.origin);
    self->use = target_deactivate_use;
}

void target_level_change_use(gentity_t* self, gentity_t* other, gentity_t* activator)
{
    G_ActivateBehavior(self, BSET_USE);

    //RoboPhred
    trap_SendConsoleCommand(EXEC_APPEND, va("map \"%s\"\n", self->message));
    //trap_SendConsoleCommand(EXEC_APPEND, va("map \"%s\"", self->message));
    //trap_SendConsoleCommand(EXEC_NOW, va("map %s", self->message));
}

/*QUAKED target_level_change (1 0 0) (-4 -4 -4) (4 4 4)
"mapname" - Name of map to change to
*/
const entityInfoData_t target_level_change_spawnflags[] = {
    {NULL, NULL}
};
const entityInfoData_t target_level_change_keys[] = {
    {"mapname", "Name of map to change to"},
    {"targetname", "make the trigger target this value for the entity to be used"},
    {NULL, NULL}
};
const entityInfo_t target_level_change_info = {
    "changes the map to whatever you specify when this entity gets used. This is dangerous in older lugormod versions or if not used with OpenJK. Escape sequences and the \';\' can be used in the mapname key maliciously. Use with care and/or remove.",
    target_level_change_spawnflags,
    target_level_change_keys
};

void SP_target_level_change(gentity_t* self)
{
    //RoboPhred
    //if (g_gametype.integer != GT_SINGLE_PLAYER){
    //	G_FreeEntity(self);
    //	return;
    //}

    char* s = NULL;

    G_SpawnString("mapname", "", &s);
    self->message = G_NewString2(s);

    // RoboPhred: dont be naughty
    if (!self->message || !self->message[0] || strchr(self->message, ';') != NULL || strchr(self->message, '\n') !=
        NULL)
    {
        G_FreeEntity(self);
        return;
    }

    G_SetOrigin(self, self->s.origin);
    self->use = target_level_change_use;
}

void target_play_music_use(gentity_t* self, gentity_t* other, gentity_t* activator)
{
    G_ActivateBehavior(self, BSET_USE);
    trap_SetConfigstring(CS_MUSIC, self->message);
}

/*QUAKED target_play_music (1 0 0) (-4 -4 -4) (4 4 4)
target_play_music
Plays the requested music files when this target is used.

"targetname"
"music"		music WAV or MP3 file ( music/introfile.mp3 [optional]  music/loopfile.mp3 )

If an intro file and loop file are specified, the intro plays first, then the looping
portion will start and loop indefinetly.  If no introfile is entered, only the loopfile
will play.
*/
const entityInfoData_t target_play_music_spawnflags[] = {
    {NULL, NULL}
};
const entityInfoData_t target_play_music_keys[] = {
    {"music", "music WAV or MP3 file ex: music/introfile.mp3 or music/loopfile.mp3"},
    {"targetname", "make the trigger target this value for the entity to be used"},
    {NULL, NULL}
};
const entityInfo_t target_play_music_info = {
    "Plays the requested music files when this target is used.",
    target_play_music_spawnflags,
    target_play_music_keys
};

void SP_target_play_music(gentity_t* self)
{
    char* s = NULL;

    G_SetOrigin(self, self->s.origin);
    if (!G_SpawnString("music", "", &s))
    {
        //G_Error( "target_play_music without a music key at %s", vtos( self->s.origin ) );
        G_Free(self);
        return;
    }

    self->message = G_NewString(s);

    self->use = target_play_music_use;
}

/*
==============
target_credits
==============
*/

const entityInfoData_t target_credits_keys[] = {
    {"count", "the amount of credits to give the activator"},
    {
        "random",
        "the amount of credits you want to randomly receive. This number is added onto the count. If your count is 20 and you set this to 10 you will receive anywhere from 20-30 credits"
    },
    {
        "targetname",
        "make the trigger target this value for the entity to be used, the person who triggers this will receive the set amount of credits"
    },
    {NULL, NULL},
};
entityInfo_t target_credits_info = {
    "Gives the person who fires this an amount of credits",
    NULL,
    target_credits_keys
};

void Use_Target_Credits(gentity_t* ent, gentity_t* other, gentity_t* activator)
{
    //RoboPhred: silly lugor
    int activatorCreds;

    if (ent->flags & FL_INACTIVE)
    {
        //if (ent->spawnflags & 1) {
        //deactivated
        return;
    }
    if (!activator || !activator->client || activator->NPC || activator->r.svFlags & SVF_BOT)
        return;

    if (activator->client->pers.Lmd.account == NULL)
        return;

    if (activator->health < 1)
        return;

    if (activator->client->pers.connected != CON_CONNECTED)
        return;


    if (activator->client->ps.pm_type == PM_SPECTATOR) //spectators don't pick stuff up
        return;

    if (ent->genericValue6 && ent->genericValue6 > level.time)
        return;

    activatorCreds = PlayerAcc_GetCredits(activator);


    ent->genericValue6 = 0;

    if (ent->wait)
        ent->genericValue6 = level.time + ent->wait;

    int amount = ent->count;

    if (ent->random > 0)
    {
        amount += Q_irand(0, ent->random);
    }

    if (amount < 0 && (-amount) > activatorCreds)
    {
        trap_SendServerCommand(activator->s.number, va("cp \"^3You cannot afford ^1CR %i^3.\"", -amount));
        amount = 0;
        G_UseTargets2(ent, activator, ent->target2);
    }
    else
    {
        if (!(ent->spawnflags & 1))
        {
            if (amount > 0)
            {
                G_Sound(activator, CHAN_AUTO, G_SoundIndex("sound/interface/secret_area.wav"));
                trap_SendServerCommand(activator->s.number, va("cp \"^3You received ^2CR %i^3.\"", amount));
            }
            else
                trap_SendServerCommand(activator->s.number, va("cp \"^3You lost ^1CR %i^3.\"", -amount));
        }
        PlayerAcc_SetCredits(activator, activatorCreds + amount);
        G_UseTargets(ent, activator);
    }
}

void SP_target_credits(gentity_t* ent)
{
    //if ( !ent->count ) {
    //	ent->count = 1;
    //}
    ent->use = Use_Target_Credits;
    ent->genericValue6 = 0;
}
