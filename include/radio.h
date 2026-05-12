//
// Radio command handling for E-BOT.
//

#ifndef EBOT_RADIO_INCLUDED
#define EBOT_RADIO_INCLUDED

struct edict_s;
typedef struct edict_s edict_t;

enum RadioMessage
{
	RADIO_COVERME = 1,
	RADIO_YOUTAKEPOINT,
	RADIO_HOLDPOSITION,
	RADIO_REGROUPTEAM,
	RADIO_FOLLOWME,
	RADIO_TAKINGFIRE,

	RADIO_GOGOGO = 11,
	RADIO_FALLBACK,
	RADIO_STICKTOGETHER,
	RADIO_GETINPOSITION,
	RADIO_STORMTHEFRONT,
	RADIO_REPORTTEAM,

	RADIO_AFFIRMATIVE = 21,
	RADIO_ENEMYSPOTTED,
	RADIO_NEEDBACKUP,
	RADIO_SECTORCLEAR,
	RADIO_IMINPOSITION,
	RADIO_REPORTINGIN,
	RADIO_SHESGONNABLOW,
	RADIO_NEGATIVE,
	RADIO_ENEMYDOWN
};

void RadioClientCommand(edict_t* ent, const char* command, const char* arg1);
void RadioClientDisconnected(edict_t* ent);
void RadioClientDied(const int clientIndex);
void RadioUpdate(void);
void RadioResetAll(void);

#endif // EBOT_RADIO_INCLUDED
