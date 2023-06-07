/** @file

Library for accessing memory protection settings.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MM_MEMORY_PROTECTION_HOB_LIB_H_
#define MM_MEMORY_PROTECTION_HOB_LIB_H_

#include <Guid/MmMemoryProtectionSettings.h>

//
//  The global used to access current Memory Protection Settings
//
extern MM_MEMORY_PROTECTION_SETTINGS  gMmMps;

#endif
