/** @file
  Set the memory protection settings based on platform configuration.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/SetMemoryProtectionSettingsLib.h>

/**
  Module entry point.

  @param[in]    FileHandle    Handle of the file being invoked.
  @param[in]    PeiServices   Describes the list of possible PEI Services.

  @return       EFI_SUCCESS   Always return EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
InitializeMemoryProtectionEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  DXE_MEMORY_PROTECTION_SETTINGS  DxeSettings;
  MM_MEMORY_PROTECTION_SETTINGS   MmSettings;

  if (!EFI_ERROR(PeiServicesRegisterForShadow (FileHandle))) {
    return EFI_SUCCESS;
  }

  DxeSettings = DxeMemoryProtectionProfiles[DxeMemoryProtectionSettingsDebug].Settings;
  MmSettings  = MmMemoryProtectionProfiles[MmMemoryProtectionSettingsDebug].Settings;

  DxeSettings.NullPointerDetection.DisableEndOfDxe = TRUE;

  SetDxeMemoryProtectionSettings (&DxeSettings, DxeMemoryProtectionSettingsDebug);
  SetMmMemoryProtectionSettings (&MmSettings, MmMemoryProtectionSettingsDebug);

  return EFI_SUCCESS;
}
