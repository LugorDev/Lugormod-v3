// Exercises real account parsing/serialization and target logic through a mock
// engine syscall boundary. No server data or external files are read or written.
#include "../game/g_local.h"
#include "../game/Lmd_Accounts_Core.h"
#include "../game/Lmd_Accounts_CustomSkills.h"
#include "../game/Lmd_Accounts_Stats.h"
#include "../game/Lmd_Checksum.h"
#include "../game/Lmd_SpecialSkills.h"
#include "../game/Lmd_Commands_Core.h"
#include "../game/Lmd_Commands_Auths.h"
#include "../game/Lmd_HiRuns.h"
#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
static LONG WINAPI reportException(EXCEPTION_POINTERS *exception) {
    struct { SYMBOL_INFO info; char name[256]; } storage = {0};
    SYMBOL_INFO *symbol = &storage.info;
    DWORD64 displacement;
    symbol->SizeOfStruct = sizeof(*symbol);
    symbol->MaxNameLen = 255;
    SymInitialize(GetCurrentProcess(), NULL, TRUE);
    if (SymFromAddr(GetCurrentProcess(), (DWORD64)exception->ExceptionRecord->ExceptionAddress, &displacement, symbol))
        fprintf(stderr, "Exception in %s + %llu\n", symbol->Name, (unsigned long long)displacement);
#ifdef _WIN64
    STACKFRAME64 frame = {0};
    CONTEXT context = *exception->ContextRecord;
    frame.AddrPC.Offset = context.Rip;
    frame.AddrFrame.Offset = context.Rbp;
    frame.AddrStack.Offset = context.Rsp;
    frame.AddrPC.Mode = frame.AddrFrame.Mode = frame.AddrStack.Mode = AddrModeFlat;
    for (int i = 0; i < 12 && StackWalk64(IMAGE_FILE_MACHINE_AMD64, GetCurrentProcess(), GetCurrentThread(),
        &frame, &context, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL); i++) {
        if (SymFromAddr(GetCurrentProcess(), frame.AddrPC.Offset, &displacement, symbol))
            fprintf(stderr, "  %s + %llu\n", symbol->Name, (unsigned long long)displacement);
    }
#endif
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

void dllEntry(int (QDECL *callback)(int, ...));
void Accounts_Prof_Register(void);
void Accounts_Stats_Register(void);
void Accounts_CustomSkills_Register(void);
qboolean parseAccount(char *name, char *buffer);
void Use_Target_Fp(gentity_t *ent, gentity_t *other, gentity_t *activator);
extern vmCvar_t lmd_accBaseDays;
extern vmCvar_t lmd_accMaxDays;
extern vmCvar_t lmd_specialSkills;
extern vmCvar_t lmd_skillCreditCost;
extern vmCvar_t lmd_startingLevel;
int Professions_StartingLevel(void);

static char saved[65536];
static size_t savedLength;
static int failOpen;
static int checks;
static const char *commandArgs[4];
static char screens[MAX_CLIENTS][MAX_STRING_CHARS + 32];
#define CHECK(condition) do { checks++; if (!(condition)) { \
  fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); exit(1); } } while (0)

static int QDECL mockEngine(int call, ...) {
    va_list args;
    va_start(args, call);
    switch (call) {
    case G_ARGV: {
        int index = va_arg(args, int);
        char *out = va_arg(args, char *);
        int size = va_arg(args, int);
        Q_strncpyz(out, index < 4 && commandArgs[index] ? commandArgs[index] : "", size);
        break;
    }
    case G_SEND_SERVER_COMMAND: {
        int client = va_arg(args, int);
        const char *message = va_arg(args, const char *);
        if (client >= 0 && client < MAX_CLIENTS && !strncmp(message, "cp ", 3))
            Q_strncpyz(screens[client], message, sizeof(screens[client]));
        break;
    }
    case G_REAL_TIME: {
        qtime_t *now = va_arg(args, qtime_t *);
        memset(now, 0, sizeof(*now));
        now->tm_year = 126;
        break;
    }
    case G_PRINT:
        fputs(va_arg(args, const char *), stdout);
        break;
    case G_ERROR:
        fprintf(stderr, "Engine error: %s\n", va_arg(args, const char *));
        exit(2);
    case G_FS_FOPEN_FILE: {
        const char *path = va_arg(args, const char *);
        fileHandle_t *handle = va_arg(args, fileHandle_t *);
        int mode = va_arg(args, int);
        (void)path;
        CHECK(mode == FS_WRITE);
        *handle = failOpen ? 0 : 1;
        if (!failOpen) savedLength = 0;
        break;
    }
    case G_FS_WRITE: {
        const char *bytes = va_arg(args, const char *);
        int length = va_arg(args, int);
        CHECK(savedLength + length < sizeof(saved));
        memcpy(saved + savedLength, bytes, length);
        savedLength += length;
        saved[savedLength] = 0;
        break;
    }
    case G_FS_FCLOSE_FILE:
        break;
    default:
        fprintf(stderr, "Unexpected engine syscall %d\n", call);
        exit(3);
    }
    va_end(args);
    return 0;
}

static void shopInput(gentity_t *player, gentity_t *shop, int movement, int buttons) {
    usercmd_t command = {0};
    command.forwardmove = movement;
    command.buttons = buttons;
    SSkills_ShopMenu(player, shop, &command);
}

static void shopInteract(gentity_t *shop, gentity_t *player) {
    SSkills_ShopOpen(shop, player);
}

int main(void) {
#ifdef _WIN32
    SetUnhandledExceptionFilter(reportException);
#endif
    dllEntry(mockEngine);
    level.time = 1000;
    lmd_accBaseDays.integer = 100000;
    lmd_accMaxDays.integer = 0;
    Accounts_Prof_Register();
    Accounts_Stats_Register();
    Accounts_CustomSkills_Register();

    char legacy[] = "id: 101\nname: Legacy\npassword: example\ncredits: 12345\n"
        "profession: 4\nlevel: 27\nkills: 19\nskill_custom_test: unlocked\n";
    CHECK(parseAccount("legacy", legacy));
    Account_t *acc = Accounts_GetById(101);
    CHECK(acc != NULL);
    CHECK(Characters_Get(~0U) == NULL);
    CHECK(Characters_Get(Characters_Count()) == NULL);
    CHECK(Account_GetNumCharacters(acc) == 1);
    CHECK(Accounts_GetCredits(acc) == 12345);
    CHECK(Accounts_Prof_GetLevel(acc) == 27);
    CHECK(Accounts_Stats_GetKills(acc) == 19);
    CHECK(!strcmp(Accounts_Custom_GetValue(acc, "test"), "unlocked"));
    CHECK(Accounts_SaveChecked(acc));
    CHECK(strstr(saved, "[character]\n") != NULL);
    char roundtrip[sizeof(saved)];
    strcpy(roundtrip, saved);
    // A distinct id allows both versions to coexist in the in-memory registry.
    char *id = strstr(roundtrip, "101");
    CHECK(id != NULL);
    memcpy(id, "102", 3);
    CHECK(parseAccount("roundtrip", roundtrip));
    Account_t *copy = Accounts_GetById(102);
    CHECK(copy && Accounts_GetCredits(copy) == 12345);
    CHECK(Accounts_Prof_GetLevel(copy) == 27);
    CHECK(Accounts_Stats_GetKills(copy) == 19);
    CHECK(!strcmp(Accounts_Custom_GetValue(copy, "test"), "unlocked"));

    Character_t *first = Account_GetActiveCharacter(copy);
    Character_t *second = Account_NewCharacter(copy, "Second");
    CHECK(second && Account_GetActiveCharacter(copy) == first);
    Account_SetActiveCharacter(copy, second);
    Accounts_SetCredits(copy, 77);
    Accounts_Custom_SetValue(copy, "test", "second");
    Account_SetActiveCharacter(copy, first);
    CHECK(Accounts_GetCredits(copy) == 12345);
    CHECK(!strcmp(Accounts_Custom_GetValue(copy, "test"), "unlocked"));
    CHECK(!Account_MoveCharacter(copy, first, copy));
    CHECK(Account_NewCharacter(copy, "Third") != NULL);
    CHECK(Account_NewCharacter(copy, "Fourth") == NULL);
    CHECK(Accounts_SaveChecked(copy));
    CHECK(strstr(saved, "Second") && strstr(saved, "Third"));
    strcpy(roundtrip, saved);
    id = strstr(roundtrip, "102");
    CHECK(id != NULL);
    memcpy(id, "103", 3);
    CHECK(parseAccount("multi", roundtrip));
    Account_t *multi = Accounts_GetById(103);
    CHECK(multi && Account_GetNumCharacters(multi) == 3);
    Account_SetActiveCharacter(multi, Account_GetCharacter(multi, 1));
    CHECK(Accounts_GetCredits(multi) == 77);
    CHECK(!strcmp(Accounts_Custom_GetValue(multi, "test"), "second"));
    CHECK(Accounts_SaveChecked(multi));
    failOpen = 1;
    CHECK(!Accounts_SaveChecked(copy));
    failOpen = 0;

    static gentity_t target, player;
    static gclient_t client;
    player.client = &client;
    CHECK(!Auths_PlayerHasAuthFlag(&player, AUTH_NO_FLOODPROTECTION));
    char powers[] = "push3.pull2";
    target.target2 = powers;
    Use_Target_Fp(&target, NULL, &player);
    CHECK(client.ps.fd.forcePowerLevel[FP_PUSH] == 3);
    CHECK(client.ps.fd.forcePowerLevel[FP_PULL] == 2);

    lmd_startingLevel.integer = 40;
    CHECK(Professions_StartingLevel() == 40);
    lmd_startingLevel.integer = 200;
    CHECK(Professions_StartingLevel() == 40);
    lmd_startingLevel.integer = -10;
    CHECK(Professions_StartingLevel() == 1);
    lmd_startingLevel.integer = 40;
    lmd_specialSkills.integer = 1;

    cmdEntry_t *shortcut = Commands_GetEntry("forceshield");
    CHECK(shortcut && shortcut->func == SSkills_Command && shortcut->iArg == SKILL_SHIELD + 1);
    int shortcuts = 0;
    for (cmdCategory_t *category = Categories; category->name; category++) {
        for (cmdEntry_t *entry = category->entries; entry->name; entry++) {
            if (entry->func == SSkills_Command && entry->iArg > 0) {
                CHECK(Commands_GetEntry(entry->name) == entry);
                shortcuts++;
            }
        }
    }
    CHECK(shortcuts == NUM_SKILLS);
    lmd_skillCreditCost.integer = 100;
    SSkills_Init();
    SSkills_Reset(&player, qtrue);
    player.inuse = qtrue;
    player.health = client.ps.stats[STAT_HEALTH] = client.ps.stats[STAT_MAX_HEALTH] = 100;
    client.pers.Lmd.account = copy;
    client.pers.Lmd.character = first;
    client.ps.fd.forcePower = client.ps.fd.forcePowerMax = 100;
    client.sess.sessionTeam = TEAM_FREE;
    Accounts_Prof_SetProfession(copy, 3); // Jedi
    Accounts_Prof_SetLevel(copy, 39);
    Accounts_SetCredits(copy, 2000);
    CHECK(!SSkills_Buy(&player, SKILL_SHIELD));
    CHECK(Accounts_GetCredits(copy) == 2000);
    Accounts_Prof_SetLevel(copy, 40);
    CHECK(!SSkills_Buy(&player, SKILL_BACTAS)); // Wrong profession.
    CHECK(SSkills_Buy(&player, SKILL_SHIELD));
    CHECK(Accounts_GetCredits(copy) == 1700);
    CHECK(!SSkills_Buy(&player, SKILL_SHIELD));
    CHECK(Accounts_GetCredits(copy) == 1700);
    CHECK(strstr(saved, "sskill_forceshield: 1") != NULL);
    commandArgs[1] = "forceshield";
    SSkills_Command(&player, 0);
    CHECK(client.ps.fd.forcePower == 20);
    CHECK((player.flags & FL_SHIELDED) != 0);
    CHECK(!SSkills_ForceAllowed(NULL, &player, FP_PUSH));
    SSkills_Reset(&player, qfalse);
    CHECK(!(player.flags & FL_SHIELDED));
    client.ps.fd.forcePower = 100;
    SSkills_Command(&player, 0);
    CHECK(client.ps.fd.forcePower == 100); // Respawn must not bypass cooldown.
    failOpen = 1;
    CHECK(!SSkills_Buy(&player, SKILL_DASH));
    CHECK(Accounts_GetCredits(copy) == 1700);
    CHECK(!SSkills_Has(&player, SKILL_DASH));
    failOpen = 0;
    Account_SetActiveCharacter(copy, second);
    CHECK(!SSkills_Has(&player, SKILL_SHIELD));
    Account_SetActiveCharacter(copy, first);
    CHECK(SSkills_Has(&player, SKILL_SHIELD));
    lmd_specialSkills.integer = 0;
    CHECK(!SSkills_Has(&player, SKILL_SHIELD));
    lmd_specialSkills.integer = 1;
    CHECK(!strcmp(powers, "push3.pull2"));
    client.ps.fd.forcePowerLevel[FP_PULL] = 0;
    Use_Target_Fp(&target, NULL, &player);
    CHECK(client.ps.fd.forcePowerLevel[FP_PULL] == 2);
    target.spawnflags = 1;
    client.ps.fd.forcePowersKnown = 1 << FP_PULL;
    client.ps.fd.forcePowerLevel[FP_PUSH] = 0;
    Use_Target_Fp(&target, NULL, &player);
    CHECK(client.ps.fd.forcePowerLevel[FP_PUSH] == 0);
    CHECK(client.ps.fd.forcePowerLevel[FP_PULL] == 2);

    // Drive the actual on-screen menu using the same input packets as the server.
    gentity_t *shop = &g_entities[MAX_CLIENTS];
    shop->inuse = qtrue;
    shop->s.number = MAX_CLIENTS;
    shop->classname = "lmd_skillshop";
    shop->interact = shopInteract;
    shop->radius = 128;
    shop->Lmd.customIndex = 2; // No console animation needed in the mock engine.
    Accounts_SetCredits(copy, 2000);
    SSkills_ShopOpen(shop, &player);
    CHECK(client.Lmd.lmdMenu.entityNum == shop->s.number);
    CHECK(strstr(screens[0], "Special Skill Shop") && strstr(screens[0], "Force Body"));
    CHECK(strstr(screens[0], "Page 1/4"));
    shopInput(&player, shop, 0, 0); // Release the opening Use press.
    shopInput(&player, shop, 0, BUTTON_USE);
    CHECK(strstr(screens[0], "Buy skill"));
    CHECK(Accounts_GetCredits(copy) == 2000);
    shopInput(&player, shop, 0, BUTTON_USE); // Holding Use cannot buy.
    CHECK(Accounts_GetCredits(copy) == 2000);
    shopInput(&player, shop, 0, 0);
    shopInput(&player, shop, 0, BUTTON_USE);
    CHECK(Accounts_GetCredits(copy) == 1700);
    CHECK(strstr(screens[0], "Skill purchased!") && strstr(screens[0], "Owned"));
    shopInput(&player, shop, 0, BUTTON_USE);
    CHECK(Accounts_GetCredits(copy) == 1700);
    shopInput(&player, shop, 0, 0);
    shopInput(&player, shop, -127, 0); // Destruction.
    shopInput(&player, shop, 0, 0);
    shopInput(&player, shop, 0, BUTTON_ATTACK);
    CHECK(strstr(screens[0], "Force Destruction") && strstr(screens[0], "Buy skill"));
    shopInput(&player, shop, 0, BUTTON_ALT_ATTACK);
    CHECK(!strstr(screens[0], "Buy skill"));
    CHECK(Accounts_GetCredits(copy) == 1700);
    shopInput(&player, shop, 0, 0);
    shopInput(&player, shop, 0, BUTTON_USE);
    lmd_skillCreditCost.integer = 200;
    shopInput(&player, shop, 0, 0);
    shopInput(&player, shop, 0, BUTTON_USE);
    CHECK(strstr(screens[0], "Price changed"));
    CHECK(Accounts_GetCredits(copy) == 1700);
    shopInput(&player, shop, 0, 0);
    shopInput(&player, shop, 0, BUTTON_USE);
    CHECK(Accounts_GetCredits(copy) == 1300);
    for (int i = 0; i < 5; i++) {
        shopInput(&player, shop, 0, 0);
        shopInput(&player, shop, -127, 0);
    }
    CHECK(strstr(screens[0], "Page 2/4"));
    shopInput(&player, shop, 0, BUTTON_ALT_ATTACK);
    CHECK(client.Lmd.lmdMenu.entityNum == 0);
    CHECK(!(player.flags & FL_GODMODE));

    // Each player keeps an independent catalogue and cursor at the same vendor.
    static gentity_t otherPlayer;
    static gclient_t otherClient;
    otherPlayer.inuse = qtrue;
    otherPlayer.client = &otherClient;
    otherPlayer.s.number = 1;
    otherPlayer.health = 100;
    otherClient.sess.sessionTeam = TEAM_FREE;
    otherClient.pers.Lmd.account = acc;
    otherClient.pers.Lmd.character = Account_GetActiveCharacter(acc);
    Accounts_Prof_SetLevel(acc, 40);
    SSkills_ShopOpen(shop, &player);
    SSkills_ShopOpen(shop, &otherPlayer);
    CHECK(strstr(screens[0], "Force Body"));
    CHECK(strstr(screens[1], "Slow Regeneration"));
    shopInput(&player, shop, 0, 0);
    shopInput(&player, shop, -127, 0);
    otherClient.Lmd.lmdMenu.nextUpdateTime = 0;
    shopInput(&otherPlayer, shop, 0, 0);
    CHECK(strstr(screens[1], "[ Slow Regeneration ]"));
    shopInput(&otherPlayer, shop, 0, BUTTON_ALT_ATTACK);
    CHECK(otherClient.Lmd.lmdMenu.entityNum == 0);
    CHECK(client.Lmd.lmdMenu.entityNum == shop->s.number);
    client.pers.Lmd.character = second;
    shopInput(&player, shop, 0, BUTTON_USE);
    CHECK(client.Lmd.lmdMenu.entityNum == 0);
    CHECK(Accounts_GetCredits(copy) == 1300);
    client.pers.Lmd.character = first;

    // Forced closure restores existing godmode and prevents remote purchases.
    player.flags |= FL_GODMODE;
    SSkills_ShopOpen(shop, &player);
    client.ps.origin[0] = 1000;
    shopInput(&player, shop, 0, BUTTON_USE);
    CHECK(client.Lmd.lmdMenu.entityNum == 0);
    CHECK(player.flags & FL_GODMODE);
    CHECK(Accounts_GetCredits(copy) == 1300);
    g_entities[0].client = &client;
    HiRuns_TimerStart(&g_entities[0], "disconnect-regression", 1);
    CHECK(client.Lmd.hiRuns.running);
    HiRuns_ClientDisconnect(&g_entities[0]);
    CHECK(!client.Lmd.hiRuns.running);
    HiRuns_TimerStart(&g_entities[0], "disconnect-regression", 1);
    CHECK(client.Lmd.hiRuns.running);
    HiRuns_ClientDisconnect(&g_entities[0]);
    g_entities[0].client = NULL;

    printf("Passed %d checks.\n", checks);
    return 0;
}
