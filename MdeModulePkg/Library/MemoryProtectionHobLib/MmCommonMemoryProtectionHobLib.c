/** @file
Library fills out gMmMps global

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Pi/PiMultiPhase.h>
#include <Uefi/UefiMultiPhase.h>

#include <Library/MmMemoryProtectionHobLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>

MM_MEMORY_PROTECTION_SETTINGS  gMmMps;

/**
  This function checks the memory protection settings and provides warnings of settings conflicts.
  For compatibility, this logic will only ever turn off protections to create consistency,
  never turn others on.
**/
VOID
MmMemoryProtectionSettingsConsistencyCheck (
  VOID
  )
{
  if (MM_MPS_IS_ANY_MEMORY_TYPE_ACTIVE (&gMmMps.PoolGuard) &&
      (!gMmMps.HeapGuard.PoolGuardEnabled))
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - PoolGuard protections are active "
      "but HeapGuard.PoolGuardEnabled is inactive.\n",
      __func__
      ));
  }

  if (MM_MPS_IS_ANY_MEMORY_TYPE_ACTIVE (&gMmMps.PageGuard) &&
      (!gMmMps.HeapGuard.PageGuardEnabled))
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - PageGuard protections are active "
      "but HeapGuard.PageGuardEnabled is inactive\n",
      __func__
      ));
  }
}

/**
  Abstraction layer for library constructor of Standalone MM and SMM instances.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
MmMemoryProtectionHobLibConstructorCommon (
  VOID
  )
{
  VOID                           *Ptr;
  MM_MEMORY_PROTECTION_SETTINGS  *MmMps;

  Ptr = GetFirstGuidHob (&gMmMemoryProtectionSettingsGuid);

  //
  // Cache the Memory Protection Settings HOB entry
  //
  if (Ptr != NULL) {
    MmMps = (MM_MEMORY_PROTECTION_SETTINGS *)GET_GUID_HOB_DATA (Ptr);
    if (!MM_MPS_IS_STRUCT_VALID (MmMps)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: - Version number of the MM Memory Protection Settings HOB is invalid!\n",
        __func__
        ));
      ASSERT (MM_MPS_IS_STRUCT_VALID (MmMps));
      ZeroMem (&gMmMps, sizeof (gMmMps));
      return EFI_SUCCESS;
    }

    CopyMem (&gMmMps, MmMps, sizeof (MM_MEMORY_PROTECTION_SETTINGS));
    MmMemoryProtectionSettingsConsistencyCheck ();
  }

  return EFI_SUCCESS;
}
