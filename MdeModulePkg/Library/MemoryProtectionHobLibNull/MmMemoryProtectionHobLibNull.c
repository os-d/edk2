/** @file
Library defines the gMmMps global

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/MmMemoryProtectionHobLib.h>

// A global variable which is uninitialized will be zero.
// The net effect is memory protections will be OFF.
MM_MEMORY_PROTECTION_SETTINGS  gMmMps;
