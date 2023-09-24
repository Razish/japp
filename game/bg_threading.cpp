#if defined PROJECT_GAME
#include "g_local.h"
#elif defined PROJECT_CGAME
#include "cg_local.h"
#elif defined PROJECT_UI
#include "ui/ui_local.h"
#endif
#include "bg_threading.h"

std::vector<TaskInterface *> tasks;

void CheckTasks(void) {
    size_t i = 0;
    for (auto it = tasks.begin(); it != tasks.end(); ++it, i++) {
        if (!*it)
            continue;
        TaskInterface *tsk = (TaskInterface *)(*it);
        if (tsk->isReady()) {
            tsk->runCallback(tsk->getResult());
            delete tasks[i];
            it = tasks.erase(tasks.begin() + i);

            if (it == tasks.end())
                break;
        }
    }
}