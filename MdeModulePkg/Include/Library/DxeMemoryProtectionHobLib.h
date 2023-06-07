/** @file

Library for accessing memory protection settings.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef DXE_MEMORY_PROTECTION_HOB_LIB_H_
#define DXE_MEMORY_PROTECTION_HOB_LIB_H_

#include <Guid/DxeMemoryProtectionSettings.h>

//
//  The global used to access current Memory Protection Settings
//
extern DXE_MEMORY_PROTECTION_SETTINGS  gDxeMps;

#endif
