#include "negcon_controls.h"
#include <string.h>
static WxlControls controls = {0,2,2,0,1,1,1};
static int choice(const char* key, const char* a, const char* b, int fallback) {
    char value[32];
    if (!psx_mod_option_value("wxl.controls.negcon", "dual-sticks", key, value, sizeof value)) return fallback;
    return !strcmp(value,a) ? 0 : !strcmp(value,b) ? 1 : 2;
}
static void map(const PSXModNegconInput* in, PSXModNegconOutput* out) {
    wxl_map_controls(&controls,in,out);
}
static void activate(void) {
    char preset[32] = "custom";
    psx_mod_option_value("wxl.controls.negcon", "dual-sticks", "preset", preset, sizeof preset);
    if (!strcmp(preset,"input-ini")) return;
    controls = (WxlControls){0,2,2,0,1,1,1};
    if (strcmp(preset,"recommended")) {
        controls.steering = choice("steering","left","right",0);
        controls.pitch = choice("pitch","off","left",2);
        controls.brakes = choice("brakes","off","normal",2);
        controls.shoulders = choice("shoulders","preserve","swap",0);
        controls.throttle = choice("throttle","off","right",1);
        controls.secondary = choice("secondary","off","left",1);
        controls.face_acceleration = choice("face-acceleration","off","on",1) == 1;
    }
    psx_mod_set_negcon_mapping(map);
}
PSX_MOD_CONSTRUCTOR(register_wxl_negcon) {
    psx_mod_register_activation_plugin("wxl.negcon",activate);
}
