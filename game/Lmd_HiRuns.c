#include "g_local.h"

#include "Lmd_Arrays.h"
#include "Lmd_Console.h"
#include "Lmd_Data.h"
#include "Lmd_HiRuns.h"

void lmd_menu_enter(gentity_t *player, gentity_t *menu);

typedef struct {
	unsigned int count;
	LmdHiRun_t *runs;
} HiRunsState_t;

static HiRunsState_t HiRunsState;

static int HiRuns_GetClientRunIndex(const gclient_t *client) {
	if (!client || client->Lmd.hiRuns.runIndex <= 0) {
		return -1;
	}
	return client->Lmd.hiRuns.runIndex - 1;
}

static void HiRuns_ClearClientState(gclient_t *client) {
	if (!client) {
		return;
	}
	client->Lmd.hiRuns.runIndex = 0;
	client->Lmd.hiRuns.pendingStart = qfalse;
	client->Lmd.hiRuns.running = qfalse;
	client->Lmd.hiRuns.pendingStop = qfalse;
}

static void HiRuns_SetClientRunIndex(gclient_t *client, int runIndex) {
	if (!client) {
		return;
	}
	client->Lmd.hiRuns.runIndex = runIndex + 1;
}

static LmdHiRun_t *HiRuns_GetRun(int index) {
	if (index < 0 || index >= (int)HiRunsState.count) {
		return NULL;
	}
	return &HiRunsState.runs[index];
}

static int HiRuns_FindIndex(const char *name) {
	if (!name || !name[0]) {
		return -1;
	}
	for (unsigned int i = 0; i < HiRunsState.count; i++) {
		if (Q_stricmp(HiRunsState.runs[i].name, name) == 0) {
			return (int)i;
		}
	}
	return -1;
}

static LmdHiRun_t *HiRuns_GetOrCreate(const char *name, int *outIndex) {
	int index = HiRuns_FindIndex(name);
	if (index >= 0) {
		if (outIndex) {
			*outIndex = index;
		}
		return &HiRunsState.runs[index];
	}

	index = (int)Lmd_Arrays_AddArrayElement((void **)&HiRunsState.runs, sizeof(LmdHiRun_t), &HiRunsState.count);
	LmdHiRun_t *run = &HiRunsState.runs[index];
	Q_strncpyz(run->name, name, sizeof(run->name));
	if (outIndex) {
		*outIndex = index;
	}
	return run;
}

static void HiRuns_FormatDuration(int durationMs, char *out, int outSize) {
	int totalSeconds = durationMs / 1000;
	int millis = durationMs % 1000;
	int minutes = totalSeconds / 60;
	int seconds = totalSeconds % 60;
	int hours = minutes / 60;
	minutes = minutes % 60;

	if (hours > 0) {
		Com_sprintf(out, outSize, "%d:%02d:%02d.%03d", hours, minutes, seconds, millis);
	} else {
		Com_sprintf(out, outSize, "%d:%02d.%03d", minutes, seconds, millis);
	}
}

static void HiRuns_SanitizeName(const char *src, char *dst, int dstSize) {
	char clean[MAX_STRING_CHARS];
	Q_strncpyz(clean, src ? src : "", sizeof(clean));
	Q_CleanStr(clean);
	Q_strncpyz(dst, clean, dstSize);
	for (int i = 0; dst[i]; i++) {
		if (dst[i] == '"') {
			dst[i] = '\'';
		}
	}
}

static void HiRuns_InsertEntry(LmdHiRun_t *run, int durationMs, const char *players) {
	if (!run || durationMs <= 0) {
		return;
	}

	int insertIndex = run->entryCount;
	for (int i = 0; i < run->entryCount; i++) {
		if (durationMs < run->entries[i].durationMs) {
			insertIndex = i;
			break;
		}
	}

	if (insertIndex >= LMD_HIRUNS_MAX_ENTRIES) {
		return;
	}

	if (run->entryCount < LMD_HIRUNS_MAX_ENTRIES) {
		if (insertIndex < run->entryCount) {
			memmove(&run->entries[insertIndex + 1], &run->entries[insertIndex],
				sizeof(run->entries[0]) * (run->entryCount - insertIndex));
		}
		run->entryCount++;
	} else if (insertIndex < LMD_HIRUNS_MAX_ENTRIES - 1) {
		memmove(&run->entries[insertIndex + 1], &run->entries[insertIndex],
			sizeof(run->entries[0]) * (LMD_HIRUNS_MAX_ENTRIES - 1 - insertIndex));
	}

	run->entries[insertIndex].durationMs = durationMs;
	Q_strncpyz(run->entries[insertIndex].players, players ? players : "", sizeof(run->entries[insertIndex].players));
}

static void HiRuns_ResetRunState(LmdHiRun_t *run) {
	if (!run) {
		return;
	}
	run->active = qfalse;
	run->requiredCount = 0;
	run->startTime = 0;
	run->participantCount = 0;
	memset(run->participants, 0, sizeof(run->participants));
	memset(run->stopRequested, 0, sizeof(run->stopRequested));
}

static qboolean HiRuns_IsParticipant(const LmdHiRun_t *run, int clientNum) {
	if (!run) {
		return qfalse;
	}
	for (int i = 0; i < run->participantCount; i++) {
		if (run->participants[i] == clientNum) {
			return qtrue;
		}
	}
	return qfalse;
}

static void HiRuns_RemoveParticipant(LmdHiRun_t *run, int clientNum) {
	if (!run) {
		return;
	}
	for (int i = 0; i < run->participantCount; i++) {
		if (run->participants[i] == clientNum) {
			if (i + 1 < run->participantCount) {
				memmove(&run->participants[i], &run->participants[i + 1],
					sizeof(run->participants[0]) * (run->participantCount - i - 1));
			}
			run->participantCount--;
			run->stopRequested[clientNum] = qfalse;
			return;
		}
	}
}

static qboolean HiRuns_AllStopped(const LmdHiRun_t *run) {
	if (!run) {
		return qfalse;
	}
	for (int i = 0; i < run->participantCount; i++) {
		int clientNum = run->participants[i];
		if (!run->stopRequested[clientNum]) {
			return qfalse;
		}
	}
	return qtrue;
}

static void HiRuns_BuildPlayersLabel(const LmdHiRun_t *run, char *out, int outSize) {
	out[0] = '\0';
	if (!run) {
		return;
	}
	for (int i = 0; i < run->participantCount; i++) {
		int clientNum = run->participants[i];
		if (clientNum < 0 || clientNum >= MAX_CLIENTS) {
			continue;
		}
		gentity_t *ent = &g_entities[clientNum];
		if (!ent->client) {
			continue;
		}
		char cleanName[LMD_HIRUNS_PLAYERS_LEN];
		HiRuns_SanitizeName(ent->client->pers.netname, cleanName, sizeof(cleanName));
		if (out[0]) {
			Q_strcat(out, outSize, " + ");
		}
		Q_strcat(out, outSize, cleanName);
	}
}

static void HiRuns_BuildDisplay(const LmdHiRun_t *run, char *out, int outSize) {
	char line[MAX_STRING_CHARS];
	out[0] = '\0';

	if (!run) {
		Q_strncpyz(out, "^3No run data found.", outSize);
		return;
	}

	Q_strncpyz(out, va("^5Top runs for ^3%s^5\n\n", run->name), outSize);

	if (run->entryCount == 0) {
		Q_strcat(out, outSize, "^7No runs recorded yet.");
		return;
	}

	int maxEntries = run->entryCount;
	if (maxEntries > LMD_HIRUNS_MAX_ENTRIES) {
		maxEntries = LMD_HIRUNS_MAX_ENTRIES;
	}

	for (int i = 0; i < maxEntries; i++) {
		char timeBuf[32];
		HiRuns_FormatDuration(run->entries[i].durationMs, timeBuf, sizeof(timeBuf));
		Com_sprintf(line, sizeof(line), "^2%2i^7) ^3%s^7 - ^5%s\n",
			i + 1, timeBuf, run->entries[i].players);
		if ((int)strlen(out) + (int)strlen(line) >= outSize) {
			break;
		}
		Q_strcat(out, outSize, line);
	}
}

static void HiRuns_AbortRun(LmdHiRun_t *run, const char *reason) {
	if (!run || run->participantCount <= 0) {
		return;
	}

	for (int i = 0; i < run->participantCount; i++) {
		int clientNum = run->participants[i];
		if (clientNum < 0 || clientNum >= MAX_CLIENTS) {
			continue;
		}
		gentity_t *ent = &g_entities[clientNum];
		if (!ent->client) {
			continue;
		}
		if (reason && reason[0]) {
			Disp(ent, reason);
		}
		HiRuns_ClearClientState(ent->client);
	}

	HiRuns_ResetRunState(run);
}

static qboolean HiRuns_Load_Callback(char *key, char *value, void *state) {
	char runName[MAX_STRING_CHARS] = "";
	char players[MAX_STRING_CHARS] = "";
	int durationMs = 0;

	if (Q_stricmp(key, "run") != 0) {
		return qfalse;
	}

	int matched = sscanf(value, "\"%1024[^\"]\" %d \"%1024[^\"]\"", runName, &durationMs, players);
	if (matched < 2) {
		return qfalse;
	}
	if (matched < 3) {
		players[0] = 0;
	}

	int index = 0;
	LmdHiRun_t *run = HiRuns_GetOrCreate(runName, &index);
	HiRuns_InsertEntry(run, durationMs, players);
	return qtrue;
}

static DBSaveFileCallbackReturn_t *HiRuns_Save_Callback(byte *structure, DBSaveFileCallbackReturn_t *arg,
	char *key, int keySze, char *value, int valueSze) {
	while (arg->func < HiRunsState.count) {
		LmdHiRun_t *run = &HiRunsState.runs[arg->func];
		if (arg->offset >= run->entryCount) {
			arg->func++;
			arg->offset = 0;
			continue;
		}

		Q_strncpyz(key, "run", keySze);
		Q_strncpyz(value, va("\"%s\" %d \"%s\"", run->name, run->entries[arg->offset].durationMs,
			run->entries[arg->offset].players), valueSze);
		arg->offset++;
		return arg;
	}

	return NULL;
}

void HiRuns_Load(void) {
	char *buf = Lmd_Data_AllocFileContents(LMD_HIRUNS_FILE);
	if (!buf) {
		return;
	}
	char *str = buf;
	Lmd_Data_ParseKeys_Old(&str, qtrue, HiRuns_Load_Callback, NULL);
	G_Free(buf);
}

void HiRuns_Save(void) {
	Lmd_Data_SaveDatafile(NULL, LMD_HIRUNS_FILE, NULL, NULL, NULL, HiRuns_Save_Callback);
}

void HiRuns_ListRuns(gentity_t *ent) {
	if (HiRunsState.count == 0) {
		Disp(ent, "^3No runs recorded.");
		return;
	}

	Disp(ent, "^5Available runs:");
	for (unsigned int i = 0; i < HiRunsState.count; i++) {
		Disp(ent, "^2%i^3: ^7%s", i + 1, HiRunsState.runs[i].name);
	}
}

void HiRuns_Show(gentity_t *ent, const char *nameOrIndex) {
	char msg[MAX_STRING_CHARS];
	int runIndex = -1;

	if (!nameOrIndex || !nameOrIndex[0]) {
		HiRuns_ListRuns(ent);
		return;
	}

	qboolean numeric = qtrue;
	for (int i = 0; nameOrIndex[i]; i++) {
		if (nameOrIndex[i] < '0' || nameOrIndex[i] > '9') {
			numeric = qfalse;
			break;
		}
	}

	if (numeric) {
		int index = atoi(nameOrIndex);
		if (index > 0 && index <= (int)HiRunsState.count) {
			runIndex = index - 1;
		}
	} else {
		int matches = 0;
		int len = strlen(nameOrIndex);
		for (unsigned int i = 0; i < HiRunsState.count; i++) {
			if (Q_stricmpn(nameOrIndex, HiRunsState.runs[i].name, len) == 0) {
				runIndex = (int)i;
				matches++;
			}
		}
		if (matches > 1) {
			Disp(ent, "^3Multiple runs match that name:");
			for (unsigned int i = 0; i < HiRunsState.count; i++) {
				if (Q_stricmpn(nameOrIndex, HiRunsState.runs[i].name, len) == 0) {
					Disp(ent, va("^2%i^3: ^7%s", i + 1, HiRunsState.runs[i].name));
				}
			}
			return;
		}
	}

	if (runIndex < 0) {
		Disp(ent, "^3No run by that name.");
		return;
	}

	LmdHiRun_t *run = HiRuns_GetRun(runIndex);
	HiRuns_BuildDisplay(run, msg, sizeof(msg));
	Disp(ent, msg);
}

void HiRuns_TimerStart(gentity_t *player, const char *runName, int requiredCount) {
	if (!player || !player->client || !runName || !runName[0]) {
		return;
	}

	if (requiredCount < 1) {
		requiredCount = 1;
	}
	if (requiredCount > MAX_CLIENTS) {
		requiredCount = MAX_CLIENTS;
	}

	int runIndex = 0;
	LmdHiRun_t *run = HiRuns_GetOrCreate(runName, &runIndex);

	int currentRunIndex = HiRuns_GetClientRunIndex(player->client);
	if (currentRunIndex >= 0 && currentRunIndex != runIndex) {
		Disp(player, "^3You are already queued for another run.");
		return;
	}

	if (run->active) {
		Disp(player, "^3That run is already active.");
		return;
	}

	if (HiRuns_IsParticipant(run, player->s.number)) {
		Disp(player, "^3You are already queued for that run.");
		return;
	}

	if (run->participantCount >= MAX_CLIENTS) {
		Disp(player, "^3That run is full.");
		return;
	}

	if (run->participantCount > 0) {
		requiredCount = run->requiredCount;
	} else {
		run->requiredCount = requiredCount;
	}
	run->participants[run->participantCount++] = player->s.number;
	HiRuns_SetClientRunIndex(player->client, runIndex);
	player->client->Lmd.hiRuns.pendingStart = qtrue;

	if (run->participantCount >= run->requiredCount) {
		run->active = qtrue;
		run->startTime = level.time;
		for (int i = 0; i < run->participantCount; i++) {
			int clientNum = run->participants[i];
			if (clientNum < 0 || clientNum >= MAX_CLIENTS) {
				continue;
			}
			gentity_t *ent = &g_entities[clientNum];
			if (!ent->client) {
				continue;
			}
			ent->client->Lmd.hiRuns.pendingStart = qfalse;
			ent->client->Lmd.hiRuns.running = qtrue;
			ent->client->Lmd.hiRuns.pendingStop = qfalse;
			run->stopRequested[clientNum] = qfalse;
			Disp(ent, "^5Run started: ^3%s", run->name);
		}
	} else {
		Disp(player, "^3Waiting for ^2%i^3 more player%s to start.",
			run->requiredCount - run->participantCount,
			(run->requiredCount - run->participantCount) == 1 ? "" : "s");
	}
}

void HiRuns_TimerStop(gentity_t *player, const char *runName) {
	if (!player || !player->client || !runName || !runName[0]) {
		return;
	}

	int runIndex = HiRuns_FindIndex(runName);
	LmdHiRun_t *run = HiRuns_GetRun(runIndex);
	if (!run) {
		Disp(player, "^3No run by that name.");
		return;
	}

	if (!run->active) {
		if (HiRuns_IsParticipant(run, player->s.number)) {
			HiRuns_RemoveParticipant(run, player->s.number);
			HiRuns_ClearClientState(player->client);
			Disp(player, "^3Removed from pending run.");
		} else {
			Disp(player, "^3That run is not active.");
		}
		return;
	}

	if (!HiRuns_IsParticipant(run, player->s.number)) {
		Disp(player, "^3You are not part of that run.");
		return;
	}

	if (run->stopRequested[player->s.number]) {
		Disp(player, "^3You have already marked your finish.");
		return;
	}

	run->stopRequested[player->s.number] = qtrue;
	player->client->Lmd.hiRuns.pendingStop = qtrue;

	if (!HiRuns_AllStopped(run)) {
		Disp(player, "^3Waiting for others to finish.");
		return;
	}

	int durationMs = level.time - run->startTime;
	char playersLabel[LMD_HIRUNS_PLAYERS_LEN];
	HiRuns_BuildPlayersLabel(run, playersLabel, sizeof(playersLabel));
	HiRuns_InsertEntry(run, durationMs, playersLabel);
	HiRuns_Save();

	char timeBuf[32];
	HiRuns_FormatDuration(durationMs, timeBuf, sizeof(timeBuf));

	for (int i = 0; i < run->participantCount; i++) {
		int clientNum = run->participants[i];
		if (clientNum < 0 || clientNum >= MAX_CLIENTS) {
			continue;
		}
		gentity_t *ent = &g_entities[clientNum];
		if (!ent->client) {
			continue;
		}
		Disp(ent, va("^5Run finished: ^3%s ^7- ^2%s", run->name, timeBuf));
		HiRuns_ClearClientState(ent->client);
	}

	HiRuns_ResetRunState(run);
}

static void HiRuns_TerminalCleanup(gentity_t *ent) {
	for (int i = 0; i < MAX_CLIENTS; i++) {
		gclient_t *client = level.clients + i;
		if (!client) {
			continue;
		}
		
		if (client->pers.connected == CON_CONNECTED &&
			client->Lmd.lmdMenu.entityNum == ent->s.number) {
			ent->nextthink = level.time + 2000;
			return;
		}
	}
	
	G_FreeEntity(ent);
}

void lmd_menu_exit(gentity_t* player);
void HiRuns_ShowTerminal(gentity_t *player, const char *runName) {
	if (!player || !player->client || !runName || !runName[0]) {
		return;
	}

	

	int runIndex = HiRuns_FindIndex(runName);
	LmdHiRun_t *run = HiRuns_GetRun(runIndex);
	char msg[MAX_STRING_CHARS];
	if (!run) {
		Q_strncpyz(msg, "^3No run by that name.", sizeof(msg));
	} else {
		HiRuns_BuildDisplay(run, msg, sizeof(msg));
	}

	gentity_t *menu = G_Spawn();
	menu->classname = G_NewString("lmd_terminal");
	menu->spawnflags = 4;
	menu->r.svFlags |= SVF_NOCLIENT;
	menu->message = G_NewString2(msg);
	menu->count = 0;
	menu->Lmd.color = G_NewString2("^7");
	menu->Lmd.color2 = G_NewString2("^7");
	menu->Lmd.customIndex = 0;
	menu->think = HiRuns_TerminalCleanup;
	menu->nextthink = level.time + 500;

	if (player->client->Lmd.lmdMenu.entityNum != 0)
		lmd_menu_exit(player);
	lmd_menu_enter(player, menu);
}

void HiRuns_ClientDisconnect(gentity_t *player) {
	if (!player || !player->client) {
		return;
	}

	int runIndex = HiRuns_GetClientRunIndex(player->client);
	LmdHiRun_t *run = HiRuns_GetRun(runIndex);
	if (!run) {
		return;
	}

	HiRuns_RemoveParticipant(run, player->s.number);
	HiRuns_ClearClientState(player->client);

	if (run->active && run->participantCount < run->requiredCount) {
		HiRuns_AbortRun(run, "^3Run canceled due to disconnect.");
	}
}
