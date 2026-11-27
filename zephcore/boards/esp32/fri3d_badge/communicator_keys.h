/*
 * SPDX-License-Identifier: MIT
 * Fri3d Camp 2026 badge — key codes of the communicator add-on's keyboard
 *
 * The keys that type a character report FRI3D_TEXT_KEY(<the character
 * without Shift>) (communicator.overlay keymap); board_init.c turns them into typed
 * characters (zephcore_input_ascii.h) with Shift and Caps Lock applied. They
 * do not use the INPUT_KEY_* letter and digit codes: the joystick UI listens
 * to every input device, and INPUT_KEY_1/B/C/D/E are its START button
 * actions (back, LED, buzzer, GPS, flood advert), INPUT_KEY_A the START
 * button's multi-tap input.
 *
 * Shared by the devicetree overlay and C: plain defines only.
 */

#ifndef FRI3D_COMMUNICATOR_KEYS_H
#define FRI3D_COMMUNICATOR_KEYS_H

/* above the INPUT_KEY_* codes (KEY_MAX 0x2ff) and ZEPHCORE_INPUT_ASCII_BASE (0x300) */
#define FRI3D_TEXT_KEY_BASE 0x400
#define FRI3D_TEXT_KEY(c)   (FRI3D_TEXT_KEY_BASE + (c))
/* a key without a use here */
#define FRI3D_KEY_UNUSED    0x4ff

#endif /* FRI3D_COMMUNICATOR_KEYS_H */
