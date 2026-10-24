#ifndef LMD_HIRUNS_H
#define LMD_HIRUNS_H

#include "g_local.h"

#define LMD_HIRUNS_FILE "hiruns"
#define LMD_HIRUNS_MAX_ENTRIES 25
#define LMD_HIRUNS_NAME_LEN 64
#define LMD_HIRUNS_PLAYERS_LEN 128

typedef struct {
	int durationMs;
	char players[LMD_HIRUNS_PLAYERS_LEN];
} LmdHiRunEntry_t;

typedef struct {
	char name[LMD_HIRUNS_NAME_LEN];
	int entryCount;
	LmdHiRunEntry_t entries[LMD_HIRUNS_MAX_ENTRIES];

	// Active run state (not saved)
	qboolean active;
	int requiredCount;
	int startTime;
	int participantCount;
	int participants[MAX_CLIENTS];
	qboolean stopRequested[MAX_CLIENTS];
} LmdHiRun_t;

void HiRuns_Load(void);
void HiRuns_Save(void);
void HiRuns_ListRuns(gentity_t *ent);
void HiRuns_Show(gentity_t *ent, const char *nameOrIndex);
void HiRuns_TimerStart(gentity_t *player, const char *runName, int requiredCount);
void HiRuns_TimerStop(gentity_t *player, const char *runName);
void HiRuns_ShowTerminal(gentity_t *player, const char *runName);
void HiRuns_ClientDisconnect(gentity_t *player);

#endif
