/** @file
Library defines the gDxeMps global

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/DxeMemoryProtectionHobLib.h>

// A global variable which is uninitialized will be zero.
// The net effect is memory protections will be OFF.
DXE_MEMORY_PROTECTION_SETTINGS  gDxeMps;
