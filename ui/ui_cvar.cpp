#include "ui_local.h"

// Cvar callbacks

/*
static void CVU_Derpity( void ) {
// ...
}
*/

// Cvar table

#define XCVAR_DECL
#include "ui_xcvar.h"
#undef XCVAR_DECL

void UI_Set2DRatio(void) {
    if (japp_ratioFix.integer & (1 << RATIOFIX_HUD)) {
        uiInfo.uiDC.widthRatioCoef = (float)(SCREEN_WIDTH * uiInfo.uiDC.glconfig.vidHeight) / (float)(SCREEN_HEIGHT * uiInfo.uiDC.glconfig.vidWidth);
    } else {
        uiInfo.uiDC.widthRatioCoef = 1.0f;
    }
}

static void CVU_Master1(void) { trap->Cmd_ExecuteText(EXEC_NOW, va("set sv_master1 \"%s\"", ui_sv_master1.string)); }
static void CVU_Master2(void) { trap->Cmd_ExecuteText(EXEC_NOW, va("set sv_master2 \"%s\"", ui_sv_master2.string)); }
static void CVU_Master3(void) { trap->Cmd_ExecuteText(EXEC_NOW, va("set sv_master3 \"%s\"", ui_sv_master3.string)); }
static void CVU_Master4(void) { trap->Cmd_ExecuteText(EXEC_NOW, va("set sv_master4 \"%s\"", ui_sv_master4.string)); }
static void CVU_Master5(void) { trap->Cmd_ExecuteText(EXEC_NOW, va("set sv_master5 \"%s\"", ui_sv_master5.string)); }

static const struct cvarTable_t {
    vmCvar_t *vmCvar;
    const char *cvarName, *defaultString;
    void (*update)(void);
    uint32_t cvarFlags;
} cvarTable[] = {
#define XCVAR_LIST
#include "ui_xcvar.h"
#undef XCVAR_LIST
};

void UI_RegisterCvars(void) {
    char buf[MAX_CVAR_VALUE_STRING];
    trap->Cvar_VariableStringBuffer("sv_master1", buf, sizeof(buf));
    trap->Cvar_Set("ui_sv_master1", buf);
    trap->Cvar_VariableStringBuffer("sv_master2", buf, sizeof(buf));
    trap->Cvar_Set("ui_sv_master2", buf);
    trap->Cvar_VariableStringBuffer("sv_master3", buf, sizeof(buf));
    trap->Cvar_Set("ui_sv_master3", buf);
    trap->Cvar_VariableStringBuffer("sv_master4", buf, sizeof(buf));
    trap->Cvar_Set("ui_sv_master4", buf);
    trap->Cvar_VariableStringBuffer("sv_master5", buf, sizeof(buf));
    trap->Cvar_Set("ui_sv_master5", buf);

    for (const auto &cv : cvarTable) {
        trap->Cvar_Register(cv.vmCvar, cv.cvarName, cv.defaultString, cv.cvarFlags);
    }
    for (const auto &cv : cvarTable) {
        if (cv.update) {
            cv.update();
        }
    }
}

void UI_UpdateCvars(void) {
    for (const auto &cv : cvarTable) {
        if (cv.vmCvar) {
            int modCount = cv.vmCvar->modificationCount;
            trap->Cvar_Update(cv.vmCvar);
            if (cv.vmCvar->modificationCount != modCount) {
                if (cv.update) {
                    cv.update();
                }
            }
        }
    }
}
