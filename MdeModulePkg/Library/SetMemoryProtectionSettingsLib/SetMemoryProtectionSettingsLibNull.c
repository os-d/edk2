/** @file
Library for setting the memory protection settings for DXE.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/SetMemoryProtectionSettingsLib.h>

DXE_MEMORY_PROTECTION_PROFILES  DxeMemoryProtectionProfiles[DxeMemoryProtectionSettingsMax] = { 0 };
MM_MEMORY_PROTECTION_PROFILES   MmMemoryProtectionProfiles[MmMemoryProtectionSettingsMax]   = { 0 };

/**
  Apply EFI_MEMORY_RO and EFI_MEMORY_XP permissions to the memory protection settings.
  This function will ASSERT if the memory protection settings could not be fetched/created
  or if the memory attribute PPI exists but the call to set permissions failed. Even if
  applying paging protections to the settings fails, later library API calls will not update
  the settings. This function will not abort if it was called previously in case the PPI was
  not available when this function was first called.

  @retval EFI_SUCCESS           The memory protection settings are locked.
  @retval EFI_NOT_STARTED       The memory protection settings have not been allocated.
  @retval EFI_UNSUPPORTED       NULL implementation called.
  @retval Others                An error occurred while setting the permissions.
**/
EFI_STATUS
EFIAPI
LockMemoryProtectionSettings (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Sets the DXE memory protection settings. If DxeMps is NULL, the settings will be set based
  on ProfileIndex.

  @param[in] DxeMps        Pointer to the memory protection settings to publish. If NULL, the
                           settings will be created based on ProfileIndex.
  @param[in] ProfileIndex  The index of the memory protection profile to use if DxeMps is NULL.

  @retval EFI_SUCCESS           The memory protection HOB was successfully created.
  @retval EFI_OUT_OF_RESOURCES  There was insufficient memory to create the HOB.
  @retval EFI_INVALID_PARAMETER The ProfileIndex was invalid or the version number of the
                                input DxeMps was not equal to the version currently present
                                in the settings.
  @retval EFI_ACCESS_DENIED     The memory protection settings are locked.
  @retval EFI_UNSUPPORTED       NULL implementation called.
**/
EFI_STATUS
EFIAPI
SetDxeMemoryProtectionSettings (
  IN DXE_MEMORY_PROTECTION_SETTINGS       *DxeMps OPTIONAL,
  IN DXE_MEMORY_PROTECTION_PROFILE_INDEX  ProfileIndex
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Sets the MM memory protection HOB entry. If MmMps is NULL, the settings will be set based
  on ProfileIndex.

  @param[in] MmMps         Pointer to the memory protection settings to publish. If NULL, the
                           settings will be created based on ProfileIndex.
  @param[in] ProfileIndex  The index of the memory protection profile to use if MmMps is NULL.

  @retval EFI_SUCCESS           The memory protection HOB was successfully created.
  @retval EFI_OUT_OF_RESOURCES  There was insufficient memory to create the HOB.
  @retval EFI_INVALID_PARAMETER The ProfileIndex was invalid or the version number of the
                                input MmMps was not equal to the version currently present
                                in the settings.
  @retval EFI_ACCESS_DENIED     The memory protection settings are locked.
  @retval EFI_UNSUPPORTED       NULL implementation called.
**/
EFI_STATUS
EFIAPI
SetMmMemoryProtectionSettings (
  IN MM_MEMORY_PROTECTION_SETTINGS       *MmMps OPTIONAL,
  IN MM_MEMORY_PROTECTION_PROFILE_INDEX  ProfileIndex
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Copies the current memory protection settings into the input buffer.

  @param[out] Mps  The memory protection settings pointer to populate.

  @retval EFI_SUCCESS           The memory protection settings were copied
                                into the input buffer.
  @retval EFI_ABORTED           The memory protection settings are invalid.
  @retval EFI_INVALID_PARAMETER Mps was NULL.
  @retval EFI_OUT_OF_RESOURCES  The memory protection settings memory needed
                                to be allocated but allocation failed.
  @retval EFI_UNSUPPORTED       NULL implementation called.
**/
EFI_STATUS
EFIAPI
GetCurrentMemoryProtectionSettings (
  OUT MEMORY_PROTECTION_SETTINGS  *Mps
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Returns TRUE any form of DXE memory protection is active.

  @retval TRUE   DXE Memory protection is active.
  @retval FALSE  DXE Memory protection is not active.
**/
BOOLEAN
EFIAPI
IsDxeMemoryProtectionActive (
  VOID
  )
{
  return FALSE;
}

/**
  Returns TRUE any form of MM memory protection is active.

  @retval TRUE   MM Memory protection is active.
  @retval FALSE  MM Memory protection is not active.
**/
BOOLEAN
EFIAPI
IsMmMemoryProtectionActive (
  VOID
  )
{
  return FALSE;
}
