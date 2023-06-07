/** @file
Library fills out gDxeMps global

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Pi/PiMultiPhase.h>
#include <Uefi/UefiMultiPhase.h>

#include <Library/DxeMemoryProtectionHobLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>

DXE_MEMORY_PROTECTION_SETTINGS  gDxeMps;

/**
  This function checks the memory protection settings and provides warnings of settings conflicts.
  For compatibility, this logic will only ever turn off protections to create consistency,
  never turn others on.
**/
VOID
DxeMemoryProtectionSettingsConsistencyCheck (
  VOID
  )
{
  if ((gDxeMps.HeapGuard.PoolGuardEnabled || gDxeMps.HeapGuard.PageGuardEnabled) &&
      gDxeMps.HeapGuard.FreedMemoryGuardEnabled)
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - HeapGuard.FreedMemoryGuardEnabled and "
      "UEFI HeapGuard.PoolGuardEnabled/HeapGuard.PageGuardEnabled "
      "cannot be active at the same time. Setting all three to ZERO in "
      "the memory protection settings global.\n",
      __func__
      ));
    ASSERT (
      !(gDxeMps.HeapGuard.FreedMemoryGuardEnabled &&
        (gDxeMps.HeapGuard.PoolGuardEnabled || gDxeMps.HeapGuard.PageGuardEnabled))
      );
    gDxeMps.HeapGuard.PoolGuardEnabled        = FALSE;
    gDxeMps.HeapGuard.PageGuardEnabled        = FALSE;
    gDxeMps.HeapGuard.FreedMemoryGuardEnabled = FALSE;
  }

  if (DXE_MPS_IS_ANY_MEMORY_TYPE_ACTIVE (&gDxeMps.PoolGuard) &&
      (!(gDxeMps.HeapGuard.PoolGuardEnabled)))
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - PoolGuard protections are active "
      "but HeapGuard.PoolGuardEnabled is inactive.\n",
      __func__
      ));
  }

  if (DXE_MPS_IS_ANY_MEMORY_TYPE_ACTIVE (&gDxeMps.PageGuard) &&
      (!(gDxeMps.HeapGuard.PageGuardEnabled)))
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - PageGuard protections are active "
      "but HeapGuard.PageGuardEnabled is inactive\n",
      __func__
      ));
  }

  if (gDxeMps.ExecutionProtection.EnabledForType[EfiBootServicesData] !=
      gDxeMps.ExecutionProtection.EnabledForType[EfiConventionalMemory])
  {
    DEBUG ((
      DEBUG_WARN,
      "%a: - EfiBootServicesData and EfiConventionalMemory must have the same "
      "ExecutionProtection value. Setting both to ZERO in the memory protection "
      "settings global.\n",
      __func__
      ));
    ASSERT (
      gDxeMps.ExecutionProtection.EnabledForType[EfiBootServicesData] ==
      gDxeMps.ExecutionProtection.EnabledForType[EfiConventionalMemory]
      );
    gDxeMps.ExecutionProtection.EnabledForType[EfiBootServicesData]   = FALSE;
    gDxeMps.ExecutionProtection.EnabledForType[EfiConventionalMemory] = FALSE;
  }
}

/**
  Populates gDxeMps global with the data present in the HOB. If the HOB entry does not exist,
  this constructor will zero the memory protection settings.

  @param[in]  ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
DxeMemoryProtectionHobLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  VOID                            *Ptr;
  DXE_MEMORY_PROTECTION_SETTINGS  *DxeMps;

  Ptr = GetFirstGuidHob (&gDxeMemoryProtectionSettingsGuid);

  //
  // Cache the Memory Protection Settings HOB entry
  //
  if (Ptr != NULL) {
    DxeMps = (DXE_MEMORY_PROTECTION_SETTINGS *)GET_GUID_HOB_DATA (Ptr);
    if (!DXE_MPS_IS_STRUCT_VALID (DxeMps)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: - Version number of the DXE Memory Protection Settings HOB is invalid!\n",
        __func__
        ));
      ASSERT (DXE_MPS_IS_STRUCT_VALID (DxeMps));
      ZeroMem (&gDxeMps, sizeof (gDxeMps));
      return EFI_SUCCESS;
    }

    CopyMem (&gDxeMps, DxeMps, sizeof (DXE_MEMORY_PROTECTION_SETTINGS));
    DxeMemoryProtectionSettingsConsistencyCheck ();
  }

  return EFI_SUCCESS;
}
