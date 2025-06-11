#pragma once

// Registry of dead space in TotalaA.exe that's been reclaimed as options for a mod developer to hack.
// Try to keep these sorted by location

static const unsigned char* TA_HOOK_DEFAULT_MEX_SNAP_RADIUS =   (const unsigned char*)0x50390a; // TotalA.exe:0x101f0a
static const unsigned char* TA_HOOK_MAX_MEX_SNAP_RADIUS =       (const unsigned char*)0x50390b; // TotalA.exe:0x101f0b
static const unsigned char* TA_HOOK_DEFAULT_WRECK_SNAP_RADIUS = (const unsigned char*)0x503912; // TotalA.exe:0x101f12
static const unsigned char* TA_HOOK_MAX_WRECK_SNAP_RADIUS =     (const unsigned char*)0x503913; // TotalA.exe:0x101f13

static const unsigned char* TA_BUGFIX_FIXED_POSN_GUARDING_CONS_OPTION= (const unsigned char*)0x50391f; // TotalA.exe:0x101f1f
