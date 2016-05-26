
#ifdef XCVAR_PROTO
#define XCVAR_DEF( name, defVal, update, flags, announce ) extern vmCvar_t name;
#endif

#ifdef XCVAR_DECL
#define XCVAR_DEF( name, defVal, update, flags, announce ) vmCvar_t name;
#endif

#ifdef XCVAR_LIST
#define XCVAR_DEF( name, defVal, update, flags, announce ) { & name , #name , defVal , update , flags , announce },
#endif

XCVAR_DEF( bg_fighterAltControl, "0", NULL, CVAR_SERVERINFO, qtrue )
XCVAR_DEF( bot_addDelay, "3", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( bot_maxbots, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( bot_minplayers, "0", NULL, CVAR_SERVERINFO, qtrue )
XCVAR_DEF( capturelimit, "8", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, qtrue )
XCVAR_DEF( com_optvehtrace, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( d_altRoutes, "0", NULL, CVAR_CHEAT, qfalse )
XCVAR_DEF( d_asynchronousGroupAI, "0", NULL, CVAR_CHEAT, qfalse )
XCVAR_DEF( d_JediAI, "0", NULL, CVAR_CHEAT, qfalse )
XCVAR_DEF( d_noGroupAI, "0", NULL, CVAR_CHEAT, qfalse )
XCVAR_DEF( d_noIntermissionWait, "0", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( d_noroam, "0", NULL, CVAR_CHEAT, qfalse )
XCVAR_DEF( d_npcai, "0", NULL, CVAR_CHEAT, qfalse )
XCVAR_DEF( d_npcaiming, "0", NULL, CVAR_CHEAT, qfalse )
XCVAR_DEF( d_npcfreeze, "0", NULL, CVAR_CHEAT, qfalse )
XCVAR_DEF( d_patched, "0", NULL, CVAR_CHEAT, qfalse )
XCVAR_DEF( d_perPlayerGhoul2, "0", NULL, CVAR_CHEAT, qtrue )
XCVAR_DEF( d_powerDuelPrint, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( d_projectileGhoul2Collision, "1", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( d_saberAlwaysBoxTrace, "0", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( d_saberBoxTraceSize, "0", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( d_saberCombat, "0", NULL, CVAR_CHEAT, qfalse )
XCVAR_DEF( d_saberGhoul2Collision, "1", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( d_saberInterpolate, "0", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( d_saberKickTweak, "1", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( d_saberSPStyleDamage, "1", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( d_saberStanceDebug, "0", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( d_siegeSeekerNPC, "0", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( dedicated, "0", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( developer, "0", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( dmflags, "0", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE, qtrue )
XCVAR_DEF( duel_fraglimit, "10", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, qtrue )
XCVAR_DEF( fraglimit, "20", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, qtrue )
XCVAR_DEF( g_adaptRespawn, "1", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( g_allowDuelSuicide, "1", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_allowHighPingDuelist, "1", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( g_allowNPC, "1", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_allowVote, "-1", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_armBreakage, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( g_austrian, "0", NULL, CVAR_ARCHIVE, qfalse )
XCVAR_DEF( g_autoMapCycle, "0", NULL, CVAR_ARCHIVE | CVAR_NORESTART, qtrue )
XCVAR_DEF( g_debugAlloc, "0", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( g_debugDamage, "0", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( g_debugMelee, "0", NULL, CVAR_SERVERINFO, qtrue )
#ifdef _DEBUG
XCVAR_DEF( g_debugMove, "0", NULL, CVAR_NONE, qfalse )
#endif
XCVAR_DEF( g_debugSaberLocks, "0", NULL, CVAR_CHEAT, qfalse )
XCVAR_DEF( g_debugServerSkel, "0", NULL, CVAR_CHEAT, qfalse )
XCVAR_DEF( g_debugTrace, "0", NULL, CVAR_NONE, qfalse )
#ifdef _DEBUG
XCVAR_DEF( g_disableServerG2, "0", NULL, CVAR_NONE, qtrue )
#endif
XCVAR_DEF( g_dismember, "0", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_doWarmup, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( g_duelWeaponDisable, "1", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH, qtrue )
XCVAR_DEF( g_ff_objectives, "0", NULL, CVAR_CHEAT | CVAR_NORESTART, qtrue )
XCVAR_DEF( g_forceBasedTeams, "0", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH, qfalse )
XCVAR_DEF( g_forceClientUpdateRate, "250", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( g_forceDodge, "1", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( g_forcePowerDisable, "0", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH, qtrue )
XCVAR_DEF( g_forceRegenTime, "200", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_forceRegenTimeDuel, "200", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( g_forceRespawn, "60", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( g_fraglimitVoteCorrection, "1", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_friendlyFire, "0", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_friendlySaber, "0", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_g2TraceLod, "3", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( g_gametype, "0", NULL, CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, qfalse )
XCVAR_DEF( g_gravity, "800", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( g_inactivity, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( g_jediVmerc, "0", NULL, CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_jplua, "1", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( g_jpluaAutoload, "1", JPLua::UpdateAutoload, CVAR_NONE, qfalse )
XCVAR_DEF( g_knockback, "1000", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( g_locationBasedDamage, "1", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( g_logAdmin, "1", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( g_logClientInfo, "0", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_logConsole, "1", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( g_logFormat, "1", NULL, CVAR_ARCHIVE, qfalse )
XCVAR_DEF( g_logItemPickup, "1", NULL, CVAR_ARCHIVE, qfalse )
XCVAR_DEF( g_logSecurity, "1", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( g_maxForceRank, "7", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH, qfalse )
XCVAR_DEF( g_maxGameClients, "0", NULL, CVAR_LATCH | CVAR_ARCHIVE, qfalse )
XCVAR_DEF( g_maxHolocronCarry, "3", NULL, CVAR_LATCH, qfalse )
XCVAR_DEF( g_motd, "", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( g_needpass, "0", NULL, CVAR_SERVERINFO | CVAR_ROM, qtrue )
XCVAR_DEF( g_noSpecMove, "0", NULL, CVAR_SERVERINFO, qtrue )
XCVAR_DEF( g_password, "", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( g_powerDuelEndHealth, "90", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_powerDuelStartHealth, "150", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_privateDuel, "23", CVU_Duel, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_privateDuelHealth, "100", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_privateDuelShield, "100", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_restarted, "0", NULL, CVAR_ROM, qfalse )
XCVAR_DEF( g_saberBladeFaces, "1", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( g_saberDamageScale, "1", NULL, CVAR_ARCHIVE, qtrue )
#ifdef DEBUG_SABER_BOX
XCVAR_DEF( g_saberDebugBox, "0", NULL, CVAR_CHEAT, qfalse )
#endif
XCVAR_DEF( g_saberDebugPrint, "0", NULL, CVAR_CHEAT, qfalse )
XCVAR_DEF( g_saberDmgDelay_Idle, "350", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_saberDmgDelay_Wound, "0", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_saberDmgVelocityScale, "0", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_saberLockFactor, "2", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_saberLocking, "1", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_saberLockRandomNess, "2", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( g_saberRealisticCombat, "0", NULL, CVAR_CHEAT, qfalse )
XCVAR_DEF( g_saberRestrictForce, "0", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_saberTraceSaberFirst, "0", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_saberWallDamageScale, "0.4", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( g_showDuelHealths, "0", NULL, CVAR_SERVERINFO, qfalse )
XCVAR_DEF( g_siegeRespawn, "20", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_siegeTeam1, "none", NULL, CVAR_ARCHIVE | CVAR_SERVERINFO, qfalse )
XCVAR_DEF( g_siegeTeam2, "none", NULL, CVAR_ARCHIVE | CVAR_SERVERINFO, qfalse )
XCVAR_DEF( g_siegeTeamSwitch, "1", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE, qfalse )
XCVAR_DEF( g_slowmoDuelEnd, "0", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_smoothClients, "1", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( g_spawnInvulnerability, "3000", NULL, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_speed, "250", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( g_spSkill, "0", NULL, CVAR_ARCHIVE | CVAR_INTERNAL, qfalse )
XCVAR_DEF( g_statLog, "0", NULL, CVAR_ARCHIVE, qfalse )
XCVAR_DEF( g_statLogFile, "statlog.log", NULL, CVAR_ARCHIVE, qfalse )
XCVAR_DEF( g_stepSlideFix, "1", NULL, CVAR_SERVERINFO, qtrue )
XCVAR_DEF( g_synchronousClients, "0", NULL, CVAR_SYSTEMINFO, qfalse )
XCVAR_DEF( g_teamAutoJoin, "0", NULL, CVAR_ARCHIVE, qfalse )
XCVAR_DEF( g_teamForceBalance, "0", NULL, CVAR_ARCHIVE, qfalse )
XCVAR_DEF( g_timeOutToSpec, "45", NULL, CVAR_ARCHIVE, qfalse )
XCVAR_DEF( g_useWhileThrowing, "1", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( g_warmup, "20", CVU_Warmup, CVAR_ARCHIVE, qtrue )
XCVAR_DEF( g_weaponDisable, "0", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH, qtrue )
XCVAR_DEF( g_warmupPrintDelay, "10000", NULL, CVAR_ARCHIVE, qfalse )
XCVAR_DEF( g_weaponRespawn, "5", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( gamedate, __DATE__, NULL, CVAR_ROM, qfalse )
XCVAR_DEF( gamename, GAMEVERSION, NULL, CVAR_SERVERINFO | CVAR_ROM, qfalse )
XCVAR_DEF( japp_accurateMuzzle, "0", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_adminEffectDuration, "5", NULL, CVAR_NONE, qfalse)
XCVAR_DEF( japp_adminEffectType, "1", NULL, CVAR_NONE, qfalse)
XCVAR_DEF( japp_allowBusyAttack, "1", CVU_BusyAttack, CVAR_NONE, qtrue )
XCVAR_DEF( japp_allowButterfly, "1", CVU_Butterfly, CVAR_NONE, qtrue )
XCVAR_DEF( japp_allowDFA, "1", CVU_DFA, CVAR_NONE, qtrue )
XCVAR_DEF( japp_allowEmotes, "1", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_allowFallSuicide, "1", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_allowFlagDrop, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_allowFlagPull, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_allowForceCombo, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_allowHook, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_allowJetpack, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_allowKata, "1", CVU_Kata, CVAR_NONE, qtrue )
XCVAR_DEF( japp_allowLedgeGrab, "0", CVU_Ledge, CVAR_NONE, qtrue )
XCVAR_DEF( japp_allowPushPullKnockdown, "1", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_allowSaberSwitch, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_allowSPCartwheel, "1", CVU_SPCartwheel, CVAR_NONE, qtrue )
XCVAR_DEF( japp_allowSpinkicks, "0", CVU_SpinKicks, CVAR_NONE, qtrue )
XCVAR_DEF( japp_allowStab, "1", CVU_Stab, CVAR_NONE, qtrue )
XCVAR_DEF( japp_allowToggleSpecialAttacks, "0", CVU_ToggleAtk, CVAR_NONE, qtrue )
XCVAR_DEF( japp_allowVoiceChat, "960", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_allowWeaponDrop, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_allowWeaponPull, "1", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_allowWeaponWallRun, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_alwaysSpawnPowerups, "0", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_ammapAnyGametype, "0", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_amrenameTime, "0.5", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_antiFakePlayer, "1", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_antiUserinfoFlood, "1", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_antiWallhack, "0", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_charRestrictRGB, "1", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_chatProtection, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_corpseRemovalTime, "30", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_damageNotifications, "0", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_detectCountry, "0", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_duelActivateSaber, "1", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_duelStats, "0", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_empowerDrain, "1", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_fallToDeathInstant, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_fixRoll, "0", CVU_FixRoll, CVAR_NONE, qtrue )
XCVAR_DEF( japp_fixWeaponCharge, "1", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_flipKick, "0", CVU_Flipkick, CVAR_NONE, qtrue )
XCVAR_DEF( japp_flipKickDamage, "20", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_flipKickKnockdown, "1", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_forceLightningDamage, "1", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_ghostTouchTriggers, "1", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_gripHolsterSaber, "0", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_hookSpeed, "800", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_hookDebouncer, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_improveYellowDFA, "0", CVU_YellowDFA, CVAR_NONE, qtrue )
XCVAR_DEF( japp_instagib, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_instantForceSwitch, "0", NULL, CVAR_NONE, qtrue)
XCVAR_DEF( japp_instantSaberSwitch, "0", NULL, CVAR_NONE, qtrue)
XCVAR_DEF( japp_itemDropStyle, "1", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_itemPush, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_kickTrace, "1", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_maxConnPerIP, "3", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_mercInfiniteAmmo, "1", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_passRankConflicts, "1", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_pauseTime, "120", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_promode, "0", CVU_Promode, CVAR_NONE, qtrue )
XCVAR_DEF( japp_randFix, "1", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_readyThreshold, "0.5", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_removeOldExplosives, "1", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_reserveEntitySlots, "32", NULL, CVAR_ARCHIVE, qfalse )
XCVAR_DEF( japp_saberIdleDamage, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_saberTweaks, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_scorePlums, "0", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_scoreUpdateRate, "1000", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_shootFromEye, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_showLaggingClients, "1", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_showNextMap, "0", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_slapDistance, "50.0", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_slaydismember, "0", NULL, CVAR_NONE, qfalse)
XCVAR_DEF( japp_slideOnHead, "1", CVU_HeadSlide, CVAR_NONE, qtrue )
XCVAR_DEF( japp_sortScoreStyle, "1", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_spawnActivateSaber, "1", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_spawnItems, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_spawnWeaps, "28", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_speedCaps, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_suicideDropFlag, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_teleportBits, "0", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_unlagged, "1", NULL, CVAR_SERVERINFO, qtrue )
XCVAR_DEF( japp_unpauseTime, "5", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_userinfoValidate, "805306367", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_version, JAPP_VERSION, NULL, CVAR_ROM | CVAR_SERVERINFO, qfalse )
XCVAR_DEF( japp_voteDelay, "3000", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( japp_voteMapAnyGT, "0", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( japp_vq3physics, "0", CVU_VQ3Physics, CVAR_NONE, qtrue )
XCVAR_DEF( japp_weaponPickupAlways, "0", CVU_WeaponPU, CVAR_NONE, qtrue )
XCVAR_DEF( japp_weaponRoll, "0", CVU_WeaponRoll, CVAR_NONE, qtrue )
XCVAR_DEF( jp_cinfo, "0", CVU_CInfo, CVAR_ROM | CVAR_SERVERINFO, qfalse )
XCVAR_DEF( jp_gripSpeedScale, "0.4", NULL, CVAR_SERVERINFO, qtrue )
XCVAR_DEF( jp_DlBaseURL, "", NULL, CVAR_SYSTEMINFO, qfalse )
XCVAR_DEF( nextmap, "", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( pmove_fixed, "0", NULL, CVAR_SYSTEMINFO, qtrue )
XCVAR_DEF( pmove_float, "0", NULL, CVAR_SYSTEMINFO, qtrue )
XCVAR_DEF( pmove_msec, "8", NULL, CVAR_SYSTEMINFO, qtrue )
XCVAR_DEF( pmove_overbounce, "1", NULL, CVAR_SERVERINFO, qtrue )
XCVAR_DEF( RMG, "0", NULL, CVAR_NONE, qfalse )
XCVAR_DEF( sv_cheats, "", NULL, CVAR_LATCH, qtrue )
XCVAR_DEF( sv_fps, "40", NULL, CVAR_NONE, qtrue )
XCVAR_DEF( sv_maxclients, "24", NULL, CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, qfalse )
XCVAR_DEF( timelimit, "0", NULL, CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, qtrue )
XCVAR_DEF( v, "2.6B1", NULL, CVAR_SERVERINFO | CVAR_ROM, qfalse )

#undef XCVAR_DEF
