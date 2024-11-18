#include "prelude.h"

typedef struct Logger Logger;

typedef i64 (*Logger_Func)(
    void* impl,
    enum Log_Level level,
);