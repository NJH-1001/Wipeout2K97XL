/* Original presentation plugin; no game-derived code or assets. */
#include "mod_plugins.h"
#include "gpu_gl_renderer.h"
#include <string.h>
static void activate_crt(void) {
    char preset[32] = "jvc";
    psx_mod_option_value("wxl.presentation.crt", "crt", "preset", preset, sizeof preset);
    gl_renderer_set_crt_preset(!strcmp(preset, "trinitron") ? 2 : 1);
}
PSX_MOD_CONSTRUCTOR(register_wxl_crt) {
    psx_mod_register_activation_plugin("wxl.crt", activate_crt);
}
