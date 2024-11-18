#include "prelude.h"
#include <stdio.h>

int main(){
    Source_Location loc = this_location();
    printf("%.*s : %d %.*s\n", fmt_bytes(loc.filename), loc.line, fmt_bytes(loc.caller_name));
    Logger logger = log_create_console_logger();
    log_debug(logger, "Hiiii");
    log_info(logger, "Hiiii");
    log_warn(logger, "Hiiii");
    log_error(logger, "Hiiii");
    log_fatal(logger, "Hiiii");
}
