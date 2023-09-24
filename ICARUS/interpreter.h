// Interpreter.h

#pragma once

#define ICARUS_VERSION 1.33

#define MAX_STRING_SIZE 256
#define MAX_VAR_NAME 64

typedef float vector_t[3];

// If you modify this, you MUST modify in g_ICARUScb.c as well.
// Token defines
enum {
    TK_BLOCK_START = TK_USERDEF,
    TK_BLOCK_END,
    TK_VECTOR_START,
    TK_VECTOR_END,
    TK_OPEN_PARENTHESIS,
    TK_CLOSED_PARENTHESIS,
    TK_VECTOR,
    TK_GREATER_THAN,
    TK_LESS_THAN,
    TK_EQUALS,
    TK_NOT,

    NUM_USER_TOKENS
};

// ID defines
enum {
    ID_AFFECT = NUM_USER_TOKENS,
    ID_SOUND,
    ID_MOVE,
    ID_ROTATE,
    ID_WAIT,
    ID_BLOCK_START,
    ID_BLOCK_END,
    ID_SET,
    ID_LOOP,
    ID_LOOPEND,
    ID_PRINT,
    ID_USE,
    ID_FLUSH,
    ID_RUN,
    ID_KILL,
    ID_REMOVE,
    ID_CAMERA,
    ID_GET,
    ID_RANDOM,
    ID_IF,
    ID_ELSE,
    ID_REM,
    ID_TASK,
    ID_DO,
    ID_DECLARE,
    ID_FREE,
    ID_DOWAIT,
    ID_SIGNAL,
    ID_WAITSIGNAL,
    ID_PLAY,

    ID_TAG,
    ID_EOF,
    NUM_IDS
};

// Type defines
enum {
    // Wait types
    TYPE_WAIT_COMPLETE = NUM_IDS,
    TYPE_WAIT_TRIGGERED,

    // Set types
    TYPE_ANGLES,
    TYPE_ORIGIN,

    // Affect types
    TYPE_INSERT,
    TYPE_FLUSH,

    // Camera types
    TYPE_PAN,
    TYPE_ZOOM,
    TYPE_MOVE,
    TYPE_FADE,
    TYPE_PATH,
    TYPE_ENABLE,
    TYPE_DISABLE,
    TYPE_SHAKE,
    TYPE_ROLL,
    TYPE_TRACK,
    TYPE_DISTANCE,
    TYPE_FOLLOW,

    // Variable type
    TYPE_VARIABLE,

    TYPE_EOF,
    NUM_TYPES
};

#undef MSG_EOF
enum messageTypes_e {
    MSG_COMPLETED,
    MSG_EOF,
    NUM_MESSAGES,
};
