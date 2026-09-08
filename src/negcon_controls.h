/* Host input mapping only; no game-derived code. */
#pragma once
#include "mod_plugins.h"
#include "negcon_protocol.h"
typedef struct WxlControls {
    int steering; /* 0 left X, 1 right X, 2 off */
    int pitch;    /* 0 off, 1 left Y, 2 right Y */
    int brakes;   /* 0 off, 1 normal right X, 2 reversed right X */
    int shoulders; /* 0 preserve bindings, 1 swap, 2 disable */
    int throttle; /* 0 off, 1 right trigger, 2 left trigger */
    int secondary; /* 0 off, 1 left trigger, 2 right trigger */
    int face_acceleration; /* preserve mapped Cross acceleration */
} WxlControls;
static inline void wxl_map_controls(const WxlControls* c, const PSXModNegconInput* in,
                                     PSXModNegconOutput* out) {
    int dz = in->deadzone;
    if (dz < 0) dz = 0;
    if (dz > 32766) dz = 32766;
    out->buttons = in->buttons;
    if (c->shoulders) {
        unsigned l = !(out->buttons & 0x0400), r = !(out->buttons & 0x0800);
        out->buttons |= 0x0c00;
        if (c->shoulders == 1) {
            if (l) out->buttons &= ~0x0800;
            if (r) out->buttons &= ~0x0400;
        }
    }
    if (c->pitch) {
        int y = in->axis[c->pitch == 1 ? 1 : 3];
        if (y < -dz) out->buttons &= ~0x0010;
        if (y > dz) out->buttons &= ~0x0040;
    }
    if (c->brakes) {
        int x = in->axis[2];
        if (x < -dz) out->buttons &= ~(c->brakes == 2 ? 0x0800 : 0x0400);
        if (x > dz) out->buttons &= ~(c->brakes == 2 ? 0x0400 : 0x0800);
    }
    int x = c->steering == 2 ? 0 : in->axis[c->steering == 1 ? 2 : 0];
    if (x < -32767) x = -32767;
    if (x > dz) x = (x-dz)*32767/(32767-dz);
    else if (x < -dz) x = (x+dz)*32767/(32767-dz);
    else x = 0;
    out->twist = negcon_twist_byte(x);
    out->i = c->face_acceleration && !(out->buttons & 0x4000) ? 255 : 0;
    out->ii = !(out->buttons & 0x8000) ? 255 : 0;
    out->l = !(out->buttons & 0x0400) ? 255 : 0;
    if (c->throttle) {
        unsigned v = negcon_pressure_byte(in->axis[c->throttle == 1 ? 5 : 4]);
        if (v > out->i) out->i = (uint8_t)v;
    }
    if (c->secondary) {
        unsigned v = negcon_pressure_byte(in->axis[c->secondary == 1 ? 4 : 5]);
        if (v > out->ii) out->ii = (uint8_t)v;
    }
}
