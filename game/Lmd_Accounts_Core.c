


#include "g_local.h"
#include "Lmd_Accounts_Data.h"
#include "Lmd_Accounts_Core.h"
#include "Lmd_Commands_Auths.h"
#include "Lmd_Data.h"
#include "Lmd_Arrays.h"
#include "Lmd_Time.h"
#include "Lmd_Checksum.h"
#include "Lmd_IPs.h"
#include "BG_Fields.h"

#include "Lmd_Professions.h"

#define SECCODE_LENGTH 6

qboolean IsValidName(char *name);
Character_t *allocCharacter(Account_t *owner);
void freeCharacter(Character_t *ch);

struct Account_s{
	char *username;
	unsigned int pwChksum;
	char *secCode;

	int id;
	int logins;
	unsigned int lastLogin;
	IP_t lastIP;
	int flags;

	struct {
		unsigned int count; // account-scope module data
		void **data;
	} data;

	Character_t *characters[MAX_CHARS_PER_ACCOUNT];
	int numCharacters;
	Character_t *activeCharacter;
	qboolean migrated;      // set after legacy single-character file has been folded into characters[0]

	int modifiedTime;
};

struct Character_s {
	char *name;
	int credits;
	int bounty;
	int time;
	int score;
	int lastPlayed; // unix timestamp of most recent /play; used to auto-select on next /login
	Account_t *account;

	struct {
		unsigned int count; // char-scope module data
		void **data;
	} data;
};

#define	ACCOUNTOFS(x) ((int)&(((Account_t *)0)->x))
#define	CHAROFS(x) ((int)&(((Character_t *)0)->x))

struct {
	unsigned int count;
	accDataModule_t **categories;
} AccountDataTypes;

struct {
	unsigned int count;
	accDataModule_t **categories;
} CharacterDataTypes;

int Lmd_Accounts_AddDataCategory(accDataModule_t *category) {
	int newIndex = Lmd_Arrays_AddArrayElement((void **)&AccountDataTypes.categories, sizeof(accDataModule_t*), &AccountDataTypes.count);
	AccountDataTypes.categories[newIndex] = category;
	return newIndex;
}

int Lmd_Accounts_AddCharacterDataCategory(accDataModule_t *category) {
	int newIndex = Lmd_Arrays_AddArrayElement((void **)&CharacterDataTypes.categories, sizeof(accDataModule_t*), &CharacterDataTypes.count);
	CharacterDataTypes.categories[newIndex] = category;
	return newIndex;
}

void* Lmd_Accounts_GetAccountCategoryData(Account_t *acc, int categoryIndex) {
	if(!acc)
		return NULL;
	if (categoryIndex < 0 || categoryIndex >= AccountDataTypes.count) {
		G_Error("GetAccountCategoryData: Index out of range");
	}

	return acc->data.data[categoryIndex];
}

void* Lmd_Accounts_GetCharacterCategoryData(Character_t *ch, int categoryIndex) {
	if(!ch)
		return NULL;
	if (categoryIndex < 0 || categoryIndex >= CharacterDataTypes.count) {
		G_Error("GetCharacterCategoryData: Index out of range");
	}
	return ch->data.data[categoryIndex];
}

void* Lmd_Accounts_GetAccCharCategoryData(Account_t *acc, int categoryIndex) {
	if (!acc || !acc->activeCharacter)
		return NULL;
	return Lmd_Accounts_GetCharacterCategoryData(acc->activeCharacter, categoryIndex);
}


qboolean Accounts_Parse_Password(char *key, char *value, void *target, void *args) {
	Account_t *acc = (Account_t*)target;
	if(Q_stricmpn(value, "0x", 2) == 0){
		//skip the starting '0x'
		acc->pwChksum = HexToInt((value + 2));
	}
	else {
		acc->pwChksum = Checksum(value);
	}
	return qtrue;
}

DataWriteResult_t Accounts_Write_Password(void *target, char key[], int keySize, char value[], int valueSize, void **writeState, void *args) {
	Account_t *acc = (Account_t*) target;
	Q_strncpyz(value, va("0x%06x", acc->pwChksum), valueSize);
	return DWR_COMPLETE;
}

qboolean Accounts_Parse_LastLogin(char *key, char *value, void *target, void *args) {
	Account_t *acc = (Account_t*)target;
	acc->lastLogin = Time_ParseString(value);
	return qtrue;
}

DataWriteResult_t Accounts_Write_LastLogin(void *target, char key[], int keySize, char value[], int valueSize, void **writeState, void *args) {
	Account_t *acc = (Account_t*) target;
	Time_ToString(acc->lastLogin, value, valueSize);
	return DWR_COMPLETE;
}

qboolean Accounts_Parse_LastIP(char *key, char *value, void *target, void *args) {
	// TODO: Parse this in ip module.
	Account_t *acc = (Account_t*)target;
	Lmd_IPs_ParseIP(value, acc->lastIP);
	return qtrue; // Even if we failed to parse the ip, this is still our key.
}

DataWriteResult_t Accounts_Write_LastIP(void *target, char *key, int keySize, char *value, int valueSize, void **writeState, void *args) {
	Account_t *acc = (Account_t*) target;
	Q_strncpyz(value, Lmd_IPs_IPToString(acc->lastIP), valueSize);
	return DWR_COMPLETE;
}

void Accounts_Update_Override(char **key, char **value) {
	if (Q_stricmp(*key, "prof_merc_jetpack") == 0)
		*key = "prof_merc_fuel";
	else if (Q_stricmp(*key, "prof_merc_strength") == 0)
		*key = "prof_merc_forceresist";
}

qboolean Accounts_Parse_Modules(char *key, char *value, void *target, void *args) {
	Account_t *acc = (Account_t*) target;

	Accounts_Update_Override(&key, &value);

	int i;
	for (i = 0; i < AccountDataTypes.count; i++){
		accDataModule_t *module = AccountDataTypes.categories[i];
		void *dataPtr = acc->data.data[i];
		if (module->numDataFields > 0 &&
			Lmd_Data_Parse_KeyValuePair(key, value, dataPtr, module->dataFields, module->numDataFields))
		{
			return qtrue;
		}
	}

	return qfalse;
}

// Per-character module dispatch (used during [character] section parsing).
qboolean Characters_Parse_Modules(char *key, char *value, void *target, void *args) {
	Character_t *ch = (Character_t*) target;

	Accounts_Update_Override(&key, &value);

	int i;
	for (i = 0; i < CharacterDataTypes.count; i++) {
		accDataModule_t *module = CharacterDataTypes.categories[i];
		void *dataPtr = ch->data.data[i];
		if (module->numDataFields > 0 &&
			Lmd_Data_Parse_KeyValuePair(key, value, dataPtr, module->dataFields, module->numDataFields))
		{
			return qtrue;
		}
	}
	return qfalse;
}

struct AccountsWriteModulesState {
	int moduleIndex;
	int dataFieldIndex;
	void *dataFieldState;
};

DataWriteResult_t Accounts_Write_Modules(void *target, char key[], int keySize, char value[], int valueSize, void **writeState, void *args) {
	Account_t *acc = (Account_t*) target;

	struct AccountsWriteModulesState *state;
	accDataModule_t *module;

	if (*writeState == NULL) {
		state = (struct AccountsWriteModulesState *)G_Alloc(sizeof(struct AccountsWriteModulesState));
		state->moduleIndex = -1;
		*writeState = state;

		nextModule:
		// Find a good category
		state->dataFieldIndex = 0;
		state->dataFieldState = NULL;
		while (++state->moduleIndex < AccountDataTypes.count) {
			module = AccountDataTypes.categories[state->moduleIndex];

			if (module->numDataFields > 0) {
				// Found a category that wants to save.
				break;
			}
		}

	}
	else {
		state = (struct AccountsWriteModulesState *)*writeState;
	}


	if (state->moduleIndex >= AccountDataTypes.count) {
		// No more modules
		G_Free(state);
		return DWR_NODATA;
	}

	module = AccountDataTypes.categories[state->moduleIndex];

	nextField:
	if (state->dataFieldIndex < module->numDataFields) {
		// Write the field.
		const DataField_t *field = &module->dataFields[state->dataFieldIndex];
		if (field->write) {
			void *dataPtr = acc->data.data[state->moduleIndex];

			Q_strncpyz(key, field->key, keySize);
			DataWriteResult_t result = field->write(dataPtr, key, keySize, value, valueSize, &state->dataFieldState, field->writeArgs);
			if (result == DWR_COMPLETE || result == DWR_NODATA) {
				// We are done with this field
				state->dataFieldIndex++;
			}

			if (result == DWR_NODATA) {
				goto nextField;
			}

			return DWR_CONTINUE;
		}
		else {
			state->dataFieldIndex++;
			goto nextField;
		}
	}

	goto nextModule;
}

DataWriteResult_t Characters_Write_Modules(void *target, char key[], int keySize, char value[], int valueSize, void **writeState, void *args) {
	Character_t *ch = (Character_t*) target;

	struct AccountsWriteModulesState *state;
	accDataModule_t *module;

	if (*writeState == NULL) {
		state = (struct AccountsWriteModulesState *)G_Alloc(sizeof(struct AccountsWriteModulesState));
		state->moduleIndex = -1;
		*writeState = state;

		nextModule:
		state->dataFieldIndex = 0;
		state->dataFieldState = NULL;
		while (++state->moduleIndex < CharacterDataTypes.count) {
			module = CharacterDataTypes.categories[state->moduleIndex];
			if (module->numDataFields > 0) {
				break;
			}
		}
	}
	else {
		state = (struct AccountsWriteModulesState *)*writeState;
	}

	if (state->moduleIndex >= CharacterDataTypes.count) {
		G_Free(state);
		return DWR_NODATA;
	}

	module = CharacterDataTypes.categories[state->moduleIndex];

	nextField:
	if (state->dataFieldIndex < module->numDataFields) {
		const DataField_t *field = &module->dataFields[state->dataFieldIndex];
		if (field->write) {
			void *dataPtr = ch->data.data[state->moduleIndex];

			Q_strncpyz(key, field->key, keySize);
			DataWriteResult_t result = field->write(dataPtr, key, keySize, value, valueSize, &state->dataFieldState, field->writeArgs);
			if (result == DWR_COMPLETE || result == DWR_NODATA) {
				state->dataFieldIndex++;
			}
			if (result == DWR_NODATA) {
				goto nextField;
			}
			return DWR_CONTINUE;
		}
		else {
			state->dataFieldIndex++;
			goto nextField;
		}
	}

	goto nextModule;
}

// AccountFields_Base writes only account-scope state. Per-character fields
// (name/credits/bounty/time/score and char-scope module keys) live in
// CharacterFields_Base and are written inside [character] sections. Legacy single-
// character .uac files have those keys at the top level; the parser routes them to
// a temporary Character_t when they don't match account fields.
#define AccountFields_Base(_m) \
	_m##_FUNC(password, Accounts_Parse_Password, Accounts_Write_Password, NULL)	\
	_m##_AUTO(seccode, ACCOUNTOFS(secCode), F_QSTRING) \
	_m##_AUTO(id, ACCOUNTOFS(id), F_INT) \
	_m##_AUTO(logins, ACCOUNTOFS(logins), F_INT) \
	_m##_FUNC(lastlogin, Accounts_Parse_LastLogin, Accounts_Write_LastLogin, NULL) \
	_m##_FUNC(lastip, Accounts_Parse_LastIP, Accounts_Write_LastIP, NULL) \
	_m##_AUTO(flags, ACCOUNTOFS(flags), F_INT) \
	_m##_DEFL(Accounts_Parse_Modules, Accounts_Write_Modules, NULL)

AccountFields_Base(DEFINE_FIELD_PRE)

DATAFIELDS_BEGIN(AccountFields)
AccountFields_Base(DEFINE_FIELD_LIST)
DATAFIELDS_END

const int AccountFields_Count = DATAFIELDS_COUNT(AccountFields);

#define CharacterFields_Base(_m) \
	_m##_AUTO(name, CHAROFS(name), F_QSTRING) \
	_m##_AUTO(credits, CHAROFS(credits), F_INT) \
	_m##_AUTO(bounty, CHAROFS(bounty), F_INT) \
	_m##_AUTO(time, CHAROFS(time), F_INT) \
	_m##_AUTO(score, CHAROFS(score), F_INT) \
	_m##_AUTO(lastPlayed, CHAROFS(lastPlayed), F_INT) \
	_m##_DEFL(Characters_Parse_Modules, Characters_Write_Modules, NULL)

CharacterFields_Base(DEFINE_FIELD_PRE)

DATAFIELDS_BEGIN(CharacterFields)
CharacterFields_Base(DEFINE_FIELD_LIST)
DATAFIELDS_END

const int CharacterFields_Count = DATAFIELDS_COUNT(CharacterFields);

struct {
	unsigned int count;
	Account_t **accounts;
}AccList;

unsigned int nextId = 1;

unsigned int Accounts_Count() {
	return AccList.count;
}

Account_t* Accounts_Get(unsigned int i) {
	assert(i < AccList.count);
	if( i < 0 || i >= AccList.count ) {
		return NULL;
	}
	return AccList.accounts[i];
}

Account_t *Accounts_GetById(int id) {
	int i;
	for(i = 0; i < AccList.count; i++) {
		if(AccList.accounts[i]->id == id)
			return AccList.accounts[i];
	}
	return NULL;
}

Account_t *Accounts_GetByUsername(char *str) {
	int i;
	for(i = 0; i < AccList.count; i++) {
		if(Q_stricmp(AccList.accounts[i]->username, str) == 0)
			return AccList.accounts[i];
	}
	return NULL;
}

Account_t *Accounts_GetByName(char *str) {
	int i, c;
	for(i = 0; i < AccList.count; i++) {
		Account_t *acc = AccList.accounts[i];
		for (c = 0; c < acc->numCharacters; c++) {
			if (acc->characters[c] && acc->characters[c]->name &&
				Q_stricmpname(acc->characters[c]->name, str) == 0)
				return acc;
		}
	}
	return NULL;
}

Character_t *Accounts_GetCharacterByName(char *str) {
	int i, c;
	for (i = 0; i < AccList.count; i++) {
		Account_t *acc = AccList.accounts[i];
		for (c = 0; c < acc->numCharacters; c++) {
			Character_t *ch = acc->characters[c];
			if (ch && ch->name && Q_stricmpname(ch->name, str) == 0)
				return ch;
		}
	}
	return NULL;
}

Character_t *Account_GetCharacter(Account_t *acc, int index) {
	if (!acc || index < 0 || index >= acc->numCharacters)
		return NULL;
	return acc->characters[index];
}

int Account_GetNumCharacters(Account_t *acc) {
	if (!acc) return 0;
	return acc->numCharacters;
}

Character_t *Account_GetActiveCharacter(Account_t *acc) {
	if (!acc) return NULL;
	return acc->activeCharacter;
}

void Account_SetActiveCharacter(Account_t *acc, Character_t *ch) {
	if (!acc) return;
	acc->activeCharacter = ch;
}

Character_t *Account_FindCharacterByName(Account_t *acc, char *name) {
	if (!acc || !name) return NULL;
	int i;
	for (i = 0; i < acc->numCharacters; i++) {
		Character_t *ch = acc->characters[i];
		if (ch && ch->name && Q_stricmpname(ch->name, name) == 0)
			return ch;
	}
	return NULL;
}

Character_t *Account_GetMostRecentCharacter(Account_t *acc) {
	if (!acc || acc->numCharacters <= 0) return NULL;
	Character_t *best = acc->characters[0];
	int i;
	for (i = 1; i < acc->numCharacters; i++) {
		Character_t *ch = acc->characters[i];
		if (ch && ch->lastPlayed > best->lastPlayed)
			best = ch;
	}
	return best;
}

Character_t *Account_NewCharacter(Account_t *acc, char *name) {
	if (!acc || !name)
		return NULL;
	if (acc->numCharacters >= MAX_CHARS_PER_ACCOUNT)
		return NULL;
	if (Accounts_GetCharacterByName(name) != NULL)
		return NULL;
	Character_t *ch = allocCharacter(acc);
	ch->name = G_NewString2(name);
	acc->characters[acc->numCharacters++] = ch;
	int i;
	for (i = 0; i < CharacterDataTypes.count; i++) {
		accDataModule_t *category = CharacterDataTypes.categories[i];
		if (category->accLoadCompleted == NULL)
			continue;
		category->accLoadCompleted(acc, ch->data.data[i]);
	}
	Lmd_Accounts_Modify(acc);
	return ch;
}

qboolean Account_DeleteCharacter(Account_t *acc, Character_t *ch) {
	if (!acc || !ch)
		return qfalse;
	int i, found = -1;
	for (i = 0; i < acc->numCharacters; i++) {
		if (acc->characters[i] == ch) {
			found = i;
			break;
		}
	}
	if (found < 0)
		return qfalse;
	if (acc->activeCharacter == ch)
		acc->activeCharacter = NULL;
	for (i = found; i < acc->numCharacters - 1; i++) {
		acc->characters[i] = acc->characters[i + 1];
	}
	acc->characters[acc->numCharacters - 1] = NULL;
	acc->numCharacters--;
	freeCharacter(ch);
	Lmd_Accounts_Modify(acc);
	return qtrue;
}

qboolean Account_MoveCharacter(Account_t *src, Character_t *ch, Account_t *dst) {
	if (!src || !dst || !ch)
		return qfalse;
	if (dst->numCharacters >= MAX_CHARS_PER_ACCOUNT)
		return qfalse;
	int i, found = -1;
	for (i = 0; i < src->numCharacters; i++) {
		if (src->characters[i] == ch) { found = i; break; }
	}
	if (found < 0)
		return qfalse;
	if (src->activeCharacter == ch)
		src->activeCharacter = NULL;
	for (i = found; i < src->numCharacters - 1; i++)
		src->characters[i] = src->characters[i + 1];
	src->characters[src->numCharacters - 1] = NULL;
	src->numCharacters--;

	dst->characters[dst->numCharacters++] = ch;
	ch->account = dst;
	Lmd_Accounts_Modify(src);
	Lmd_Accounts_Modify(dst);
	return qtrue;
}

unsigned int Characters_Count() {
	unsigned int total = 0;
	int i;
	for (i = 0; i < AccList.count; i++)
		total += AccList.accounts[i]->numCharacters;
	return total;
}

Character_t *Characters_Get(unsigned int idx) {
	int i;
	for (i = 0; i < AccList.count; i++) {
		Account_t *acc = AccList.accounts[i];
		if ((int)idx < acc->numCharacters)
			return acc->characters[idx];
		idx -= acc->numCharacters;
	}
	return NULL;
}

Account_t *Character_GetAccount(Character_t *ch) {
	if (!ch) return NULL;
	return ch->account;
}

char *Character_GetName(Character_t *ch) {
	if (!ch) return NULL;
	if (IsValidName(ch->name) == qfalse)
		return "Padawan";
	return ch->name;
}

void Character_SetName(Character_t *ch, char *name) {
	if (!ch) return;
	if (IsValidName(name) == qfalse)
		name = "Padawan";
	G_Free(ch->name);
	ch->name = G_NewString2(name);
	if (ch->account)
		Lmd_Accounts_Modify(ch->account);
}

int Character_GetCredits(Character_t *ch) { return ch ? ch->credits : 0; }
void Character_SetCredits(Character_t *ch, int v) {
	if (!ch) return;
	if (v < 0) v = 0;
	ch->credits = v;
	if (ch->account) Lmd_Accounts_Modify(ch->account);
}
int Character_GetBounty(Character_t *ch) { return ch ? ch->bounty : 0; }
void Character_SetBounty(Character_t *ch, int v) {
	if (!ch) return;
	ch->bounty = v;
	if (ch->account) Lmd_Accounts_Modify(ch->account);
}
int Character_GetTime(Character_t *ch) { return ch ? ch->time : 0; }
void Character_SetTime(Character_t *ch, int v) {
	if (!ch) return;
	ch->time = v;
	if (ch->account) Lmd_Accounts_Modify(ch->account);
}
int Character_GetScore(Character_t *ch) { return ch ? ch->score : 0; }
void Character_SetScore(Character_t *ch, int v) {
	if (!ch) return;
	ch->score = v;
	if (ch->account) Lmd_Accounts_Modify(ch->account);
}

void Character_StampLastPlayed(Character_t *ch) {
	if (!ch) return;
	ch->lastPlayed = Time_Now();
	if (ch->account) Lmd_Accounts_Modify(ch->account);
}

gentity_t *Accounts_GetPlayerByAcc(Account_t *acc) {
	int i;
	for(i = 0; i < MAX_CLIENTS; i++) {
		if(!g_entities[i].inuse || !g_entities[i].client)
			continue;
		if(g_entities[i].client->pers.Lmd.account == acc)
			return &g_entities[i];
	}
	return NULL;
}

Character_t *allocCharacter(Account_t *owner) {
	Character_t *ch = (Character_t *)G_Alloc(sizeof(Character_t));
	memset(ch, 0, sizeof(*ch));
	ch->account = owner;
	ch->data.count = CharacterDataTypes.count;
	ch->data.data = (void **)malloc(sizeof(void *) * CharacterDataTypes.count);
	int i;
	for (i = 0; i < CharacterDataTypes.count; i++) {
		accDataModule_t *category = CharacterDataTypes.categories[i];
		void *dataPtr = ch->data.data[i] = G_Alloc(category->dataSize);
		memset(dataPtr, 0, category->dataSize);
		if (category->allocData) {
			category->allocData(dataPtr);
		}
	}
	return ch;
}

void freeCharacter(Character_t *ch) {
	int i;
	Lmd_Data_FreeFields((void*)ch, CharacterFields, CharacterFields_Count);
	for (i = 0; i < CharacterDataTypes.count; i++) {
		accDataModule_t *module = CharacterDataTypes.categories[i];
		void *dataPtr = ch->data.data[i];
		if (dataPtr) {
			Lmd_Data_FreeFields(dataPtr, module->dataFields, module->numDataFields);
			if (module->freeData)
				module->freeData(dataPtr);
			G_Free(dataPtr);
		}
	}
	Lmd_Arrays_RemoveAllElements((void **)&ch->data.data);
	G_Free(ch);
}

Account_t *allocAccount(){
	int i;
	Account_t *acc = (Account_t *)G_Alloc(sizeof(Account_t));
	memset(acc, 0, sizeof(*acc));
	acc->data.count = AccountDataTypes.count;
	acc->data.data = (void **)malloc(sizeof(void *) * AccountDataTypes.count);
	for(i = 0; i < AccountDataTypes.count; i++) {
		accDataModule_t *category = AccountDataTypes.categories[i];
		void *dataPtr = acc->data.data[i] = G_Alloc(category->dataSize);
		memset(dataPtr, 0, category->dataSize);
		if(category->allocData) {
			category->allocData(dataPtr);
		}
	}

	return acc;
}

void freeAccount(Account_t *acc) {
	int i;

	Lmd_Data_FreeFields((void*)acc, AccountFields, AccountFields_Count);

	for(i = 0; i < AccountDataTypes.count; i++) {
		accDataModule_t *module = AccountDataTypes.categories[i];
		void *dataPtr = acc->data.data[i];

		if (dataPtr) {
			Lmd_Data_FreeFields(dataPtr, module->dataFields, module->numDataFields);
			if (module->freeData)
				module->freeData(dataPtr);
			G_Free(dataPtr);
		}
	}

	for (i = 0; i < acc->numCharacters; i++) {
		if (acc->characters[i])
			freeCharacter(acc->characters[i]);
	}

	Lmd_Arrays_RemoveAllElements((void **)&acc->data.data);
	G_Free(acc->username);
	G_Free(acc);
}

void addAccount(Account_t *acc) {
	int i = Lmd_Arrays_AddArrayElement((void **)&AccList.accounts, sizeof(Account_t *), &AccList.count);
	AccList.accounts[i] = acc;
	if(acc->id >= nextId)
		nextId = acc->id + 1;
}

void removeAccount(Account_t *acc) {
	int i, index = -1;
	for(i = 0; i < AccList.count; i++) {
		if(AccList.accounts[i] == acc) {
			index = i;
			break;
		}
	}
	assert(index > -1);
	Lmd_Arrays_RemoveArrayElement((void **)&AccList.accounts, index, sizeof(Account_t *), &AccList.count);
}

void deleteAccount(Account_t *acc) {
	Lmd_Data_DeleteFile("accounts", va("%s.uac", acc->username));
	freeAccount(acc);
}

void Lmd_Accounts_Player_Logout(gentity_t *ent);
void Accounts_Delete(Account_t *acc) {
	gentity_t *player = Accounts_GetPlayerByAcc(acc);
	if(player)
		Lmd_Accounts_Player_Logout(player);
	removeAccount(acc);
	deleteAccount(acc);
}

void Accounts_Save(Account_t *acc)
{
	fileHandle_t f = Lmd_Data_OpenDataFile("accounts", va("%s.uac", acc->username), FS_WRITE);
	Lmd_Data_WriteToFile_LinesDelimited(f, AccountFields, AccountFields_Count, (void *)acc);

	int i;
	for (i = 0; i < acc->numCharacters; i++) {
		Character_t *ch = acc->characters[i];
		if (!ch)
			continue;
		const char *open = "[character]\n";
		const char *close = "[/character]\n";
		trap_FS_Write(open, strlen(open), f);
		Lmd_Data_WriteToFile_LinesDelimited(f, CharacterFields, CharacterFields_Count, (void *)ch);
		trap_FS_Write(close, strlen(close), f);
	}

	trap_FS_FCloseFile(f);

	acc->modifiedTime = 0;
}

void updatePlayer(gentity_t *ent);
void Accounts_SaveAll(qboolean force){
	int i;
	gentity_t *player;
	for(i = 0; i < AccList.count; i++){
		if(AccList.accounts[i]->modifiedTime > 0 && (force || 
			level.time - AccList.accounts[i]->modifiedTime > 60000)) {
				player = Accounts_GetPlayerByAcc(AccList.accounts[i]);
				if(player)
					updatePlayer(player);
				Accounts_Save(AccList.accounts[i]);
		}
	}
}

extern vmCvar_t lmd_accBaseDays;
extern vmCvar_t lmd_accLevelDays;
extern vmCvar_t lmd_accMaxDays;

//Should this be in the profession area?
int accountLiveTime(int level) {
	int keep = lmd_accBaseDays.integer + (level * lmd_accLevelDays.integer);
	if(keep == 0)
		return 0;
	if(lmd_accMaxDays.integer && keep > lmd_accMaxDays.integer)
		keep = lmd_accMaxDays.integer;
	return keep;
}

qboolean accountLiveTimeCheck(Account_t *acc) {
	int now = Time_Days(Time_Now());
	int keep = accountLiveTime(Accounts_Prof_GetLevel(acc));
	if(now - Time_Days(acc->lastLogin) > keep)
		return qfalse;
	return qtrue;
}

qboolean validateNewAccount(Account_t *acc) {
	if(acc->id <= 0)
		return qfalse;
	if(Accounts_GetById(acc->id) != NULL)
		return qfalse;

	if(!accountLiveTimeCheck(acc) && Auths_AccHasAdmin(acc) == qfalse)
		return qfalse;

	return qtrue;
}

qboolean parseAccount(char *name, char *buf){
	Account_t *acc = allocAccount();
	if(g_developer.integer > 0)
		Com_Printf("Loading account: %s\n", name);
	acc->username = G_NewString2(name); //guarenteed to be unique, since it's a filename.

	// During load, char-scope module keys appearing at top level (legacy files) need
	// to land somewhere. We allocate a "legacy character" up front to receive them,
	// and only keep it if no real [character] sections appear.
	Character_t *legacyCh = allocCharacter(acc);

	char *str = buf;
	char *p;
	char key[MAX_STRING_CHARS], value[MAX_STRING_CHARS];
	Character_t *currentChar = NULL;
	qboolean sawCharacterSection = qfalse;

	while (str && *str) {
		p = COM_ParseExt((const char **)&str, qtrue);
		if (!p[0])
			break;

		// Section markers — bare tokens with no `key:` colon.
		if (Q_stricmp(p, "[character]") == 0) {
			sawCharacterSection = qtrue;
			if (acc->numCharacters < MAX_CHARS_PER_ACCOUNT) {
				currentChar = allocCharacter(acc);
				acc->characters[acc->numCharacters++] = currentChar;
			}
			else {
				currentChar = NULL; // overflow: skip
			}
			continue;
		}
		if (Q_stricmp(p, "[/character]") == 0) {
			currentChar = NULL;
			continue;
		}

		Q_strncpyz(key, p, sizeof(key));
		int colPos = strlen(key) - 1;
		if (colPos <= 0)
			continue;
		char *valuePtr;
		if (key[colPos] == ':') {
			key[colPos] = 0;
			Q_strncpyz(value, COM_ParseLine((const char **)&str), sizeof(value));
			valuePtr = value;
		}
		else {
			valuePtr = NULL;
		}

		if (currentChar) {
			Lmd_Data_Parse_KeyValuePair(key, valuePtr, currentChar, CharacterFields, CharacterFields_Count);
		}
		else {
			// Try account fields first (covers legacy per-char keys via AccountFields_Legacy).
			if (!Lmd_Data_Parse_KeyValuePair(key, valuePtr, acc, AccountFields, AccountFields_Count)) {
				// Legacy char-scope module data lived at account scope. Route to the
				// pre-allocated legacy character so its modules receive the data.
				Lmd_Data_Parse_KeyValuePair(key, valuePtr, legacyCh, CharacterFields, CharacterFields_Count);
			}
		}
	}

	if (!sawCharacterSection) {
		// Legacy file: legacyCh already absorbed name/credits/bounty/time/score and any
		// char-scope module data from the top-level fallthrough. Promote it.
		acc->characters[0] = legacyCh;
		acc->numCharacters = 1;
		acc->activeCharacter = legacyCh;
		acc->migrated = qtrue;
		// Mark dirty so the new sectioned format lands on disk at next save.
		// level.time is 0 during startup load; 1 ensures the >0 gate passes.
		acc->modifiedTime = level.time > 0 ? level.time : 1;
	}
	else {
		freeCharacter(legacyCh);
		if (acc->numCharacters > 0)
			acc->activeCharacter = acc->characters[0];
	}

	if(validateNewAccount(acc)) {
		addAccount(acc);
	}
	else {
		deleteAccount(acc);
	}

	return qtrue;
}

unsigned int Accounts_Load(){
	unsigned int result = Lmd_Data_ProcessFiles("accounts", ".uac", parseAccount, Q3_INFINITE);
	int i, a, c;
	for(i = 0; i < AccountDataTypes.count; i++){
		accDataModule_t *module = AccountDataTypes.categories[i];
		if(module->accLoadCompleted == NULL)
			continue;
		for(a = 0; a < AccList.count; a++) {
			module->accLoadCompleted(AccList.accounts[a], AccList.accounts[a]->data.data[i]);
		}
	}
	for (i = 0; i < CharacterDataTypes.count; i++) {
		accDataModule_t *module = CharacterDataTypes.categories[i];
		if (module->accLoadCompleted == NULL)
			continue;
		for (a = 0; a < AccList.count; a++) {
			Account_t *acc = AccList.accounts[a];
			for (c = 0; c < acc->numCharacters; c++) {
				module->accLoadCompleted(acc, acc->characters[c]->data.data[i]);
			}
		}
	}

	return result;
}

Account_t *Accounts_New(char *username, char *name, char *password) {
	if(Accounts_GetByUsername(username) || Accounts_GetByName(name))
		return NULL;
	Account_t *acc = allocAccount();
	acc->username = G_NewString2(username);
	acc->modifiedTime = level.time;
	acc->pwChksum = Checksum(password);
	acc->id = nextId;
	int i;
	for(i = 0; i < AccountDataTypes.count; i++){
		accDataModule_t *category = AccountDataTypes.categories[i];
		if(category->accLoadCompleted == NULL)
			continue;
		category->accLoadCompleted(acc, acc->data.data[i]);
	}

	// New accounts get character[0] from the registration name.
	Character_t *ch = allocCharacter(acc);
	ch->name = G_NewString2(name);
	acc->characters[0] = ch;
	acc->numCharacters = 1;
	acc->activeCharacter = ch;
	for (i = 0; i < CharacterDataTypes.count; i++) {
		accDataModule_t *category = CharacterDataTypes.categories[i];
		if (category->accLoadCompleted == NULL)
			continue;
		category->accLoadCompleted(acc, ch->data.data[i]);
	}

	addAccount(acc);
	return acc;
}

void Lmd_Accounts_Modify(Account_t *acc) {
	if(acc->modifiedTime == 0)
		acc->modifiedTime = level.time;
}

/*
=======================================================================================
This could be moved into a seperate account module file.
Probably should be, too.
=======================================================================================
*/

int Accounts_GetId(Account_t *acc) {
	if(!acc)
		return 0;
	return acc->id;
}

char* Accounts_GetUsername(Account_t *acc) {
	if(!acc)
		return NULL;
	return acc->username;
}

char* Accounts_GetName(Account_t *acc) {
	if(!acc || !acc->activeCharacter)
		return NULL;
	return Character_GetName(acc->activeCharacter);
}

void Accounts_SetName(Account_t *acc, char *name) {
	if(!acc || !acc->activeCharacter)
		return;
	Character_SetName(acc->activeCharacter, name);
}

unsigned int Accounts_GetPassword(Account_t *acc) {
	if(!acc)
		return 0;
	return acc->pwChksum;
}

void Accounts_SetPassword(Account_t *acc, char *password) {
	if(!acc)
		return;
	acc->pwChksum = Checksum(password);
	Lmd_Accounts_Modify(acc);
}

char* Accounts_GetSeccode(Account_t *acc) {
	if(!acc)
		return "";
	return acc->secCode;
}

void Accounts_ClearSeccode(Account_t *acc) {
	if(!acc)
		return;
	G_Free(acc->secCode);
	acc->secCode = NULL;
	Lmd_Accounts_Modify(acc);
}

char* Accounts_NewSeccode(Account_t *acc) {
	if(!acc) {
		assert(!"New security code for no account.");
		return "";
	}
	char code[SECCODE_LENGTH];
	int i;
	int r;
	for(i = 0; i < SECCODE_LENGTH; i++) {
		r = Q_irand(0, 1);
		if(r == 0)
			code[i] = Q_irand('1', '9'); //skip 0, might look like O
		else if(r == 1) { 
			if(Q_irand(0, 1) == 0) //in jka font, I looks like l
				code[i] = Q_irand('A', 'K');
			else
				code[i] = Q_irand('M', 'Z');
		}
	}
	code[SECCODE_LENGTH - 1] = 0;
	G_Free(acc->secCode);
	acc->secCode = G_NewString2(code);
	Lmd_Accounts_Modify(acc);
	return acc->secCode;
}

int Accounts_GetBounty(Account_t *acc)
{
	if(!acc || !acc->activeCharacter) return 0;
	return acc->activeCharacter->bounty;
}

void Accounts_SetBounty(Account_t *acc, int value)
{
	if(!acc || !acc->activeCharacter) return;
	Character_SetBounty(acc->activeCharacter, value);
}

void Accounts_PrintBountyList(gentity_t* ent)
{
	qboolean found = qfalse;
	unsigned int total = Characters_Count();
	unsigned int i;

	for (i = 0; i < total; i++) {
		Character_t *ch = Characters_Get(i);
		if (!ch) continue;
		int bounty = Character_GetBounty(ch);
		if (bounty > 0) {
			Disp(ent, va("^7%s ^5- ^6%d ^5CR", Character_GetName(ch), bounty));
			found = qtrue;
		}
	}

	if (!found)
	{
		Disp(ent, "^5No bounties currently placed.");
	}
}

int Accounts_GetCredits(Account_t *acc) {
	if(!acc || !acc->activeCharacter)
		return 0;
	return acc->activeCharacter->credits;
}

void Accounts_SetCredits(Account_t *acc, int value) {
	if(!acc || !acc->activeCharacter)
		return;
	Character_SetCredits(acc->activeCharacter, value);
}

int Accounts_GetScore(Account_t *acc) {
	if(!acc || !acc->activeCharacter)
		return 0;
	return acc->activeCharacter->score;
}

void Accounts_SetScore(Account_t *acc, int value) {
	if(!acc || !acc->activeCharacter)
		return;
	Character_SetScore(acc->activeCharacter, value);
}

int Accounts_GetTime(Account_t *acc) {
	if(!acc || !acc->activeCharacter)
		return 0;
	return acc->activeCharacter->time;
}

void Accounts_SetTime(Account_t *acc, int value) {
	if(!acc || !acc->activeCharacter)
		return;
	Character_SetTime(acc->activeCharacter, value);
}

// lumaya Titles:
#define TITLES_FILE "prof_titles.txt"
#define MAX_TITLE_LENGTH 32
#define DEFAULT_TITLE "Unknown"

typedef struct {
	char jedi_titles[5][MAX_TITLE_LENGTH];
	char sith_titles[5][MAX_TITLE_LENGTH];
	char merc_titles[5][MAX_TITLE_LENGTH];
} LmdTitleData_t;

static LmdTitleData_t lmd_titleData;
static int lmd_titlesLoaded = 0;

void Accounts_CreateDefaultTitlesFile(void) {
	fileHandle_t f;
	const char *defaults =
		"# Jedi Titles (Level 1-9, 10-19, 20-29, 30-39, 40)\n"
		"JEDI,Youngling,Jedi Padawan,Jedi Knight,Jedi Master,Grand Master\n"
		"# Sith Titles\n"
		"SITH,Initiate,Sith Acolyte,Sith Apprentice,Sith Warrior,Sith Lord\n"
		"# Merc Titles\n"
		"MERC,Rookie,Hired Gun,Outlaw,Bounty Hunter,Elite Enforcer\n";

	f = Lmd_Data_OpenDataFile(NULL, TITLES_FILE, FS_WRITE);
	if (!f) {
		G_Printf("Failed to create default titles file\n");
		return;
	}
	trap_FS_Write(defaults, strlen(defaults), f);
	trap_FS_FCloseFile(f);
	G_Printf("Created default titles file\n");
}



int Accounts_LoadTitles(void) {
	char *file = Lmd_Data_AllocFileContents(TITLES_FILE);
	if (!file) {
		G_Printf("Titles file not found, creating default...\n");
		Accounts_CreateDefaultTitlesFile();
		file = Lmd_Data_AllocFileContents(TITLES_FILE);
		if (!file) {
			G_Printf("Failed to open titles file\n");
			return 0;
		}
	}

	for (int i = 0; i < 5; i++) {
		Q_strncpyz(lmd_titleData.jedi_titles[i], DEFAULT_TITLE, MAX_TITLE_LENGTH);
		Q_strncpyz(lmd_titleData.sith_titles[i], DEFAULT_TITLE, MAX_TITLE_LENGTH);
		Q_strncpyz(lmd_titleData.merc_titles[i], DEFAULT_TITLE, MAX_TITLE_LENGTH);
	}

	char *line = strtok(file, "\n");
	while (line != NULL) {
		if (line[0] == '#' || line[0] == '\0') {
			line = strtok(NULL, "\n");
			continue;
		}
		
		char *type = strtok(line, ",");
		if (!type) {
			line = strtok(NULL, "\n");
			continue;
		}

		char *titles[6] = {0};
		for (int i = 0; i < 6; i++) {
			titles[i] = strtok(NULL, ",");
		}

		if (!titles[0] || !titles[4]) {
			line = strtok(NULL, "\n");
			continue; // invalid line
		}

		char (*dest)[MAX_TITLE_LENGTH] = NULL;

		if (!Q_stricmp(type, "JEDI")) dest = lmd_titleData.jedi_titles;
		else if (!Q_stricmp(type, "SITH")) dest = lmd_titleData.sith_titles;
		else if (!Q_stricmp(type, "MERC")) dest = lmd_titleData.merc_titles;
		else {
			line = strtok(NULL, "\n");
			continue;
		}

		for (int i = 0; i < 5; i++) {
			if (titles[i]) {
				Q_strncpyz(dest[i], titles[i], MAX_TITLE_LENGTH);
			}
		}

		line = strtok(NULL, "\n");
	}

	G_Free(file);
	lmd_titlesLoaded = 1;
	G_Printf("Titles loaded successfully\n");
	return 1;
}

static int GetTitleIndex(int levelp)
{
	if (levelp <= 9) return 0;
	if (levelp <= 19) return 1;
	if (levelp <= 29) return 2;
	if (levelp <= 39) return 3;
	return 4;
}

extern int Jedi_GetAccSide(Account_t *acc);
char* Accounts_GetTitle(Account_t *acc)
{
	if (!acc) return DEFAULT_TITLE;
	if (!lmd_titlesLoaded && !Accounts_LoadTitles()) return DEFAULT_TITLE;

	int level = Accounts_Prof_GetLevel(acc);
	if (level < 1) level = 1;
	if (level > 40) level = 40;

	int index = GetTitleIndex(level);
	int prof = Accounts_Prof_GetProfession(acc);

	if (prof == PROF_JEDI) {
		int side = Jedi_GetAccSide(acc);
		if (side == FORCE_LIGHTSIDE) return lmd_titleData.jedi_titles[index];
		if (side == FORCE_DARKSIDE) return lmd_titleData.sith_titles[index];
		return "None";
	}

	if (prof == PROF_MERC)
		return lmd_titleData.merc_titles[index];

	return DEFAULT_TITLE;
}

int Accounts_GetFlags(Account_t *acc) {
	if(!acc)
		return 0;
	return acc->flags;
}

void Accounts_AddFlags(Account_t *acc, int flags) {
	if(!acc)
		return;

	if(flags > 0) {
		acc->flags |= flags;
	}
	else {
		acc->flags &= ~(-flags);
	}

	Lmd_Accounts_Modify(acc);
}

int Accounts_GetLogins(Account_t *acc) {
	if(!acc)
		return 0;
	return acc->logins;
}

void Accounts_SetLogins(Account_t *acc, int value) {
	if(!acc)
		return;
	acc->logins = value;
	Lmd_Accounts_Modify(acc);
}

void Accounts_GetLastIp(Account_t *acc, IP_t value) {
	memcpy(value, acc->lastIP, sizeof(IP_t));
}

void Accounts_SetLastIp(Account_t *acc, IP_t value) {
	if(!acc)
		return;
	memcpy(acc->lastIP, value, sizeof(IP_t));
	Lmd_Accounts_Modify(acc);
}

int Accounts_GetLastLogin(Account_t *acc) {
	if(!acc)
		return 0;
	return acc->lastLogin;
}

void Accounts_SetLastLogin(Account_t *acc, int value) {
	if(!acc)
		return;
	acc->lastLogin = value;
	Lmd_Accounts_Modify(acc);
}
