/* Original opt-in presentation plugin. Visibility sites live in game.toml. */
#include "mod_plugins.h"
#include "gpu.h"
#include "cpu_state.h"
#include <string.h>
/* Main8001A504 enters menu4DA10; all race/attract paths enter3EA68.
 * Confirmed against Ghidra and native/Beetle executable bytes. Presentation
 * only: the menu's rotating preview is not a widescreen world scene. */
static int race_hud, font_anchor;
static void menu_enter(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;gpu_ws_set_ui_mode(1);
    race_hud=0; font_anchor=0; gpu_ws_clear_hud_quads();
}
static void race_enter(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;gpu_ws_set_ui_mode(0);
    race_hud=1; font_anchor=0; gpu_ws_clear_hud_quads();
}
/* Return PCs are verified JAL+8 sites in race HUD 80039018. Shared font
 * helpers also draw menus, warnings and centered overlays: those stay put. */
static void hud_font_enter(CPUState *cpu,uint32_t address) {
    (void)address;
    font_anchor=0;
    if(!race_hud) return;
    switch(cpu->gpr[31]) {
    case 0x80039540u: /* current lap time */
    case 0x800395acu: /* LAP TIMES heading */
    case 0x80039628u: /* lap row numbers */
    case 0x80039704u: /* completed lap times */
    case 0x80039b7cu: /* CHECK */
    case 0x80039cfcu: /* checkpoint countdown */
        font_anchor=-1; break;
    case 0x8003986cu: case 0x800398a0u: case 0x800398c0u: /* optional counter */
    case 0x800398dcu: case 0x80039940u: case 0x8003995cu: case 0x80039988u: /* lap */
    case 0x80039be4u: case 0x80039c04u: case 0x80039c20u: case 0x80039c3cu: /* position */
    case 0x80039de4u: /* numeric speed */
    case 0x80039b60u: /* complete music crawl, including its leftmost glyphs */
        font_anchor=1; break;
    default: break;
    }
}
static void hud_add_prim(CPUState *cpu,uint32_t address) {
    (void)address;
    int anchor=0;
    if(race_hud) switch(cpu->gpr[31]) {
    case 0x800372b8u: case 0x800373e4u: case 0x80037584u:
    case 0x80037694u: case 0x800377ccu: case 0x80037a38u:
    case 0x80037de0u: case 0x800381ccu: case 0x8005fca0u:
        anchor=font_anchor; break;
    case 0x80039fdcu: /* gauge frame and labels */
    case 0x80036e34u: case 0x80036eccu: case 0x80036f00u: /* animated fills */
        anchor=1; break;
    default: break;
    }
    gpu_ws_tag_hud_quad(cpu->gpr[5],anchor);
}
static void activate_widescreen(void) {
    gpu_ws_set_ui_mode(0);
    psx_mod_register_function_entry_plugin("wxl.widescreen",0x8004da10u,menu_enter);
    psx_mod_register_function_entry_plugin("wxl.widescreen",0x8003ea68u,race_enter);
    char layout[16];
    if(psx_mod_option_value("wxl.presentation.widescreen","widescreen",
                            "hud-layout",layout,sizeof(layout)) && !strcmp(layout,"edges")) {
        const uint32_t fonts[]={0x80036f34u,0x80037818u,0x80037a94u,0x80037e34u,0x8005f9b8u};
        for(unsigned i=0;i<sizeof(fonts)/sizeof(fonts[0]);++i)
            psx_mod_register_function_entry_plugin("wxl.widescreen",fonts[i],hud_font_enter);
        psx_mod_register_function_entry_plugin("wxl.widescreen",0x8007b430u,hud_add_prim);
    }
    gpu_ws_set_gte_game_mode(1);
    gpu_ws_set_solid_pillarbox(1);
    psx_mod_set_fixed_display_aspect(16, 9);
}
PSX_MOD_CONSTRUCTOR(register_wxl_widescreen) {
    psx_mod_register_activation_plugin("wxl.widescreen", activate_widescreen);
}
