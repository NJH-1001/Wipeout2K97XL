/* Original opt-in presentation plugin. Visibility sites live in game.toml. */
#include "mod_plugins.h"
#include "gpu.h"
/* Main8001A504 enters menu4DA10; all race/attract paths enter3EA68.
 * Confirmed against Ghidra and native/Beetle executable bytes. Presentation
 * only: the menu's rotating preview is not a widescreen world scene. */
static void menu_enter(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;gpu_ws_set_ui_mode(1);
}
static void race_enter(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;gpu_ws_set_ui_mode(0);
}
static void activate_widescreen(void) {
    gpu_ws_set_ui_mode(0);
    psx_mod_register_function_entry_plugin("wxl.widescreen",0x8004da10u,menu_enter);
    psx_mod_register_function_entry_plugin("wxl.widescreen",0x8003ea68u,race_enter);
    gpu_ws_set_gte_game_mode(1);
    gpu_ws_set_solid_pillarbox(1);
    psx_mod_set_fixed_display_aspect(16, 9);
}
PSX_MOD_CONSTRUCTOR(register_wxl_widescreen) {
    psx_mod_register_activation_plugin("wxl.widescreen", activate_widescreen);
}
