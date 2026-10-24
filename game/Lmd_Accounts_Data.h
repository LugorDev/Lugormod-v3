
#include "Lmd_Data.h"

#include "Lmd_Accounts_Public.h"

typedef struct Character_s Character_t;

void Lmd_Accounts_Modify(Account_t *acc);
#define PlayerAcc_Modify(ent) Lmd_Accounts_Modify(ent->client->pers.Lmd.account)

int Lmd_Accounts_AddDataCategory(accDataModule_t *category);
void* Lmd_Accounts_GetAccountCategoryData(Account_t *acc, int categoryIndex);

// Per-character module storage (parallel registry to AddDataCategory).
int Lmd_Accounts_AddCharacterDataCategory(accDataModule_t *category);
void* Lmd_Accounts_GetCharacterCategoryData(Character_t *ch, int categoryIndex);
// Convenience: redirects to acc->activeCharacter's char-scope data. Used by
// modules that previously took Account_t* but are now per-character.
void* Lmd_Accounts_GetAccCharCategoryData(Account_t *acc, int categoryIndex);
