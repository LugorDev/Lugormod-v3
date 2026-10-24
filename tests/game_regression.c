// Exercises real account parsing/serialization and target logic through a mock
// engine syscall boundary. No server data or external files are read or written.
#include "../game/g_local.h"
#include "../game/Lmd_Accounts_Core.h"
#include "../game/Lmd_Accounts_CustomSkills.h"
#include "../game/Lmd_Accounts_Stats.h"
#include "../game/Lmd_Checksum.h"
#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
static LONG WINAPI reportException(EXCEPTION_POINTERS *exception) {
    char storage[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO *symbol = (SYMBOL_INFO *)storage;
    DWORD64 displacement;
    memset(storage, 0, sizeof(storage));
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

static char saved[65536];
static size_t savedLength;
static int failOpen;
static int checks;
#define CHECK(condition) do { checks++; if (!(condition)) { \
  fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); exit(1); } } while (0)

static int QDECL mockEngine(int call, ...) {
    va_list args;
    va_start(args, call);
    switch (call) {
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
    char powers[] = "push3.pull2";
    target.target2 = powers;
    Use_Target_Fp(&target, NULL, &player);
    CHECK(client.ps.fd.forcePowerLevel[FP_PUSH] == 3);
    CHECK(client.ps.fd.forcePowerLevel[FP_PULL] == 2);
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

    printf("Passed %d checks.\n", checks);
    return 0;
}
