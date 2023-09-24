int Q3_PlaySound(int taskID, int entID, const char *name, const char *channel);
qboolean Q3_Set(int taskID, int entID, const char *type_name, const char *data);
void Q3_Lerp2Pos(int taskID, int entID, vector3 *origin, vector3 *angles, float duration);
void Q3_Lerp2Origin(int taskID, int entID, vector3 *origin, float duration);
void Q3_Lerp2Angles(int taskID, int entID, vector3 *angles, float duration);
int Q3_GetTag(int entID, const char *name, int lookup, vector3 *info);
void Q3_Lerp2Start(int entID, int taskID, float duration);
void Q3_Lerp2End(int entID, int taskID, float duration);
void Q3_Use(int entID, const char *target);
void Q3_Kill(int entID, const char *name);
void Q3_Remove(int entID, const char *name);
void Q3_Play(int taskID, int entID, const char *type, const char *name);
int Q3_GetFloat(int entID, int type, const char *name, float *value);
int Q3_GetVector(int entID, int type, const char *name, vector3 *value);
int Q3_GetString(int entID, int type, const char *name, const char **value);
