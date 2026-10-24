#include "g_local.h"

#include "Lmd_Data.h"
#include "Lmd_KeyPairs.h"
#include "Lmd_Commands_Data.h"
#include "Lmd_Commands_Auths.h"
#include "Lmd_PlayerActions.h"
#include "Lmd_EntityCore.h"
#include "Lmd_Confirm.h"
#include "Lmd_Console.h"

#define GRAB_ORIGIN 1
#define GRAB_X 2
#define GRAB_Y 3
#define GRAB_Z 4

// Forward declaration for action handler
qboolean Action_UndoDelete(gentity_t* ent, Action_t* action);

gentity_t* AimAnyTarget(gentity_t* ent, int length);
char* ConcatArgs(int start);
#define STANDARD_BEAM "env/hevil_bolt"
BG_field_t* getField(const char* fieldName);


char* FormattedEntString(gentity_t* ent)
{
    return va("^%i%s", (ent->Lmd.spawnData) ? 5 : 3, ent->classname);
}

void DismembermentTest(gentity_t* self);
void Cmd_Kill_f(gentity_t* ent);

void BlowUpEntity(gentity_t* ent)
{
    vec3_t upVec, pos;
    VectorCopy(ent->r.currentOrigin, pos);
    pos[2] = pos[2] + ent->r.mins[2] + 64;

    VectorSet(upVec, -90, 0, 0);
    G_PlayEffectID(G_EffectIndex("env/ion_cannon_explosion"), pos, upVec);

    G_RadiusDamage(pos, NULL, 150, 256, ent, ent, MOD_TRIGGER_HURT);
    if (ent->s.number >= MAX_CLIENTS)
    {
        G_FreeEntity(ent);
    }
    else
    {
        ent->client->noCorpse = qtrue;
        Cmd_Kill_f(ent);
        if (ent->health < 1)
        {
            DismembermentTest(ent);
        }
    }
}

extern void G_TestLine(vec3_t start, vec3_t end, int color, int time);

void Lmd_Display_Bounds(gentity_t* targ, int lineTime)
{
    // Too early to refresh the "test lines"
    if (level.time < targ->Lmd.boundsDisplayedUntil)
    {
        return;
    }

    targ->Lmd.boundsDisplayedUntil = level.time + lineTime;

    vec3_t ofs;
    VectorSubtract(targ->r.absmax, targ->r.absmin, ofs);

    // Bottom

    vec3_t b1;
    vec3_t b2;
    vec3_t b3;
    vec3_t b4;

    VectorCopy(targ->r.absmin, b1);
    VectorCopy(targ->r.absmin, b2);
    VectorCopy(targ->r.absmin, b3);
    VectorCopy(targ->r.absmin, b4);

    b2[0] += ofs[0];
    b3[1] += ofs[1];
    b4[0] += ofs[0];
    b4[1] += ofs[1];

    G_TestLine(b1, b2, 0x000000ff, lineTime);
    G_TestLine(b1, b3, SABER_GREEN, lineTime);
    G_TestLine(b2, b4, SABER_GREEN, lineTime);
    G_TestLine(b3, b4, 0x000000ff, lineTime);

    // Top

    vec3_t t1;
    vec3_t t2;
    vec3_t t3;
    vec3_t t4;

    VectorCopy(b1, t1);
    VectorCopy(b2, t2);
    VectorCopy(b3, t3);
    VectorCopy(b4, t4);

    t1[2] += ofs[2];
    t2[2] += ofs[2];
    t3[2] += ofs[2];
    t4[2] += ofs[2];

    G_TestLine(t1, t2, 0x000000ff, lineTime);
    G_TestLine(t1, t3, SABER_GREEN, lineTime);
    G_TestLine(t2, t4, SABER_GREEN, lineTime);
    G_TestLine(t3, t4, 0x000000ff, lineTime);


    // Sides
    G_TestLine(b1, t1, SABER_BLUE, lineTime);
    G_TestLine(b2, t2, SABER_BLUE, lineTime);
    G_TestLine(b3, t3, SABER_BLUE, lineTime);
    G_TestLine(b4, t4, SABER_BLUE, lineTime);
}

void Lmd_Bounds_Think(gentity_t* targ)
{
	if (!targ->inuse || !targ->Lmd.displayBounds)
	{
		targ->think = NULL;
		return;
	}

	const int lineTime = 1000;
	Lmd_Display_Bounds(targ, lineTime);
	targ->nextthink = level.time + lineTime;
}


int G_FindConfigstringIndex(const char* name, int start, int max, qboolean create);
extern vec3_t model_mins[MAX_MODELS];
extern vec3_t model_maxs[MAX_MODELS];
unsigned char G_ReadMD3MinsMaxes(const fileHandle_t fh, vec3_t mins, vec3_t maxs);

void Cmd_Bounds_f(gentity_t* ent, int iArg)
{
    // If no args, trace model and give built in and current
    // If int args, use int as entity num and give built in and current
    // If string args, use as model path
    char* arg = ConcatArgs(1);
    int i = atoi(arg);
    gentity_t* targ = NULL;
    if (!arg[0])
    {
        targ = AimAnyTarget(ent, 8192);
    }
    else if (i != 0 || (arg[0] == '0' && arg[1] == 0))
    {
        targ = GetEnt(i);
    }

    if (targ)
    {
        if (!targ->Lmd.displayBounds)
        {
            // The entity isn't displaying its bounds (yet)
            // Show entity type + number, then current bounds and contents
            Disp(ent, va("^3%s ^7(^2%d^7)", FormattedEntString(targ), targ->s.number));
            Disp(ent, va(
                CT_B"Mins: "CT_V"%s\n"
                CT_B"Maxs: "CT_V"%s",
                vtos2(targ->r.mins),
                vtos2(targ->r.maxs)));
            Disp(ent, va("^3Contents: ^2%x ^3Clipmask: ^2%x", targ->r.contents, targ->clipmask));
            // Toggle the bounding box ON
            targ->Lmd.displayBounds = qtrue;
            if (!targ->think)
            {
                targ->think = Lmd_Bounds_Think;
                targ->nextthink = level.time;
            }
            Disp(ent, "^3Bounding box ^2ON");
        }
        else
        {
            // The entity is currently displaying its bounds => Restore entity "think"
            // Toggle the bounding box OFF
            targ->Lmd.displayBounds = qfalse;
            if (targ->Lmd.oldThink)
            {
                targ->think = targ->Lmd.oldThink;
                targ->nextthink = level.time + targ->Lmd.oldNextthink;
            }
            else
            {
                targ->think = NULL;
                targ->nextthink = 0;
            }
            // Reset other fields
            targ->Lmd.oldThink = 0;
            targ->Lmd.oldNextthink = 0;
            // Notify the user
            Disp(ent, va("^3%s ^7(^2%d^7) - Bounding box ^1OFF", FormattedEntString(targ), targ->s.number));
        }
        return;
    }
    else if (arg[0])
    {
        int configIndex = G_FindConfigstringIndex(arg, CS_MODELS, MAX_MODELS, qfalse);
        if (configIndex > 0)
        {
            Disp(ent, va(
                     CT_B"Mins: "CT_V"%s\n"
                     CT_B"Maxs: "CT_V"%s",
                     vtos(model_mins[configIndex]),
                     vtos(model_maxs[configIndex])));
        }
        else
        {
            fileHandle_t fh = 0;
            char model[MAX_STRING_CHARS];
            char* p;
            if (!Lmd_Data_IsCleanPath(arg))
            {
                Disp(ent, CT_B"Invalid path.");
                return;
            }

            Q_strncpyz(model, arg, sizeof(model));
            // Why someone would want stuff.glm.md3, I dunno... but support it anyway.
            if ((p = strstr(model, ".glm")) && p[4] == 0)
            {
                Disp(ent, CT_B"Ghoul2 models are not supported.");
                return;
            }
            if (!(p = strstr(model, ".md3")) || p[4] != 0)
            {
                Q_strcat(model, sizeof(model), ".md3");
            }

            trap_FS_FOpenFile(model, &fh, FS_READ);
            if (!fh)
            {
                //try models/ then, this is assumed for registering models
                trap_FS_FOpenFile(va("models/%s", model), &fh, FS_READ);
                //trap_FS_FOpenFile(name, &fh, FS_READ);
                if (!fh)
                {
                    Disp(ent, "^3Model not found, or not a valid md3 model.");
                    return;
                }
            }

            if (fh)
            {
                vec3_t mins;
                vec3_t maxs;
                G_ReadMD3MinsMaxes(fh, mins, maxs);
                Disp(ent, va(CT_V"%s", model));
                Disp(ent, va(
                         CT_B"Mins: "CT_V"%s\n"
                         CT_B"Maxs: "CT_V"%s",
                         vtos(mins),
                         vtos(maxs)));
                trap_FS_FCloseFile(fh);
            }
        }
    }
    else
    {
        Disp(ent, CT_B"Usage: \'"CT_C"bounds "CT_AO"[model name | entity number]");
    }
}

void Cmd_BlowUp_f(gentity_t* ent, int iArg)
{
    gentity_t* tEnt;
    char* spawnstring = NULL;
    int   sLen = 0;
    Action_t* action;
    char  entInfo[128];

    if (trap_Argc() > 1)
        tEnt = GetEnt(atoi(ConcatArgs(1)));
    else {
        G_PlayEffectID(G_EffectIndex(STANDARD_BEAM),
            ent->client->renderInfo.eyePoint,
            ent->client->ps.viewangles);
        tEnt = AimAnyTarget(ent, 8192);
    }

    if (!tEnt || !tEnt->inuse) { Disp(ent, "^3Invalid entity."); return; }
    if (tEnt->s.number == ENTITYNUM_WORLD) { Disp(ent, "^3You cannot blow up the worldspawn."); return; }

    Com_sprintf(entInfo, sizeof(entInfo), "^2%s ^7(^3%d^7)",
        FormattedEntString(tEnt), tEnt->s.number);

    if (tEnt->Lmd.spawnData) {
        sLen = Lmd_Entites_GetSpawnstringLen(tEnt->Lmd.spawnData);
        if (sLen > 0) {
            spawnstring = (char*)G_Alloc(sLen);
            Lmd_Entities_getSpawnstring(tEnt->Lmd.spawnData, spawnstring, sLen);
        }
    }

    BlowUpEntity(tEnt);

    Disp(ent, va("^3Detonated entity: %s", entInfo));
    if (spawnstring)
        Disp(ent, va("^3Spawnstring:\n^2%s", spawnstring));

    if (spawnstring) {
        if ((action = PlayerActions_Add(ent, "undodelete",
            "View the spawnstring of, or respawn, the last detonated entity",
            Action_UndoDelete, qtrue))) {
            action->strArgs[0] = spawnstring;
            action->iArgs[0] = Lmd_Entities_IsSaveable(tEnt);
        }
        else {
            G_Free(spawnstring);
        }
    }
    else {
        Disp(ent, "^3No spawnstring - cannot undo.");
    }
}

void Cmd_NextMap_f(gentity_t *ent, int iArg) {
	trap_SendConsoleCommand(EXEC_APPEND, "vstr nextmap");
}

gentity_t* Lmd_logic_entity(int index);

void Cmd_Nudge_f(gentity_t *ent, int iArg){
	gentity_t *targ = NULL;//, *newent;
	vec3_t NudgeNudge, v2;
	SpawnData_t *curSpawn, *backupSpawn;
	char arg[MAX_STRING_CHARS];
	if(trap_Argc() > 5 || trap_Argc() < 4) {
		Disp(ent, "^3Usage: nudge ^5[entity number] ^2<x> <y> <z>");
		return;
	}
	if (trap_Argc() == 5){
		trap_Argv(1, arg, sizeof(arg));
		int EntNum = atoi(arg);
		if(EntNum >= 0) {
			if(EntNum >= MAX_CLIENTS && EntNum < MAX_GENTITIES) {
				targ = &g_entities[EntNum];
			}
			else if(EntNum >= MAX_GENTITIES) {
				targ = Lmd_logic_entity(EntNum - MAX_GENTITIES);
			}
		}

		if(!targ) {
			Disp(ent, "^3Invalid entity index.");
			return;

		}

		trap_Argv(2, arg, sizeof(arg));
		NudgeNudge[0] = atoi(arg);
		trap_Argv(3, arg, sizeof(arg));
		NudgeNudge[1] = atoi(arg);
		trap_Argv(4, arg, sizeof(arg));
		NudgeNudge[2] = atoi(arg);
	} 
	else{
		targ = AimAnyTarget(ent, 8192);
		trap_Argv(1, arg, sizeof(arg));
		NudgeNudge[0] = atoi(arg);
		trap_Argv(2, arg, sizeof(arg));
		NudgeNudge[1] = atoi(arg);
		trap_Argv(3, arg, sizeof(arg));
		NudgeNudge[2] = atoi(arg);
	}
	if(!targ || !targ->Lmd.spawnData){
		Disp(ent, "^3Invalid entity.");
		return;
	}

	if(Lmd_Entities_getSpawnstringKey(targ->Lmd.spawnData, "origin", arg, sizeof(arg)))
		sscanf(arg, "%f %f %f", &v2[0], &v2[1], &v2[2]);
	else
		VectorCopy(vec3_origin, v2);
	VectorAdd(v2, NudgeNudge, v2);

	qboolean wasBoundingBoxDisplayed = targ->Lmd.displayBounds;
	curSpawn = targ->Lmd.spawnData;
	backupSpawn = cloneSpawnstring(curSpawn);
	Lmd_Entities_setSpawnstringKey(curSpawn, "origin", vtos2(v2));

	if(spawnEntity(targ, curSpawn) == NULL){
		Disp(ent, "^3Entity failed to respawn, reverting...");
		if (spawnEntity(targ, backupSpawn) == NULL) {
			Disp(ent, "^1Failed to revert to old spawnstring, entity did not spawn.");
			return;
		}
	}
	else{
		Disp(ent, "^2Entity nudged.");
		removeSpawnstring(backupSpawn);
	}

	if (wasBoundingBoxDisplayed) {
		targ->Lmd.displayBounds = qtrue;
		targ->think = Lmd_Bounds_Think;
		targ->nextthink = level.time;
	}
}

void Lmd_ModifyMaxsMinsSelect(gentity_t *ent, qboolean push)
{
	gentity_t *tEnt = GetEnt(ent->client->pers.Lmd.selectedEntity);
	if (!tEnt || !tEnt->inuse)
	{
		ent->client->pers.Lmd.selectedEntity = 0;
		Disp(ent, "^1SelectedEntity not found anymore. Selection cleared.");
		return;
	}

	if (tEnt->Lmd.displayBounds)
		tEnt->Lmd.displayBounds = qfalse;
	SpawnData_t *curSpawn = tEnt->Lmd.spawnData;

	SpawnData_t *backupSpawn = cloneSpawnstring(curSpawn);
	vec3_t mins, maxs;

	// Brush models use absmin/absmax relative to origin, not mins/maxs
	if (tEnt->model && tEnt->model[0] == '*')
	{
		VectorSubtract(tEnt->r.absmin, tEnt->r.currentOrigin, mins);
		VectorSubtract(tEnt->r.absmax, tEnt->r.currentOrigin, maxs);
	}
	else
	{
		VectorCopy(tEnt->r.mins, mins);
		VectorCopy(tEnt->r.maxs, maxs);
	}

	qboolean affectXY = ent->client->buttons & BUTTON_WALKING;
	qboolean affectZ  = ent->client->buttons & BUTTON_USE;

	for ( int i = 0; i < 3; i++ )
	{
		qboolean affect = qfalse;

		if ( affectXY )
		{
			affect = (i < 2);
		}
		else if ( affectZ )
		{
			affect = (i == 2);
		}
		else
		{
			affect = qtrue;
		}

		if ( !affect )
			continue;

		if ( push )
		{
			mins[i] -= 1.0f;
			maxs[i] += 1.0f;
		}
		else
		{
			mins[i] += 1.0f;
			maxs[i] -= 1.0f;

			if ( mins[i] > maxs[i] )
			{
				float mid = (mins[i] + maxs[i]) * 0.5f;
				mins[i] = mid;
				maxs[i] = mid;
			}
		}
	}


	char minsStr[64], maxsStr[64];
	Com_sprintf(minsStr, sizeof(minsStr), "%i %i %i",
		(int)mins[0], (int)mins[1], (int)mins[2]);
	Com_sprintf(maxsStr, sizeof(maxsStr), "%i %i %i",
		(int)maxs[0], (int)maxs[1], (int)maxs[2]);
					
	// Remove brush model so entity uses mins/maxs instead of BSP bounds
	if (tEnt->model && tEnt->model[0] == '*')
	{
		Lmd_Entities_deleteSpawnstringKey(curSpawn, "model");
	}

	Lmd_Entities_setSpawnstringKey(curSpawn, "mins", minsStr);
	Lmd_Entities_setSpawnstringKey(curSpawn, "maxs", maxsStr);

	if(spawnEntity(tEnt, curSpawn) == NULL){
		Disp(ent, "^3Entity failed to respawn, reverting...");
		if (spawnEntity(tEnt, backupSpawn) == NULL) {
			Disp(ent, "^1Failed to revert to old spawnstring, entity did not spawn.");
			return;
		}
	}
	else{
		tEnt->Lmd.displayBounds = qtrue;
		tEnt->think = Lmd_Bounds_Think;
		tEnt->nextthink = level.time;
		Disp(ent, "^3Mins (^2%s^3). Maxs (^2%s^3).", minsStr, maxsStr);
		removeSpawnstring(backupSpawn);
	}
}

qboolean Action_UndoDelete(gentity_t* ent, Action_t* action)
{
	char arg[MAX_STRING_CHARS];
	//actions undodelete <arg>
	trap_Argv(2, arg, sizeof(arg));
	if (Q_stricmp(arg, "respawn") == 0)
	{
		SpawnData_t* spawnData;
		gentity_t* spawn;
		Disp(ent, va("^3Restoring entity with spawnstring: \n^2%s", action->strArgs[0]));
		spawnData = ParseEntitySpawnstring(action->strArgs[0]);
		Lmd_Entities_SetSaveable(spawnData, action->iArgs[0]);
		if (!(spawn = spawnEntityFromData(spawnData)))
			Disp(ent, "^3Failed to spawn entity.");
		else
		{
			Disp(ent, va("^3Entity respawned with entity number ^2%i^3.", spawn->s.number));
			return qtrue;
		}
	}
	else
	{
		Disp(ent, va(
				 "^3Last deleted entity had spawnstring:\n^2%s\n^3Use \'^2/actions undodelete respawn^3\' to respawn it.",
				 action->strArgs[0]));
	}
	return qfalse;
}

qboolean Action_UndoPlace(gentity_t *ent, Action_t *action){
	char arg[MAX_STRING_CHARS], *spawnstring;
	int sLen;
	gentity_t *last = NULL;
	if(action->iArgs[0] < MAX_GENTITIES)
		last = &g_entities[action->iArgs[0]];
	else
		last = Lmd_logic_entity(action->iArgs[0] - MAX_GENTITIES);
	if(!last) {
		Disp(ent, "^3Invalid entity number.");
		return qtrue;
	}

	//check to make sure the entity was not deleted already.
	sLen = Lmd_Entites_GetSpawnstringLen(last->Lmd.spawnData);
	spawnstring = (char *)G_Alloc(sLen);
	Lmd_Entities_getSpawnstring(last->Lmd.spawnData, spawnstring, sLen);
	if(!last->inuse || !last->Lmd.spawnData || Q_stricmp(action->strArgs[0], spawnstring) != 0){
		Disp(ent, "^3The last placed entity was deleted or modified.");
		return qtrue;
	}
	
	//actions undoplace <arg>
	trap_Argv(2, arg, sizeof(arg));
	if(Q_stricmp(arg, "delete") == 0){
		Action_t *newAction;
		if((newAction = PlayerActions_Add(ent, "undodelete", "View the spawnstring of, or respawn, the last deleted entity", Action_UndoDelete, qtrue))){
			newAction->strArgs[0] = G_NewString(spawnstring);
			newAction->iArgs[0] = Lmd_Entities_IsSaveable(last);
		}
		G_FreeEntity(last);
		Disp(ent, "^3Last placed entity deleted.  You can undo the deletion with the undodelete action.");
		return qtrue;
	}
	else{
		Disp(ent, va("^3The last placed entity has an entity number of ^2%i^3 and has the spawnstring:\n^2%s",
			action->iArgs[0], spawnstring));
		Disp(ent, "^3Use \'^2/actions undoplace delete^3\' to delete this entity.");
	}
	G_Free(spawnstring);
	return qfalse;
}

//RoboPhred
qboolean isValidClassname(const char *classname);
extern gentity_t *errorMessageTarget;
void Cmd_Place_f (gentity_t *ent, int iArg){
	char classname[MAX_STRING_CHARS];
	char arg[MAX_STRING_CHARS];
	char *string;
	int sLen;
	gentity_t *spawn;
	Action_t *action;
	trace_t tr;
	SpawnData_t *spawnData;

	if (trap_Argc() < 3) {
		Disp(ent, "^3Usage: place <classname> <dist> <field>,<value>,...");
		return;
	}

	trap_Argv(1, classname, sizeof(classname));

	if(!isValidClassname(classname)) {
		Disp(ent, "^3There is no entity by that classname.");
		return;
	}

	string = ConcatArgs(3);

	string = va("classname,%s,%s", classname,string);

	trap_Argv(2, arg, sizeof(arg));
	spawnData = ParseEntitySpawnstring(string);
	if(arg[0] != '*') {
		vec3_t origin, dir;
		int move = atoi(arg);
		VectorCopy(ent->client->renderInfo.eyePoint, origin);
		AngleVectors(ent->client->ps.viewangles, dir,NULL,NULL);
		VectorNormalize(dir);
		VectorMA(origin, 8192, dir, origin);
		trap_Trace(&tr, ent->client->renderInfo.eyePoint, NULL, NULL, origin, ent->s.number, ent->clipmask);
		VectorCopy(tr.endpos, origin);

		if(move)
			VectorMA(origin, move, tr.plane.normal, origin);
		Lmd_Entities_setSpawnstringKey(spawnData, "origin", vtos2(origin));
	}

	if(Auths_PlayerHasAuthFlag(ent, AUTH_SAVEPLACED))
		Lmd_Entities_SetSaveable(spawnData, qtrue);

	errorMessageTarget = ent;
	spawn = spawnEntityFromData(spawnData);
	errorMessageTarget = NULL;

	if(!spawn){
		Disp(ent, "^3Failed to spawn entity.");
		Disp(ent, va("^3Try using \'^2/entityinfo %s^3\' for help on spawning this entity.", classname));
		return;
	}

	Disp(ent, va("^3Entity spawned as number ^2%i^3.", spawn->s.number));


	if(classname[0] != '*') {
		if (spawn->r.contents) {
			qboolean change = qfalse;

			if(tr.plane.normal[2] > 0 && spawn->r.mins[2]){
				spawn->s.origin[2] -= spawn->r.mins[2];
				spawn->r.currentOrigin[2] -= spawn->r.mins[2];
				spawn->pos1[2] -= spawn->r.mins[2];
				spawn->pos2[2] -= spawn->r.mins[2];
				spawn->pos3[2] -= spawn->r.mins[2];
				spawn->s.pos.trBase[2] -= spawn->r.mins[2];
				change = qtrue;
			}
			else if(tr.plane.normal[2] < 0 && spawn->r.maxs[2]){
				spawn->s.origin[2] -= spawn->r.maxs[2];
				spawn->r.currentOrigin[2] -= spawn->r.maxs[2];
				spawn->pos1[2] -= spawn->r.maxs[2];
				spawn->pos2[2] -= spawn->r.maxs[2];
				spawn->pos3[2] -= spawn->r.maxs[2];
				spawn->s.pos.trBase[2] -= spawn->r.maxs[2];
				change = qtrue;
			}

			if(change){
				//RoboPhred: messes up doors, done above now.
				//G_SetOrigin(spawn, spawn->s.origin);
				//RoboPhred: reset origin key
				Lmd_Entities_setSpawnstringKey(spawnData, "origin", vtos2(spawn->s.origin));
				trap_LinkEntity(spawn);
			}
		}
	}

	sLen = Lmd_Entites_GetSpawnstringLen(spawn->Lmd.spawnData);
	string = (char *)G_Alloc(sLen);
	Lmd_Entities_getSpawnstring(spawn->Lmd.spawnData, string, sLen);
	if((action = PlayerActions_Add(ent, "undoplace", "View the entity number and spawnstring of the last placed entity, and delete it", Action_UndoPlace, qtrue))){
		action->strArgs[0] = classname; //we dont free the alloced string, so this is fine.
		action->iArgs[0] = spawn->s.number;
	}
}

void placeinfront (gentity_t * ent, const char *string) {
	gentity_t *spawn;
	vec3_t          origin,dir;
	VectorCopy(ent->r.currentOrigin, origin);
	AngleVectors(ent->client->ps.viewangles, dir,NULL,NULL);
	dir[2] = 0;
	VectorNormalize(dir);
	VectorMA(origin, ent->r.maxs[0] + 40,dir, origin);
	if(!(spawn = trySpawn(va("origin,%s,angle,%.0f,%s", vtos2(origin), ent->client->ps.viewangles[YAW], string))))
		Disp(ent, "^3Failed to spawn entity.");
	else
		Disp(ent, va("^3Entity spawned as number ^2%i^3.", spawn->s.number));
}

void Cmd_Select_f(gentity_t *ent, int iArg)
{
	char arg[MAX_STRING_CHARS];
	if (trap_Argc() < 2)
	{
		Disp(ent, "^3Usage: SelectEnt <entity number>");
		return;
	}
	
	trap_Argv(1, arg, sizeof(arg));
	int i = atoi(arg);
	gentity_t* target = GetEnt(i);
	if (!target || !target->inuse)
	{
		Disp(ent, "^3Invalid entity.");
		return;
	}

	if (ent->client->pers.Lmd.selectedEntity == i)
	{
		ent->client->pers.Lmd.selectedEntity = 0;
		target->Lmd.displayBounds = qfalse;
		Disp(ent, "^3Unselected entity ^2%i^3.", i);
		return;
	}
	
	ent->client->pers.Lmd.selectedEntity = i;
	Disp(ent, "^3Selected entity ^2%i^3.", i);
}

void Cmd_PlaceCannon_f (gentity_t *ent, int iArg){
	placeinfront(ent, "classname,emplaced_eweb,count,60000,spawnflags,1,");
}

void SP_random_spot (gentity_t *ent);
void Cmd_PlaceRandomSpot_f (gentity_t *ent, int iArg){
	gentity_t *spot;
	if(!(spot = trySpawn(va("classname,random_spot,origin,%s,angle,%.0f", vtos2(ent->client->ps.origin), ent->client->ps.viewangles[YAW])))){
		Disp(ent, "^3Failed to spawn entity.");
		return;
	}
	else
		Disp(ent, va("^3Entity spawned as number ^2%i^3.", spot->s.number));

	if(Auths_PlayerHasAuthFlag(ent, AUTH_SAVEPLACED))
		Lmd_Entities_SetSaveable(spot->Lmd.spawnData, qtrue);
}

void listRemaps(gentity_t *ent, int offset);
qboolean removeRemap(int index);
void Cmd_Remap_f(gentity_t *ent, int iArg){
	char arg[MAX_STRING_CHARS];
	int argc = trap_Argc();
	if(argc == 1){
		Disp(ent, "^3Usage: remap {^2list ^6[starting offset]^3} {^2remove ^5<index>^3} {^2replace ^5<old shader> <new shader>^3}");
		return;
	}
	trap_Argv(1, arg, sizeof(arg));
	if(Q_stricmp(arg, "list") == 0){
		trap_Argv(2, arg, sizeof(arg));
		listRemaps(ent, atoi(arg));
		return;
	}
	else if(Q_stricmp(arg, "remove") == 0){
		int i;
		trap_Argv(2, arg, sizeof(arg));
		i = atoi(arg);
		if(argc < 3 || i < 0 || (arg == 0 && !(arg[0] == '0' && arg[1] == 0))){
			Disp(ent, "^3You must enter the index of the remap to remove.");
			return;
		}
		if(!removeRemap(i))
			Disp(ent, "^3Index out of range.");
		else
			Disp(ent, "^2Remap removed.");
	}
	else if(Q_stricmp(arg, "replace") == 0){
		char old[MAX_QPATH];
		if(argc < 4){
			Disp(ent, "^3You must enter the old shader and the new shader to replace it with.");
			return;
		}
		trap_Argv(2, old, sizeof(old));
		trap_Argv(3, arg, sizeof(arg));
		AddRemap(old, arg, level.time * 0.001);
		trap_SetConfigstring(CS_SHADERSTATE, BuildShaderStateConfig());
		Disp(ent, "^2Remap added.");
	}
	else
		Disp(ent, "^3Uknown argument.  Use the command without args for its usage.");


	//trap_G2API_InitGhoul2Model: one of the args is a custom shader, check it?
}

void Cmd_ShowEntities_f (gentity_t *ent, int iArg){
	char arg[MAX_STRING_CHARS];
	int i, len, val, count = 0;
	gentity_t *point;
	vec3_t dir;
	if (trap_Argc() < 2)
		return;
	VectorSet(dir, -90, 0, 0);
	trap_Argv(1,arg,sizeof(arg));
	val = atoi(arg);
	len = strlen(arg);

	for (i = MAX_CLIENTS; i < level.num_entities && count < 20; i++) {
		point = &g_entities[i];
		if (!point->inuse || (Q_strncmp(arg, point->classname, len) && i != val))
			continue;
		count++;
		gentity_t *te = G_PlayEffectID(G_EffectIndex(STANDARD_BEAM),point->s.origin, dir);
		te->r.svFlags |= SVF_SINGLECLIENT;
		te->r.singleClient = ent->s.number;
	}
}

void Cmd_Spawnstring_f(gentity_t *ent, int iArg){
	char arg[MAX_STRING_CHARS];
	char arg2[MAX_STRING_CHARS];
	gentity_t *targ;
	int argc = trap_Argc();
	trap_Argv(1, arg, sizeof(arg));
	trap_Argv(2, arg2, sizeof(arg2));

	if(argc < 2){
		Disp(ent, "^3Usage: /spawnstring {view [entity number]} {edit [entity number] <key> <value>} {deletekey <key name>}");
		return;
	}

	if(Q_stricmp(arg, "view") == 0){
		char *str;
		int slen;
		if(argc >= 3){
			int i = atoi(arg2);	
			if(i < MAX_GENTITIES)
				targ = &g_entities[i];
			else
				targ = Lmd_logic_entity(i - MAX_GENTITIES);
		}
		else
			targ = AimAnyTarget(ent, 2048);

		if(!targ || !targ->inuse){
			Disp(ent, "^3Invalid target.");
			return;
		}
		else if(!targ->Lmd.spawnData) {
			Disp(ent, "^3Target is not a custom entity.");
			return;
		}

		slen = Lmd_Entites_GetSpawnstringLen(targ->Lmd.spawnData);
		str = (char *)G_Alloc(slen);
		Lmd_Entities_getSpawnstring(targ->Lmd.spawnData, str, slen);
		Disp(ent, va("^2%s", str));
		G_Free(str);
	}
	else if(Q_stricmp(arg, "edit") == 0){
		SpawnData_t *curSpawn, *backupSpawn;
		//gentity_t *newent;
		//spawnstring edit <index> <key> <value>
		if(argc < 4){
			Disp(ent, "^3Invalid arguments, expected \'^2spawnstring edit <index> <key> <value>.");
			return;
		}
		if(argc >= 5){
			int i = atoi(arg2);
			if(i < MAX_GENTITIES)
				targ = &g_entities[i];
			else
				targ = Lmd_logic_entity(i - MAX_GENTITIES);
		}
		else{
			targ = AimAnyTarget(ent, 2048);
		}

		if(!targ || !targ->inuse || !targ->Lmd.spawnData){
			Disp(ent, "^3Invalid target.");
			return;
		}

		trap_Argv(2 + (argc >= 5), arg, sizeof(arg));
		trap_Argv(3 + (argc >= 5), arg2, sizeof(arg2));

		qboolean wasBoundingBoxDisplayed = targ->Lmd.displayBounds;
		curSpawn = targ->Lmd.spawnData;

		backupSpawn = cloneSpawnstring(curSpawn);
		Lmd_Entities_setSpawnstringKey(curSpawn, arg, arg2);

		if(spawnEntity(targ, curSpawn) == NULL){
			Disp(ent, "^3Entity failed to respawn, reverting...");
			if (spawnEntity(targ, backupSpawn) == NULL) {
				Disp(ent, "^1Failed to revert to old spawnstring, entity did not spawn.");
				return;
			}
		}
		else{
			Disp(ent, va("^3Entity modified."));
			removeSpawnstring(backupSpawn);
		}

		if (wasBoundingBoxDisplayed) {
			targ->Lmd.displayBounds = qtrue;
			targ->think = Lmd_Bounds_Think;
			targ->nextthink = level.time;
		}
	}
	else if(Q_stricmp(arg, "deletekey") == 0){
		SpawnData_t *curSpawn, *backupSpawn;
		if(argc > 3){
			int i = atoi(arg2);
			if(i < MAX_GENTITIES)
				targ = &g_entities[i];
			else
				targ = Lmd_logic_entity(i - MAX_GENTITIES);
		}
		else
			targ = AimAnyTarget(ent, 2048);

		if(!targ || !targ->inuse || !targ->Lmd.spawnData){
			Disp(ent, "^3Invalid target.");
			return;
		}

		trap_Argv(2 + (argc > 3), arg, sizeof(arg));

		qboolean wasBoundingBoxDisplayed = targ->Lmd.displayBounds;
		curSpawn = targ->Lmd.spawnData;

		backupSpawn = cloneSpawnstring(curSpawn);
		Lmd_Entities_deleteSpawnstringKey(curSpawn, arg);

		if(spawnEntity(targ, curSpawn) == NULL){
			Disp(ent, "^3Entity failed to respawn, reverting...");
			if (spawnEntity(targ, backupSpawn) == NULL) {
				Disp(ent, "^1Failed to revert to old spawnstring, entity did not spawn.");
				return;
			}
			else
				removeSpawnstring(curSpawn);
		}
		else{
			Disp(ent, va("^3Entity modified."));
			removeSpawnstring(backupSpawn);
		}

		if (wasBoundingBoxDisplayed) {
			targ->Lmd.displayBounds = qtrue;
			targ->think = Lmd_Bounds_Think;
			targ->nextthink = level.time;
		}
	}
	else
		Disp(ent, "^3Invalid argument.");
}

BG_field_t *getField (const char *fieldName);
void getSpawnstringPairs(SpawnData_t *spawnData, KeyPairSet_t *set);
void dispEntityInfo (gentity_t *ent, gentity_t *tEnt){
	KeyPairSet_t set;
	int i;
	memset(&set, 0, sizeof(set));
	DispContiguous(ent, "^3---------------------------------");
	DispContiguous(ent, va("%s ^3(^2%i^3)", FormattedEntString(tEnt), tEnt->s.number));
	if(tEnt->Lmd.group)
		DispContiguous(ent, va("^5group: ^2%s", tEnt->Lmd.group));
	DispContiguous(ent, va("^3constant origin: (^2%s^3)", vtos2(tEnt->s.origin)));
	DispContiguous(ent, va("^3current origin: (^2%s^3)", vtos2(tEnt->r.currentOrigin)));

	if (tEnt->model)
		DispContiguous(ent, va("^3model: ^2%s", tEnt->model));

	if (tEnt->model && tEnt->model[0] == '*')
		DispContiguous(ent, va("^3absmin: (^2%s^3)\n^3absmax: (^2%s^3)", vtos2(tEnt->r.absmin), vtos2(tEnt->r.absmax)));
	else if(VectorCompare(tEnt->r.mins, vec3_origin) == qfalse || VectorCompare(tEnt->r.maxs, vec3_origin) == qfalse)
		DispContiguous(ent, va("^3mins: (^2%s^3)\n^3maxs: (^2%s^3)", vtos2(tEnt->r.mins), vtos2(tEnt->r.maxs)));
	
	if(tEnt->Lmd.spawnData) {
		getSpawnstringPairs(tEnt->Lmd.spawnData, &set);
		for(i = 0; i < set.count; i++) {
			if(Q_stricmp(set.pairs[i].key, "group") == 0 || Q_stricmp(set.pairs[i].key, "origin") == 0 ||
				Q_stricmp(set.pairs[i].key, "classname") == 0 || Q_stricmp(set.pairs[i].key, "model") == 0 ||
				Q_stricmp(set.pairs[i].key, "mins") == 0 || Q_stricmp(set.pairs[i].key, "maxs") == 0) {
					continue;
			}
			DispContiguous(ent, va("^3%s: ^2%s", set.pairs[i].key, set.pairs[i].value));

			if(Q_stricmpn(set.pairs[i].key, "target", 6) == 0 && (set.pairs[i].key[6] == 0 || (set.pairs[i].key[6] >= '1' && set.pairs[i].key[6] <= '6'))) {
				gentity_t *t = NULL;
				while ( (t = G_Find (t, FOFS(targetname), set.pairs[i].value)) != NULL ) {
					DispContiguous(ent, va("   ^2%i: %s", t->s.number, FormattedEntString(t)));
				}
			}
			else if(Q_stricmp(set.pairs[i].key, "targetname") == 0) {
				gentity_t *t = NULL;
				qboolean hasOne = qfalse;

				//t1
				while ( (t = G_Find (t, FOFS(target), set.pairs[i].value)) != NULL ) {
					if(hasOne == qfalse) {
						hasOne = qtrue;
						DispContiguous(ent, va("   ^3target"));
					}
					DispContiguous(ent, va("   ^2%i: %s", t->s.number, FormattedEntString(t)));
				}

				//t2
				hasOne = qfalse;
				while ( (t = G_Find (t, FOFS(target2), set.pairs[i].value)) != NULL ) {
					if(hasOne == qfalse) {
						hasOne = qtrue;
						DispContiguous(ent, va("   ^3target2"));
					}
					DispContiguous(ent, va("   ^2%i: %s", t->s.number, FormattedEntString(t)));
				}

				//t3
				hasOne = qfalse;
				while ( (t = G_Find (t, FOFS(target3), set.pairs[i].value)) != NULL ) {
					if(hasOne == qfalse) {
						hasOne = qtrue;
						DispContiguous(ent, va("   ^3target3"));
					}
					DispContiguous(ent, va("   ^2%i: %s", t->s.number, FormattedEntString(t)));
				}

				//t4
				hasOne = qfalse;
				while ( (t = G_Find (t, FOFS(target4), set.pairs[i].value)) != NULL ) {
					if(hasOne == qfalse) {
						hasOne = qtrue;
						DispContiguous(ent, va("   ^3target4"));
					}
					DispContiguous(ent, va("   ^2%i: %s", t->s.number, FormattedEntString(t)));
				}

				//t5
				hasOne = qfalse;
				while ( (t = G_Find (t, FOFS(target5), set.pairs[i].value)) != NULL ) {
					if(hasOne == qfalse) {
						hasOne = qtrue;
						DispContiguous(ent, va("   ^3target5"));
					}
					DispContiguous(ent, va("   ^2%i: %s", t->s.number, FormattedEntString(t)));
				}

				//t6
				hasOne = qfalse;
				while ( (t = G_Find (t, FOFS(target6), set.pairs[i].value)) != NULL ) {
					if(hasOne == qfalse) {
						hasOne = qtrue;
						DispContiguous(ent, va("   ^3target6"));
					}
					DispContiguous(ent, va("   ^2%i: %s", t->s.number, FormattedEntString(t)));
				}
			}
		}
	}

	DispContiguous(ent, NULL);
}

void Cmd_Trace_f (gentity_t *ent, int iArg) {
	gentity_t *tEnt;
	if (trap_Argc() > 1) {
		int entnr;
		char arg[16];
		trap_Argv(1, arg, sizeof(arg));
		entnr = atoi(arg);
		if(entnr == 0 && !(arg[0] == '0' && arg[1] == 0)) {
			Disp(ent, "^3Invalid entity number");
			return;
		}
		tEnt = GetEnt(entnr);
	}
	else {
		if (!ent)
			return;

		G_PlayEffectID(G_EffectIndex("env/hevil_bolt"), ent->client->renderInfo.eyePoint, ent->client->ps.viewangles);
		tEnt = AimAnyTarget(ent, 8192);
	}
	if (!tEnt || !tEnt->inuse){
		Disp(ent, "^3Entity not found");
		return;
	}
	dispEntityInfo(ent,tEnt);
}

void Cmd_UseEnt_f(gentity_t *ent, int iArg){
	gentity_t *tEnt;
	char arg[MAX_STRING_CHARS] = "";
	if(trap_Argc() > 1){
		trap_Argv(1, arg, sizeof(arg));
		if(iArg == 0){
			int num = atoi(arg);
			if(num < MAX_GENTITIES)
				tEnt = &g_entities[num];
			else
				tEnt = Lmd_logic_entity(num - MAX_GENTITIES);
			if(!tEnt || !tEnt->inuse){
				Disp(ent, "^3Invalid entity.");
				return;
			}
		}
	}
	else{
		G_PlayEffectID(G_EffectIndex("env/hevil_bolt"),ent->client->renderInfo.eyePoint, ent->client->ps.viewangles);
		tEnt = AimAnyTarget(ent, 8192);
		if(!tEnt || !tEnt->inuse){
			Disp(ent, "^3Invalid entity.");
			return;
		}
		if(iArg == 1){
			if(!tEnt->targetname){
				Disp(ent, "^3This entity has no targetname.");
				return;
			}
			Q_strncpyz(arg, tEnt->targetname, sizeof(arg));
		}
	}
	if(iArg == 0) {
		GlobalUse(tEnt, ent, ent);
		Disp(ent, "^2Entity used.");
	}
	else if(iArg == 1) {
		G_UseTargets2(ent, ent, arg);
		Disp(ent, "^2Targetname triggered.");
	}
}

int Lmd_Entities_getSpawnstringKeyIndex(SpawnData_t* ent, char* key);
char* Lmd_Entities_getSpawnstringKeyValue(SpawnData_t* ent, int index);

qboolean EntityMatches(gentity_t* ent, char* key, char* value, qboolean partial)
{
    int index = Lmd_Entities_getSpawnstringKeyIndex(ent->Lmd.spawnData, key);
    char* keyValue;
    if (index == -1)
        return qfalse;
    keyValue = Lmd_Entities_getSpawnstringKeyValue(ent->Lmd.spawnData, index);

    if (partial)
        return (strstr(keyValue, value) != NULL);
    else
        return (Q_stricmp(keyValue, value) == 0);
}

typedef struct Cmd_DelAll_Confirm_Data_s
{
    char key[MAX_STRING_CHARS];
    char value[MAX_STRING_CHARS];
} Cmd_DelAll_Confirm_Data_t;


void Cmd_DelAll_Confirm(gentity_t* ent, void* dataptr)
{
    Cmd_DelAll_Confirm_Data_t* data = (Cmd_DelAll_Confirm_Data_t*)dataptr;
    gentity_t* trace = NULL;
    int c = 0;
    while (trace = IterateEnts(trace))
    {
        if (!trace->Lmd.spawnData)
            continue;
        if (!EntityMatches(trace, data->key, data->value, qfalse))
            continue;
        G_FreeEntity(trace);
        c++;
    }
    Disp(ent, va("^2%i^3 entities with  %s ^2%s deleted.", c, data->key, data->value));
}

void Cmd_DelAll_f(gentity_t* ent, int iArg)
{
    int argc = trap_Argc();
    char key[MAX_STRING_CHARS], *value;
    gentity_t* trace = NULL;
    int argbase = 0;
    int c = 0;

    if (argc >= 3)
    {
        trap_Argv(1, key, sizeof(key));
        value = ConcatArgs(2);
    }
    else if (argc == 2)
    {
        Q_strncpyz(key, "classname", sizeof(key));
        value = ConcatArgs(1);
    }
    else
    {
        Disp(ent, "^3Usage: DelAll <field or classname> [value]");
        return;
    }


    while (trace = IterateEnts(trace))
    {
        if (!trace->Lmd.spawnData)
            continue;
        if (!EntityMatches(trace, key, value, qfalse))
            continue;
        c++;
    }

    Cmd_DelAll_Confirm_Data_t* data = (Cmd_DelAll_Confirm_Data_t*)G_Alloc(sizeof(Cmd_DelAll_Confirm_Data_t));
    Q_strncpyz(data->key, key, sizeof(data->key));
    Q_strncpyz(data->value, value, sizeof(data->value));
    if (Confirm_Set(ent, Cmd_DelAll_Confirm, data))
    {
        Disp(ent, va("^3This command will delete ^2%i^3 entities with %s ^2%s.\n^3This command cannot be undone!", c,
                     key, value));
    }
}

void Cmd_Delent_f(gentity_t* ent, int iArg)
{
    int index;
    gentity_t* targ = NULL;
    Action_t* action;
    char* spawnstring;
    int sLen;

    if (trap_Argc() < 2)
    {
        targ = AimAnyTarget(ent, 8192);
        if (!targ)
        {
            Disp(ent, "^3You are not aiming at a target.");
            return;
        }
    }
    else
    {
        index = atoi(ConcatArgs(1));
        targ = GetEnt(index);
        if (!targ || !targ->inuse)
        {
            Disp(ent, "^3Invalid entity.");
            return;
        }
        if (targ->s.number == ENTITYNUM_WORLD)
        {
            Disp(ent, "^3You cannot delete the worldspawn.");
            return;
        }
    }
    if (!targ->Lmd.spawnData)
    {
        Disp(ent, "^3You cannot delete this entity.");
        return;
    }

    sLen = Lmd_Entites_GetSpawnstringLen(targ->Lmd.spawnData);
    spawnstring = (char*)G_Alloc(sLen);
    Lmd_Entities_getSpawnstring(targ->Lmd.spawnData, spawnstring, sLen);
    char delInfo[128];
    Com_sprintf(delInfo, sizeof(delInfo), "^2%s ^7(^3%d^7)", FormattedEntString(targ), targ->s.number);
    Disp(ent, va("^3Deleting entity: %s", delInfo));
    Disp(ent, va("^3Spawnstring:\n^2%s", spawnstring));
    if ((action = PlayerActions_Add(ent, "undodelete", "View the spawnstring of, or respawn, the last deleted entity",
                                    Action_UndoDelete, qtrue)))
    {
        action->strArgs[0] = spawnstring;
        action->iArgs[0] = Lmd_Entities_IsSaveable(targ);
    }
    G_FreeEntity(targ);
}

void Cmd_DisableEnt_f(gentity_t* ent, int iArg)
{
    gentity_t* targ = NULL;
    if (trap_Argc() >= 2)
    {
        char buf[MAX_STRING_CHARS];
        int itarg;
        trap_Argv(1, buf, sizeof(buf));
        itarg = atoi(buf);
        targ = GetEnt(itarg);
    }
    else
    {
        G_PlayEffectID(G_EffectIndex(STANDARD_BEAM), ent->client->renderInfo.eyePoint, ent->client->ps.viewangles);
        targ = AimAnyTarget(ent, 8024);
    }

    if (!targ || !targ->inuse)
    {
        Disp(ent, "^3Invalid entity.");
        return;
    }

    if (targ->flags & FL_TEAMSLAVE)
    {
        targ = targ->teammaster;

        if (!targ || !targ->inuse)
        {
            Disp(ent, "^3Entity is a part of a team bound to an invalid leader.");
            return;
        }
        Disp(ent, "^3Effect entity auto redirected to team master.");
    }
    if (targ->flags & FL_INACTIVE)
    {
        targ->flags &= ~FL_INACTIVE;
        Disp(ent, "^3Target ^5activated");
    }
    else
    {
        targ->flags |= FL_INACTIVE;
        Disp(ent, "^3target ^6deactivated");
    }
}

void Cmd_Entitylist_f(gentity_t* ent, int iArg)
{
    int argc = trap_Argc();
    int ofs = 0, show = 25, c = 0;
    char key[MAX_STRING_CHARS], *value;
    gentity_t* trace = NULL;
    int argbase = 0;
    qboolean partial = qfalse;
    //entitylist [offset] <key/classname> <value>

    trap_Argv(1, key, sizeof(key));
    if (atoi(key) != 0 || (key[0] == '0' && key[1] == 0))
    {
        ofs = atoi(key) - 1;
        if (ofs < 0)
            ofs = 0;
        argbase = 1;
        key[0] = 0;
    }

    if (argc >= 3 + argbase)
    {
        trap_Argv(1 + argbase, key, sizeof(key));
        value = ConcatArgs(2 + argbase);
    }
    else if (argc == 2 + argbase)
    {
        Q_strncpyz(key, "classname", sizeof(key));
        value = ConcatArgs(1 + argbase);
        partial = qtrue;
    }
    DispContiguous(ent, "^3-------------------------------------");

    show = -ofs;
    while (trace = IterateEnts(trace))
    {
        if (key[0] && (!trace->Lmd.spawnData || !EntityMatches(trace, key, value, qtrue)))
            continue;

        c++;

        show++;
        if (show <= 0)
            continue;
        if (show < 25)
        {
            DispContiguous(ent, va("^2%-7i %s", trace->s.number, FormattedEntString(trace)));
        }
    }
    if (ofs > c)
        DispContiguous(ent, va("^3Offset out of range.  Only ^2%i^3 entities were found.", c));
    else
    {
        int end = ofs + 25;
        if (end > c)
            end = c;
        DispContiguous(ent, va("^3Displaying ^2%i^3 to ^2%i^3 of ^2%i^3 entities found.", ofs + 1, end, c));
    }
    DispContiguous(ent, "^3-------------------------------------");
    DispContiguous(ent, NULL);
}

//Ufo:
void Cmd_GetEnt_f(gentity_t* ent, int iArg)
{
    if (trap_Argc() < 2)
    {
        Disp(ent, "^3No entity specified.");
        return;
    }
    gentity_t* targ = GetEnt(atoi(ConcatArgs(1)));
    vec3_t dest, temp;

    if (!targ || !targ->inuse || (!iArg && !targ->Lmd.spawnData) || (iArg && targ->client))
    {
        Disp(ent, "^3Invalid entity.");
        return;
    }

    if (targ->r.bmodel)
    {
        VectorAverage(targ->r.mins, targ->r.maxs, dest);
        if (targ->r.currentAngles[0] || targ->r.currentAngles[1] || targ->r.currentAngles[2])
        {
            VectorCopy(dest, temp);
            RotatePointAroundVector(dest, axisDefault[0], temp, targ->r.currentAngles[2]);
            VectorCopy(dest, temp);
            RotatePointAroundVector(dest, axisDefault[1], temp, targ->r.currentAngles[0]);
            VectorCopy(dest, temp);
            RotatePointAroundVector(dest, axisDefault[2], temp, targ->r.currentAngles[1]);
        }
        if (iArg)
            VectorAdd(dest, targ->r.currentOrigin, dest);
        else
            VectorSubtract(ent->r.currentOrigin, dest, dest);
    }
    else
        VectorCopy(iArg ? targ->r.currentOrigin : ent->r.currentOrigin, dest);
    if (iArg)
    {
        ent->client->ps.eFlags ^= EF_TELEPORT_BIT;
        VectorCopy(dest, ent->client->ps.origin);
        Disp(ent, "^2Teleported to entity.");
    }
    else
    {
        qboolean wasBoundingBoxDisplayed = targ->Lmd.displayBounds;
        Lmd_Entities_setSpawnstringKey(targ->Lmd.spawnData, "origin",
                                       va("%d %d %d", (int)dest[0], (int)dest[1], (int)dest[2]));
        spawnEntity(targ, targ->Lmd.spawnData);
        Disp(ent, "^2Entity moved to your position.");
        if (wasBoundingBoxDisplayed)
        {
            targ->Lmd.displayBounds = qtrue;
            targ->think = Lmd_Bounds_Think;
            targ->nextthink = level.time;
        }
    }
}

void Cmd_PlayerSpawnPoint_f(gentity_t* ent, int iArg)
{
    team_t team;
    gentity_t* spawnent;
    //vec3_t          origin;
    char* classname;
    SpawnData_t* spawnData;

    team = (team_t)iArg;

    if (!ent || !ent->client)
    {
        return;
    }


    switch (team)
    {
    case TEAM_RED:
        classname = "info_player_start_red";
        break;
    case TEAM_BLUE:
        classname = "info_player_start_blue";
        break;
    case TEAM_JAILED:
        classname = "info_player_jail";
        break;
    default:
        classname = "info_player_deathmatch";
        break;
    }
    spawnData = ParseEntitySpawnstring(va("classname,%s,origin,%.0f %.0f %.0f,angle,%.0f,", classname,
                                          ent->r.currentOrigin[0],
                                          ent->r.currentOrigin[1], ent->r.currentOrigin[2],
                                          ent->client->ps.viewangles[YAW]));
    if (Auths_PlayerHasAuthFlag(ent, AUTH_SAVEPLACED))
        Lmd_Entities_SetSaveable(spawnData, qtrue);
    spawnent = spawnEntityFromData(spawnData);
    if (!spawnent)
        Disp(ent, "^3Failed to spawn entity.");
    else
    {
        Disp(ent, va("^3Entity ^2%i^3 has been spawned.", spawnent->s.number));
    }
}


int Lmd_logic_count();

void Cmd_Mapents_f(gentity_t* ent, int iArg)
{
    int argc = trap_Argc();
    char arg[MAX_STRING_CHARS];
    if (argc == 1)
    {
        Disp(
            ent,
            "^3Usage: mapents {^2count^3} {^2snapshot^3} {^2save ^5<name>^3} {^2load ^5<name>^3 ^5[^3'nodefaults'^5]^3}");
        return;
    }
    trap_Argv(1, arg, sizeof(arg));
    if (Q_stricmp(arg, "count") == 0)
    {
        int t = 0, c = 0, i = 0;
        int l = 0, lc = Lmd_logic_count();
        for (i = 0; i < MAX_GENTITIES; i++)
        {
            if (g_entities[i].inuse)
            {
                if (g_entities[i].Lmd.spawnData)
                    c++;
                t++;
            }
        }
        gentity_t* scan;
        for (i = 0; i < lc; i++)
        {
            scan = Lmd_logic_entity(i);
            if (scan->inuse)
            {
                if (scan->Lmd.spawnData)
                {
                    c++;
                }
                l++;
                t++;
            }
        }

        Disp(ent, va(
                 "^3Total entities: ^2%i\n"
                 "^6Editable entities: ^2%i\n"
                 "^5Game entities: ^2%i\n"
                 "^2Logic entities: ^2%i", t, c, t - l, l));
        return;
    }
    else if (Q_stricmp(arg, "save") == 0)
    {
        if (argc < 2)
        {
            Disp(ent, "^3Expected name of entities to save.");
            return;
        }
        trap_Argv(2, arg, sizeof(arg));
        SaveEntitiesData(arg);
        Disp(ent, "^2Entities saved.");
    }
    else if (Q_stricmp(arg, "load") == 0)
    {
        qboolean nodefaults = qfalse;
        if (argc < 2)
        {
            Disp(ent, "^3Expected name of entities to load.");
            return;
        }
        trap_Argv(3, arg, sizeof(arg));
        if (Q_stricmp(arg, "nodefaults") == 0)
            nodefaults = qtrue;
        trap_Argv(2, arg, sizeof(arg));
        LoadEntitiesData(arg, nodefaults);
        return;
    }
    else if (!Q_stricmp(arg, "snapshot")
        || !Q_stricmp(arg, "snap"))
    {
        int sequence = 0;
        int count = 0;

        while (trap_BotGetSnapshotEntity(ent->s.number, sequence) != -1)
        {
            sequence++;
            count++;
        }

        Disp(ent, va("^3Snapshot Entities: ^2%i", count));
    }

    else
        Disp(ent, "^3Unknown argument.  For usage information, enter the command without args.");
}

    void Cmd_Nearby_f(gentity_t* ent, int iArg)
    {
        int i = 128, count;
        char buf[MAX_STRING_CHARS];
        gentity_t* ent_list[MAX_GENTITIES];
        char search[MAX_STRING_CHARS] = "";
        trap_Argv(1, buf, sizeof(buf));
        if (buf[0])
            i = atoi(buf);
        if (trap_Argc() >= 3)
        {
            trap_Argv(2, search, sizeof(search));
        }

        count = G_RadiusList(ent->r.currentOrigin, i, ent, qfalse, ent_list, qtrue);
        for (i = 0; i < count; i++)
        {
            if (search[0])
            {
                if (Q_stricmpn(ent_list[i]->classname, search, strlen(search)) != 0)
                    continue;
            }
            Disp(ent, va("^2%i ^3(^2%.00f %.00f %.00f^3) ^3%s\n\"", ent_list[i]->s.number, ent_list[i]->s.origin[0],
                         ent_list[i]->s.origin[1],
                         ent_list[i]->s.origin[2], ent_list[i]->classname));
            if (i == 75)
            {
                Disp(ent, "^3Only the first 75 entites were displayed.");
                break;
            }
        }
    }

void Cmd_NewMap_f(gentity_t* ent, int iArg) 
{
  char mapName[MAX_QPATH];
	char suffix[MAX_QPATH];
	qboolean hasSuffix = qfalse;

	if (trap_Argc() < 2) {
		return;
	}

	trap_Argv(1, mapName, sizeof(mapName));
	if (!mapName[0]) {
		return;
	}

	if (trap_Argc() >= 3) {
		trap_Argv(2, suffix, sizeof(suffix));
		if (suffix[0]) {
			if (!Lmd_Data_IsCleanPath(suffix)) {
				if (ent) {
					Disp(ent, "^3Invalid entity suffix.");
				} else {
					Com_Printf("Invalid entity suffix.\n");
				}
				return;
			}
			trap_Cvar_Set("lmd_mapEntitySuffix", suffix);
			hasSuffix = qtrue;
		}
	}

	if (!hasSuffix) {
		trap_Cvar_Set("lmd_mapEntitySuffix", "");
	}

	trap_SendConsoleCommand(EXEC_APPEND, va("map \"%s\"\n", mapName));
}

    void DiagnoseBuilding(gentity_t* ent)
    {
        //TODO: check for number of random spots, random spots in solids, suchlike.
    }

    void Cmd_Entityinfo_t(gentity_t* ent, int iArg);

    // grab [entity number]
    void Lmdp_Grabbed_Think(gentity_t* self)
    {
        gentity_t* player = self->activator;

        // validate player
        if (!player || !player->client || player->client->pers.connected != CON_CONNECTED)
        {
            // restore entity thinking
            if (self->Lmd.oldThink)
            {
                self->think = self->Lmd.oldThink;
                self->nextthink = level.time + self->Lmd.oldNextthink;
            }
            else if (self->Lmd.displayBounds)
            {
                self->think = Lmd_Bounds_Think;
                self->nextthink = level.time;
            }

            // reset dummy fields
            self->Lmd.oldThink = 0;
            self->Lmd.oldNextthink = 0;
        }
        else
        {
            // move with player
            vec3_t end, start, forward;
            trace_t tr;
            int playerNum = player->s.number;

            if (self->Lmd.grabOrigin)
            {
                AngleVectors(player->client->ps.viewangles, forward, NULL, NULL);
                VectorCopy(player->client->ps.origin, start);
                start[2] += player->client->ps.viewheight;
                VectorMA(start, player->client->Lmd.grabOffset, forward, end);
                trap_Trace(&tr, start, NULL, NULL, end, playerNum, MASK_SHOT);
                VectorAdd(end, self->Lmd.grabOffset, end);
                SnapVector(end);

                if (self->r.bmodel)
                {
                    vec3_t dest, temp;
                    VectorAverage(self->r.mins, self->r.maxs, dest);
                    VectorCopy(dest, temp);
                    RotatePointAroundVector(dest, axisDefault[0], temp, self->r.currentAngles[2]);
                    VectorCopy(dest, temp);
                    RotatePointAroundVector(dest, axisDefault[1], temp, self->r.currentAngles[0]);
                    VectorCopy(dest, temp);
                    RotatePointAroundVector(dest, axisDefault[2], temp, self->r.currentAngles[1]);
                    VectorSubtract(end, dest, dest);

                    VectorCopy(dest, self->s.pos.trBase);
                    self->s.pos.trTime = player->s.pos.trTime;
                    self->s.pos.trDuration = level.time + Q3_INFINITE;
                    self->s.pos.trType = TR_STATIONARY;

                    VectorCopy(dest, self->r.currentOrigin);
                }
                else
                {
                    VectorCopy(end, self->s.pos.trBase);
                    self->s.pos.trTime = player->s.pos.trTime;
                    self->s.pos.trDuration = level.time + Q3_INFINITE;

                    VectorCopy(player->s.pos.trDelta, self->s.pos.trDelta);
                    SnapVector(self->s.pos.trDelta);

                    VectorCopy(end, self->r.currentOrigin);
                    trap_LinkEntity(self);
                }
            }

            if (self->Lmd.grabX || self->Lmd.grabY || self->Lmd.grabZ)
            {
                vec3_t initialPosition;
                if (self->r.bmodel)
                {
                    // The origin of "func" entities is not located inside the entity
                    vec3_t temp;
                    VectorAverage(self->r.mins, self->r.maxs, initialPosition);
                    VectorCopy(initialPosition, temp);
                    RotatePointAroundVector(initialPosition, axisDefault[0], temp, self->r.currentAngles[2]);
                    VectorCopy(initialPosition, temp);
                    RotatePointAroundVector(initialPosition, axisDefault[1], temp, self->r.currentAngles[0]);
                    VectorCopy(initialPosition, temp);
                    RotatePointAroundVector(initialPosition, axisDefault[2], temp, self->r.currentAngles[1]);
                }

                vec_t newAngle = 0;

                // Rotating an entity along any axis is done by rotating the player character horizontally (right/left)
                newAngle = player->client->ps.viewangles[1] * 4; // X4 muliplier (otherwise, it's too slow)
                // Snap to axes:
                while (newAngle < 0.0)
                {
                    newAngle += 360.0;
                }
                while (newAngle > 360.0)
                {
                    newAngle -= 360.0;
                }
                if (newAngle <= 8.0 || newAngle >= 352.0)
                {
                    newAngle = 0.0;
                }
                else if (newAngle >= 82.0 && newAngle <= 98.0)
                {
                    newAngle = 90.0;
                }
                else if (newAngle >= 172.0 && newAngle <= 188.0)
                {
                    newAngle = 180.0;
                }
                else if (newAngle >= 262.0 && newAngle <= 278.0)
                {
                    newAngle = 270.0;
                }

                if (self->Lmd.grabX)
                {
                    self->r.currentAngles[2] = self->s.angles[2] = self->s.apos.trBase[2] = newAngle;
                }

                if (self->Lmd.grabY)
                {
                    self->r.currentAngles[0] = self->s.angles[0] = self->s.apos.trBase[0] = newAngle;
                }

                if (self->Lmd.grabZ)
                {
                    self->r.currentAngles[1] = self->s.angles[1] = self->s.apos.trBase[1] = newAngle;
                }

                if (self->r.bmodel)
                {
                    // The origin of "func" entities is not located inside the entity
                    vec3_t newPosition, temp;
                    VectorAverage(self->r.mins, self->r.maxs, newPosition);
                    VectorCopy(newPosition, temp);
                    RotatePointAroundVector(newPosition, axisDefault[0], temp, self->r.currentAngles[2]);
                    VectorCopy(newPosition, temp);
                    RotatePointAroundVector(newPosition, axisDefault[1], temp, self->r.currentAngles[0]);
                    VectorCopy(newPosition, temp);
                    RotatePointAroundVector(newPosition, axisDefault[2], temp, self->r.currentAngles[1]);

                    vec3_t positionOffset;
                    VectorSubtract(initialPosition, newPosition, positionOffset);

                    VectorAdd(positionOffset, self->s.pos.trBase, self->s.pos.trBase);
                    self->s.pos.trTime = player->s.pos.trTime;
                    self->s.pos.trDuration = level.time + Q3_INFINITE;
                    self->s.pos.trType = TR_STATIONARY;

                    VectorAdd(positionOffset, self->r.currentOrigin, self->r.currentOrigin);
                }
            }

            if (self->Lmd.displayBounds)
            {
                Lmd_Display_Bounds(self, 1000); // Don't set "lineTime" too low, or the lines will blink / be invisible
            }
        }

        self->nextthink = level.time + 10;
    }

    qboolean Lmdp_EditEntity(gentity_t* ent)
    {
        SpawnData_t* spawnData = NULL;
        SpawnData_t* backupData = NULL;
        gentity_t* newEnt = NULL;

        if (ent == NULL)
            return qfalse; // invalid entity or key/value

        qboolean wereBoundsDisplayed = ent->Lmd.displayBounds;
        spawnData = ent->Lmd.spawnData; // get entity's spawn data
        backupData = cloneSpawnstring(spawnData); // backup ent data
        newEnt = spawnEntity(ent, spawnData); // respawn ent

        if (newEnt == NULL)
        {
            // modified entity failed to spawn, use backup
            newEnt = spawnEntity(newEnt, backupData);
            if (newEnt == NULL)
            {
                G_FreeEntity(newEnt); // backup failed
                // TODO: print warning, entity was completely lost
                return qfalse;
            }
            if (wereBoundsDisplayed)
            {
                newEnt->Lmd.displayBounds = qtrue;
                newEnt->think = Lmd_Bounds_Think;
                newEnt->nextthink = level.time;
            }
            return qfalse;
        }

        if (wereBoundsDisplayed)
        {
            newEnt->Lmd.displayBounds = qtrue;
            newEnt->think = Lmd_Bounds_Think;
            newEnt->nextthink = level.time;
        }

        removeSpawnstring(backupData); // free backup spawndata
        return qtrue;
    }

    int Lmdp_Grabbed_Set(gentity_t* player, gentity_t* ent, int mode, qboolean msg, vec3_t offset, qboolean pickup)
    {
        // Set the initial distance between the grabbed entity and the player
        // (Can be increased with /GrabOffsetIncrease and decreased with /GrabOffsetDecrease)

        vec3_t initialPosition;
        if (ent->r.bmodel)
        {
            // The origin of "func" entities is not located inside the entity
            vec3_t temp;
            VectorAverage(ent->r.mins, ent->r.maxs, initialPosition);
            VectorCopy(initialPosition, temp);
            RotatePointAroundVector(initialPosition, axisDefault[0], temp, ent->r.currentAngles[2]);
            VectorCopy(initialPosition, temp);
            RotatePointAroundVector(initialPosition, axisDefault[1], temp, ent->r.currentAngles[0]);
            VectorCopy(initialPosition, temp);
            RotatePointAroundVector(initialPosition, axisDefault[2], temp, ent->r.currentAngles[1]);
            VectorAdd(ent->r.currentOrigin, initialPosition, initialPosition);
        }
        else
        {
            // Regular entity (not a "func")
            VectorCopy(ent->r.currentOrigin, initialPosition);
        }

        vec3_t originDiff;
        VectorSubtract(initialPosition, player->r.currentOrigin, originDiff);
        float distance = VectorLength(originDiff);
        player->client->Lmd.grabOffset = distance;

        if (ent->think == Lmdp_Grabbed_Think)
        {
            // restore entity thinking
            if (ent->Lmd.oldThink)
            {
                ent->think = ent->Lmd.oldThink;
                ent->nextthink = level.time + ent->Lmd.oldNextthink;
            }
            else if (ent->Lmd.displayBounds)
            {
                ent->think = Lmd_Bounds_Think;
                ent->nextthink = level.time;
            }
            else
            {
                ent->think = NULL;
                ent->nextthink = 0;
            }

            // reset dummy fields
            ent->Lmd.oldThink = 0;
            ent->Lmd.oldNextthink = 0;

            if (ent->Lmd.grabOrigin)
            {
                G_SetOrigin(ent, ent->r.currentOrigin);

                Lmd_Entities_setSpawnstringKey(ent->Lmd.spawnData, "origin",
                                               va("%i %i %i",
                                                  (int)ent->r.currentOrigin[0],
                                                  (int)ent->r.currentOrigin[1],
                                                  (int)ent->r.currentOrigin[2]));
            }

            if (ent->Lmd.grabX || ent->Lmd.grabY || ent->Lmd.grabZ)
            {
                Lmd_Entities_deleteSpawnstringKey(ent->Lmd.spawnData, "angle");
                Lmd_Entities_setSpawnstringKey(ent->Lmd.spawnData, "angles",
                                               va("%i %i %i",
                                                  (int)ent->r.currentAngles[0],
                                                  (int)ent->r.currentAngles[1],
                                                  (int)ent->r.currentAngles[2]));

                if (ent->r.bmodel)
                {
                    G_SetOrigin(ent, ent->r.currentOrigin);
                    Lmd_Entities_setSpawnstringKey(ent->Lmd.spawnData, "origin",
                                                   va("%i %i %i",
                                                      (int)ent->r.currentOrigin[0],
                                                      (int)ent->r.currentOrigin[1],
                                                      (int)ent->r.currentOrigin[2]));
                }
            }

            player->client->Lmd.grabbing--;

            if (!Lmdp_EditEntity(ent))
            {
                Disp(player, "^1Entity failed to respawn.");
                return 0;
            }
            ent->Lmd.grabOrigin = qfalse;
            ent->Lmd.grabX = qfalse;
            ent->Lmd.grabY = qfalse;
            ent->Lmd.grabZ = qfalse;
            ent->s.eFlags &= ~EF_CLIENTSMOOTH;
            VectorClear(ent->Lmd.grabOffset);


            if (msg)
                Disp(player, "^3Entity ^2%i ^3dropped.", ent->s.number);
        }
        else if (pickup == qtrue)
        {
            // backup entity think fields
            if (ent->think)
            {
                ent->Lmd.oldThink = ent->think;
                ent->Lmd.oldNextthink = ent->nextthink - level.time;
            }
            // set activator
            ent->activator = player;

            // initialize s.pos
            VectorCopy(ent->s.pos.trBase, player->s.pos.trBase);
            ent->s.pos.trTime = level.time;
            ent->s.pos.trType = TR_LINEAR_STOP;

            // initialize r.currentAngles
            VectorCopy(ent->s.angles, ent->r.currentAngles);

            // replace entity thinking
            ent->think = Lmdp_Grabbed_Think;
            ent->nextthink = level.time;
            if (mode == GRAB_ORIGIN)
            {
                ent->Lmd.grabOrigin = qtrue;
            }
            else if (mode == GRAB_X)
            {
                ent->Lmd.grabX = qtrue;
            }
            else if (mode == GRAB_Y)
            {
                ent->Lmd.grabY = qtrue;
            }
            else if (mode == GRAB_Z)
            {
                ent->Lmd.grabZ = qtrue;
            }
            ent->s.eFlags |= EF_CLIENTSMOOTH;
            VectorCopy(offset, ent->Lmd.grabOffset);

            player->client->Lmd.grabbing++;

            if (msg)
                Disp(player, "^3Entity ^2%i ^3grabbed.", ent->s.number);
        }

        return 1;
    }

    void Cmd_Grab_f(gentity_t* ent, int iArg)
    {
        gentity_t* targ;

        if (trap_Argc() > 1)
        {
            char arg[MAX_STRING_CHARS] = "";
            trap_Argv(1, arg, sizeof(arg));
            targ = GetEnt(atoi(arg));
        }
        else
            targ = AimAnyTarget(ent, 8192);

        if (!targ || !targ->inuse)
        {
            Disp(ent, "^3Entity not found");
            return;
        }
        if (targ)
        {
            if (targ->Lmd.spawnData == NULL)
                Disp(ent, "^1Only custom entities can be grabbed.");
            else
                Lmdp_Grabbed_Set(ent, targ, iArg, qtrue, vec3_origin/*offs*/, qtrue);
        }
    }

    void Cmd_CancelGrab_f(gentity_t* ent)
    {
        gentity_t* targ;

        if (trap_Argc() > 1)
        {
            char arg[MAX_STRING_CHARS] = "";
            trap_Argv(1, arg, sizeof(arg));
            targ = GetEnt(atoi(arg));
        }
        else
        {
            targ = AimAnyTarget(ent, 8192);
        }

        if (!targ || !targ->inuse)
        {
            Disp(ent, "^3Entity not found");
            return;
        }

        if (targ->think != Lmdp_Grabbed_Think)
        {
            Disp(ent, "^3The selected entity is not currently being grabbed.");
            return;
        }

        // Restore entity "think"
        if (targ->Lmd.oldThink)
        {
            targ->think = targ->Lmd.oldThink;
            targ->nextthink = level.time + targ->Lmd.oldNextthink;
        }
        else if (targ->Lmd.displayBounds)
        {
            targ->think = Lmd_Bounds_Think;
            targ->nextthink = level.time;
        }
        else
        {
            targ->think = NULL;
            targ->nextthink = 0;
        }

        // Reset other fields
        targ->Lmd.oldThink = 0;
        targ->Lmd.oldNextthink = 0;
        targ->Lmd.grabOrigin = qfalse;
        targ->Lmd.grabX = qfalse;
        targ->Lmd.grabY = qfalse;
        targ->Lmd.grabZ = qfalse;
        targ->s.eFlags &= ~EF_CLIENTSMOOTH;
        VectorClear(targ->Lmd.grabOffset);

        ent->client->Lmd.grabbing--;

        if (!Lmdp_EditEntity(targ))
        {
            Disp(ent, "^1Entity failed to respawn.");
            return;
        }

        Disp(ent, "^3Entity ^2%i ^3dropped. Initial position restored.", targ->s.number);
    }

    // Increase the distance between the grabbed entity and the player
    void Cmd_GrabOffsetInc_f(gentity_t* player)
    {
        if (player->client->Lmd.grabOffset >= 10000)
        {
            player->client->Lmd.grabOffset = 10000;
        }
        else if (player->client->Lmd.grabOffset < 0)
        {
            player->client->Lmd.grabOffset = 0;
        }
        else
        {
            player->client->Lmd.grabOffset += 8;
        }
    }

    // Decrease the distance between the grabbed entity and the player
    void Cmd_GrabOffsetDec_f(gentity_t* player)
    {
        if (player->client->Lmd.grabOffset > 10000)
        {
            player->client->Lmd.grabOffset = 10000;
        }
        else if (player->client->Lmd.grabOffset <= 0)
        {
            player->client->Lmd.grabOffset = 0;
        }
        else
        {
            player->client->Lmd.grabOffset -= 8;
        }
    }

    typedef struct SpawnData_s
    {
        qboolean canSave;
        gentity_t* spawned;
        KeyPairSet_t keys;
    } SpawnData_t;

    void Lmdp_Clone(gentity_t* ent, gentity_t* targ, qboolean msg, vec3_t offset)
    {
        // get target entity
        if (targ->Lmd.spawnData)
        {
            SpawnData_t* spawnData = cloneSpawnstring(targ->Lmd.spawnData);

            if (spawnData)
            {
                gentity_t* newEnt = spawnEntity(NULL, spawnData);

                if (newEnt)
                {
                    newEnt->Lmd.spawnData->canSave = qtrue;
                    if (
                        !Lmdp_Grabbed_Set(ent, targ, GRAB_ORIGIN, qfalse, vec3_origin, qfalse)
                        || !Lmdp_Grabbed_Set(ent, newEnt, GRAB_ORIGIN, qfalse, offset, qtrue)
                    )
                    {
                        removeSpawnstring(spawnData);
                        G_FreeEntity(newEnt);
                    }
                    else if (msg)
                        Disp(ent, "^3Entity ^2%i ^3spawned.", newEnt->s.number);
                }
                else
                {
                    // TODO: maybe this is not necessary
                    removeSpawnstring(spawnData);
                    G_FreeEntity(newEnt);
                    Disp(ent, "^1Error trying to spawn clone.");
                }
            }
        }
        else
            Disp(ent, "^1Error trying to spawn clone.");
    }

    void Cmd_Clone_f(gentity_t* ent, int iArg)
    {
        gentity_t* targ;

        if (trap_Argc() > 1)
        {
            char arg[MAX_STRING_CHARS] = "";
            trap_Argv(1, arg, sizeof(arg));
            targ = GetEnt(atoi(arg));
        }
        else
            targ = AimAnyTarget(ent, 8192);
        if (!targ || !targ->inuse)
        {
            Disp(ent, "^3Entity not found");
            return;
        }

        if (targ)
            Lmdp_Clone(ent, targ, qtrue, vec3_origin);
    }

    // measure
    // a measuring command
    // todo: point to two entities to get their distance
    void Cmd_Measure_f(gentity_t* ent, int iArg)
    {
        trace_t tr;
        vec3_t end, start, forward;
        gclient_t* cl = ent->client;

        AngleVectors(cl->ps.viewangles, forward, NULL, NULL);
        VectorSet(start, cl->ps.origin[0], cl->ps.origin[1], cl->ps.origin[2] + cl->ps.viewheight);
        VectorMA(start, Q3_INFINITE, forward, end);
        trap_Trace(&tr, start, NULL, NULL, end, ent->s.number, MASK_SHOT);

        if (tr.fraction != 1)
        {
            //Lmdp_Player_t *player = Lmdp_Players + playerNum;
            if (VectorCompare(ent->client->Lmd.mark, vec3_origin))
            {
                VectorCopy(tr.endpos, ent->client->Lmd.mark);
                Disp(ent, "^3Marked location: (%i %i %i)",
                     (int)tr.endpos[0], (int)tr.endpos[1], (int)tr.endpos[2]);
            }
            else
            {
                VectorCopy(ent->client->Lmd.mark, start);
                start[0] = tr.endpos[0];
                VectorCopy(start, end);
                end[1] = tr.endpos[1];
                G_TestLine(ent->client->Lmd.mark, start, 0x00000ff, 5000); // x
                G_TestLine(start, end, SABER_GREEN, 5000); // y
                G_TestLine(end, tr.endpos, SABER_BLUE, 5000); // z
                G_TestLine(ent->client->Lmd.mark, tr.endpos, SABER_PURPLE, 5000);

                Disp(ent, "^3(%i %i %i)->(%i %i %i) ^6%i: ^1%i ^2%i ^4%i",
                     (int)ent->client->Lmd.mark[0], (int)ent->client->Lmd.mark[1], (int)ent->client->Lmd.mark[2],
                     (int)tr.endpos[0], (int)tr.endpos[1], (int)tr.endpos[2],
                     (int)Distance(ent->client->Lmd.mark, tr.endpos),
                     (int)(tr.endpos[0] - ent->client->Lmd.mark[0]),
                     (int)(tr.endpos[1] - ent->client->Lmd.mark[1]),
                     (int)(tr.endpos[2] - ent->client->Lmd.mark[2]));

                // clear mark
                memset(ent->client->Lmd.mark, NULL, sizeof(ent->client->Lmd.mark));
            }
        }
    }

    cmdEntry_t entityCommandEntries[] = {
        {"Blowup", "[entity number]\nTarget entity is blown up. (Unstable)", Cmd_BlowUp_f, 0, qtrue, 1, 0, 0},
        {
            "BlueSpawnPoint", "\nMakes the current position a spawn point for blue team players.",
            Cmd_PlayerSpawnPoint_f,
            TEAM_BLUE, qtrue, 1, 0, 0
        },
        {
            "Bounds",
            "[model | entity number]\nGet the default bounding box of a model, or show the bounding box of an entity.",
            Cmd_Bounds_f, 0, qtrue, 1, 0, 0
        },
        {
            "DelAll",
            "<field or classname> [value]\nDelete all entities with the given key/value or classname.  Only deletes editable entities.",
            Cmd_DelAll_f, 0, qtrue, 1, 0, 0
        },
        {
            "DelEnt",
            "[entity number]\nDeletes the specified entity number.  If no entity number is given, delete the ent you are aiming at.",
            Cmd_Delent_f, 0, qtrue, 1, 0, 0
        },
        {
            "Disable",
            "[entity number]\nDisable or enable the specified entity.  If no entity is given, the entity being targeted will be affected.",
            Cmd_DisableEnt_f, 0, qtrue, 2, 0, 0
        },
        {
            "EntityInfo",
            "[classname]\nSearches for and gets information about the given entity.  If no classname is given, then the entity being targeted will be used.  If a partial entity name is given, it lists all entities that match the given name.  Otherwise it lists all spawnflags and keys of the found entity.",
            Cmd_Entityinfo_t, 0, qtrue, 1, 0, 0
        },
        {
            "EntityList",
            "[offset] [key or classname] [value]\nShow all entities, or search entities by spawnstring keys.",
            Cmd_Entitylist_f, 0, qtrue, 1, 0, 0
        },
        {"GetEnt", "[entity number]\nMoves the entity into your current position.", Cmd_GetEnt_f, 0, qtrue, 1, 0, 0},
        {
            "GotoEnt", "[entity number]\nTeleports you to the entity's current position.", Cmd_GetEnt_f, 1, qtrue, 2, 0,
            0
        },
        {
            "JailSpawnPoint", "\nMakes the current position a jail spawn point.", Cmd_PlayerSpawnPoint_f, TEAM_JAILED,
            qtrue, 1, 0, 0
        },
        {
            "MapEnts", "{^2count^3} {^2save ^5<name>^3} {^2load ^5<name>^3}\nCount, save, and load map entities.",
            Cmd_Mapents_f, 0, qtrue, 1, 0, 0
        },
        {
            "Nearby",
            "[<distance> [filter]]\nLists all nearby entities.  If the filter arg is given, only entities with the classnames matching the front-partial value will be listed.",
            Cmd_Nearby_f, 0, qtrue, 1, 0, 0
        },
        {"NewMap", "<mapname>\nLoad a new map.", Cmd_NewMap_f, 0, qtrue, 2, 0, 0},
        {"NextMap", "\nGo to the next map in the rotation, if any.", Cmd_NextMap_f, 0, qtrue, 2, 0, 0},
        {
            "Nudge",
            "[entity index] <x y z>\nPush the specified entity in the given direction.  If no entity is specified, than the entity being aimed at will be affected.",
            Cmd_Nudge_f, 0, qtrue, 1, 0, 0
        },
        {
            "Place",
            "<classname> <dist> [field],[value], ...\nSpawn anything. It will spawn at the distance <dist> from the surface you are aiming at. ",
            Cmd_Place_f, 1, qtrue, 1, 0, 0
        },
	{
		"SelectEnt",
		"<EntityNumber>.\nSelect an Entity to have certain options afterwards.",
		Cmd_Select_f, 1, qtrue, 1, 0, 0
	},
        {
            "PlaceCannon", "\nSpawns an emplaced gun.", Cmd_PlaceCannon_f, 0, qtrue, 2, 0,
            (1 << GT_SIEGE) | (1 << GT_BATTLE_GROUND)
        },
        {"PlaceRandomSpot", "\nPlace a spot for random placements.", Cmd_PlaceRandomSpot_f, 0, qtrue, 1, 0, 0},
        {
            "PlayerSpawnPoint", "\nMakes the current position a spawn point for players.", Cmd_PlayerSpawnPoint_f,
            TEAM_FREE, qtrue, 1, 0, 0
        },
        {
            "RedSpawnPoint", "\nMakes the current position a spawn point for red team players.", Cmd_PlayerSpawnPoint_f,
            TEAM_RED, qtrue, 1, 0, 0
        },
        {
            "Remap",
            "{^2list ^6[starting offset]^3} {^2remove ^5<index>^3} {^2replace ^5<old shader> <new shader>^3}\nRemap a shader, or list all remaps.  Changes are not permenant.",
            Cmd_Remap_f, 0, qtrue, 2, 0, 0
        },
        {
            "Show", "<entity class name|entity number>\nShow the location of entities.", Cmd_ShowEntities_f, 0, qtrue,
            1, 0,
            0
        },
        {
            "SpawnString", "<view, edit>\nView or edit a spawnstring on an existing entity.", Cmd_Spawnstring_f, 0,
            qtrue,
            1, 0, 0
        },
        {
            "Trace",
            "[entitynumber]\nGet info on entity with number [entitynumber]. If no argument is provided, the target in sight will be investigated.",
            Cmd_Trace_f, 0, qtrue, 1, 0, 0
        },
        {"UseEnt", "[number]\nTarget entity is activated.", Cmd_UseEnt_f, 0, qtrue, 1, 0, 0},
        {
            "UseTarg",
            "<targetname>\nUse a group of entities by targetname.  If no argument is provided, the targetname of the entity in sight will be used.",
            Cmd_UseEnt_f, 1, qtrue, 1, 0, 0
        },
        {"Grab", "[number]\nGrab entity.", Cmd_Grab_f, GRAB_ORIGIN, qtrue, 1, 0, 0},
        {
            "GrabOffsetIncrease", "\nIncrease the distance between the grabbed entity and the player.",
            Cmd_GrabOffsetInc_f,
            0, qtrue, 1, 0, 0
        },
        {
            "GrabOffsetDecrease", "\nDecrease the distance between the grabbed entity and the player.",
            Cmd_GrabOffsetDec_f,
            0, qtrue, 1, 0, 0
        },
        {"GrabX", "[number]\nGrab entity, and rotate it along the X axis.", Cmd_Grab_f, GRAB_X, qtrue, 1, 0, 0},
        {"GrabY", "[number]\nGrab entity, and rotate it along the Y axis.", Cmd_Grab_f, GRAB_Y, qtrue, 1, 0, 0},
        {"GrabZ", "[number]\nGrab entity, and rotate it along the Z axis.", Cmd_Grab_f, GRAB_Z, qtrue, 1, 0, 0},
        {
            "CancelGrab", "[number]\nDrop a grabbed entity, and restore its original position / angle.",
            Cmd_CancelGrab_f,
            0, qtrue, 1, 0, 0
        },
        {"Clone", "[number]\nClone entity.", Cmd_Clone_f, 0, qtrue, 1, 0, 0},
        {"Measure", "Measure distance.", Cmd_Measure_f, 0, qtrue, 1, 0, 0},
        {NULL}
    };
