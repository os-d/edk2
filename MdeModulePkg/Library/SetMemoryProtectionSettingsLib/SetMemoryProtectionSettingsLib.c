/** @file
Library for setting the memory protection settings for DXE.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SetMemoryProtectionSettingsLib.h>

#pragma pack(1)

typedef struct {
  // Protection settings
  MEMORY_PROTECTION_SETTINGS    Mps;
  // Extra byte for tracking if protection settings have been locked
  BOOLEAN                       MemoryProtectionSettingsLocked;
} MEMORY_PROTECTION_SETTINGS_PRIVATE;

#pragma pack()

STATIC MEMORY_PROTECTION_SETTINGS_PRIVATE  *mMpsp = NULL;
/////////////////////////////
// DXE PROFILE DEFINITIONS //
/////////////////////////////

//
//  A memory profile with strict settings ideal for development scenarios.
//
#define DXE_MEMORY_PROTECTION_SETTINGS_DEBUG          \
{                                                     \
  DXE_MEMORY_PROTECTION_SIGNATURE,                    \
  DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION,     \
  TRUE, /* Stack Guard */                             \
  TRUE, /* Stack Execution Protection */              \
  {     /* NULL Pointer Detection */                  \
    .Enabled                                = TRUE,   \
    .DisableEndOfDxe                        = FALSE,  \
    .NonstopModeEnabled                     = TRUE    \
  },                                                  \
  { /* Image Protection */                            \
    .ProtectImageFromUnknown                = FALSE,   \
    .ProtectImageFromFv                     = FALSE    \
  },                                                  \
  { /* Execution Protection */                        \
    .EnabledForType = {                               \
      [EfiReservedMemoryType]               = FALSE,   \
      [EfiLoaderCode]                       = FALSE,  \
      [EfiLoaderData]                       = FALSE,   \
      [EfiBootServicesCode]                 = FALSE,  \
      [EfiBootServicesData]                 = FALSE,   \
      [EfiRuntimeServicesCode]              = FALSE,  \
      [EfiRuntimeServicesData]              = FALSE,   \
      [EfiConventionalMemory]               = FALSE,   \
      [EfiUnusableMemory]                   = FALSE,   \
      [EfiACPIReclaimMemory]                = FALSE,   \
      [EfiACPIMemoryNVS]                    = FALSE,   \
      [EfiMemoryMappedIO]                   = FALSE,   \
      [EfiMemoryMappedIOPortSpace]          = FALSE,   \
      [EfiPalCode]                          = FALSE,   \
      [EfiPersistentMemory]                 = FALSE,  \
      [EfiUnacceptedMemoryType]             = FALSE,   \
      [OEM_RESERVED_MPS_MEMORY_TYPE]        = FALSE,   \
      [OS_RESERVED_MPS_MEMORY_TYPE]         = FALSE    \
    }                                                 \
  },                                                  \
  { /* Heap Guard */                                  \
    .PageGuardEnabled                       = TRUE,   \
    .PoolGuardEnabled                       = TRUE,   \
    .FreedMemoryGuardEnabled                = FALSE,  \
    .NonstopModeEnabled                     = TRUE,   \
    .GuardAlignedToTail                     = TRUE   \
  },                                                  \
  { /* Pool Guard */                                  \
    .EnabledForType = {                               \
      [EfiReservedMemoryType]               = TRUE,   \
      [EfiLoaderCode]                       = TRUE,   \
      [EfiLoaderData]                       = TRUE,   \
      [EfiBootServicesCode]                 = TRUE,   \
      [EfiBootServicesData]                 = TRUE,   \
      [EfiRuntimeServicesCode]              = TRUE,   \
      [EfiRuntimeServicesData]              = TRUE,   \
      [EfiConventionalMemory]               = FALSE,  \
      [EfiUnusableMemory]                   = TRUE,   \
      [EfiACPIReclaimMemory]                = TRUE,   \
      [EfiACPIMemoryNVS]                    = TRUE,   \
      [EfiMemoryMappedIO]                   = TRUE,   \
      [EfiMemoryMappedIOPortSpace]          = TRUE,   \
      [EfiPalCode]                          = TRUE,   \
      [EfiPersistentMemory]                 = FALSE,  \
      [EfiUnacceptedMemoryType]             = TRUE,   \
      [OEM_RESERVED_MPS_MEMORY_TYPE]        = TRUE,   \
      [OS_RESERVED_MPS_MEMORY_TYPE]         = TRUE    \
    }                                                 \
  },                                                  \
  { /* Page Guard */                                  \
    .EnabledForType = {                               \
      [EfiReservedMemoryType]               = TRUE,   \
      [EfiLoaderCode]                       = TRUE,   \
      [EfiLoaderData]                       = TRUE,   \
      [EfiBootServicesCode]                 = TRUE,   \
      [EfiBootServicesData]                 = TRUE,   \
      [EfiRuntimeServicesCode]              = TRUE,   \
      [EfiRuntimeServicesData]              = TRUE,   \
      [EfiConventionalMemory]               = FALSE,  \
      [EfiUnusableMemory]                   = TRUE,   \
      [EfiACPIReclaimMemory]                = TRUE,   \
      [EfiACPIMemoryNVS]                    = TRUE,   \
      [EfiMemoryMappedIO]                   = TRUE,   \
      [EfiMemoryMappedIOPortSpace]          = TRUE,   \
      [EfiPalCode]                          = TRUE,   \
      [EfiPersistentMemory]                 = FALSE,  \
      [EfiUnacceptedMemoryType]             = TRUE,   \
      [OEM_RESERVED_MPS_MEMORY_TYPE]        = TRUE,   \
      [OS_RESERVED_MPS_MEMORY_TYPE]         = TRUE    \
    }                                                 \
  }                                                   \
}

//
//  A memory profile recommended for production. Compared to the debug
//  settings, this profile removes the pool guards and uses page guards
//  for fewer memory types.
//
#define DXE_MEMORY_PROTECTION_SETTINGS_PROD_MODE      \
{                                                     \
  DXE_MEMORY_PROTECTION_SIGNATURE,                    \
  DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION,     \
  TRUE, /* Stack Guard */                             \
  TRUE, /* Stack Execution Protection */              \
  {     /* NULL Pointer Detection */                  \
    .Enabled                                = TRUE,   \
    .DisableEndOfDxe                        = FALSE,  \
    .NonstopModeEnabled                     = FALSE   \
  },                                                  \
  { /* Image Protection */                            \
    .ProtectImageFromUnknown                = FALSE,  \
    .ProtectImageFromFv                     = TRUE    \
  },                                                  \
  { /* Execution Protection */                        \
    .EnabledForType = {                               \
      [EfiReservedMemoryType]               = TRUE,   \
      [EfiLoaderCode]                       = FALSE,  \
      [EfiLoaderData]                       = TRUE,   \
      [EfiBootServicesCode]                 = FALSE,  \
      [EfiBootServicesData]                 = TRUE,   \
      [EfiRuntimeServicesCode]              = FALSE,  \
      [EfiRuntimeServicesData]              = TRUE,   \
      [EfiConventionalMemory]               = TRUE,   \
      [EfiUnusableMemory]                   = TRUE,   \
      [EfiACPIReclaimMemory]                = TRUE,   \
      [EfiACPIMemoryNVS]                    = TRUE,   \
      [EfiMemoryMappedIO]                   = TRUE,   \
      [EfiMemoryMappedIOPortSpace]          = TRUE,   \
      [EfiPalCode]                          = TRUE,   \
      [EfiPersistentMemory]                 = FALSE,  \
      [EfiUnacceptedMemoryType]             = TRUE,   \
      [OEM_RESERVED_MPS_MEMORY_TYPE]        = TRUE,   \
      [OS_RESERVED_MPS_MEMORY_TYPE]         = TRUE    \
    }                                                 \
  },                                                  \
  { /* Heap Guard */                                  \
    .PageGuardEnabled                       = TRUE,   \
    .PoolGuardEnabled                       = FALSE,  \
    .FreedMemoryGuardEnabled                = FALSE,  \
    .NonstopModeEnabled                     = FALSE,  \
    .GuardAlignedToTail                     = FALSE   \
  },                                                  \
  { /* Pool Guard */                                  \
    0                                                 \
  },                                                  \
  { /* Page Guard */                                  \
    .EnabledForType = {                               \
      [EfiReservedMemoryType]               = FALSE,  \
      [EfiLoaderCode]                       = FALSE,  \
      [EfiLoaderData]                       = FALSE,  \
      [EfiBootServicesCode]                 = FALSE,  \
      [EfiBootServicesData]                 = TRUE,   \
      [EfiRuntimeServicesCode]              = FALSE,  \
      [EfiRuntimeServicesData]              = TRUE,   \
      [EfiConventionalMemory]               = FALSE,  \
      [EfiUnusableMemory]                   = FALSE,  \
      [EfiACPIReclaimMemory]                = FALSE,  \
      [EfiACPIMemoryNVS]                    = FALSE,  \
      [EfiMemoryMappedIO]                   = FALSE,  \
      [EfiMemoryMappedIOPortSpace]          = FALSE,  \
      [EfiPalCode]                          = FALSE,  \
      [EfiPersistentMemory]                 = FALSE,  \
      [EfiUnacceptedMemoryType]             = FALSE,  \
      [OEM_RESERVED_MPS_MEMORY_TYPE]        = FALSE,  \
      [OS_RESERVED_MPS_MEMORY_TYPE]         = FALSE   \
    }                                                 \
  }                                                   \
}

//
//  A memory profile which mirrors DXE_MEMORY_PROTECTION_SETTINGS_PROD_MODE
//  but doesn't include page guards.
//
#define DXE_MEMORY_PROTECTION_SETTINGS_PROD_MODE_NO_PAGE_GUARDS   \
{                                                                 \
  DXE_MEMORY_PROTECTION_SIGNATURE,                                \
  DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION,                 \
  TRUE, /* Stack Guard */                                         \
  TRUE, /* Stack Execution Protection */                          \
  {     /* NULL Pointer Detection */                              \
    .Enabled                                = TRUE,               \
    .DisableEndOfDxe                        = FALSE,              \
    .NonstopModeEnabled                     = FALSE               \
  },                                                              \
  { /* Image Protection */                                        \
    .ProtectImageFromUnknown                = FALSE,              \
    .ProtectImageFromFv                     = TRUE                \
  },                                                              \
  { /* Execution Protection */                                    \
    .EnabledForType = {                                           \
      [EfiReservedMemoryType]               = TRUE,               \
      [EfiLoaderCode]                       = FALSE,              \
      [EfiLoaderData]                       = TRUE,               \
      [EfiBootServicesCode]                 = FALSE,              \
      [EfiBootServicesData]                 = TRUE,               \
      [EfiRuntimeServicesCode]              = FALSE,              \
      [EfiRuntimeServicesData]              = TRUE,               \
      [EfiConventionalMemory]               = TRUE,               \
      [EfiUnusableMemory]                   = TRUE,               \
      [EfiACPIReclaimMemory]                = TRUE,               \
      [EfiACPIMemoryNVS]                    = TRUE,               \
      [EfiMemoryMappedIO]                   = TRUE,               \
      [EfiMemoryMappedIOPortSpace]          = TRUE,               \
      [EfiPalCode]                          = TRUE,               \
      [EfiPersistentMemory]                 = FALSE,              \
      [EfiUnacceptedMemoryType]             = TRUE,               \
      [OEM_RESERVED_MPS_MEMORY_TYPE]        = TRUE,               \
      [OS_RESERVED_MPS_MEMORY_TYPE]         = TRUE                \
    }                                                             \
  },                                                              \
  { /* Heap Guard */                                              \
    0                                                             \
  },                                                              \
  { /* Pool Guard */                                              \
    0                                                             \
  },                                                              \
  { /* Page Guard */                                              \
    0                                                             \
  }                                                               \
}

//
//  A memory profile which disables all DXE memory protection settings.
//
#define DXE_MEMORY_PROTECTION_SETTINGS_OFF            \
{                                                     \
  DXE_MEMORY_PROTECTION_SIGNATURE,                    \
  DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION,     \
  FALSE, /* Stack Guard */                            \
  FALSE, /* Stack Execution Protection */             \
  {      /* NULL Pointer Detection */                 \
    0                                                 \
  },                                                  \
  { /* Image Protection */                            \
    0                                                 \
  },                                                  \
  { /* Execution Protection */                        \
    0                                                 \
  },                                                  \
  { /* Heap Guard */                                  \
    0                                                 \
  },                                                  \
  { /* Pool Guard */                                  \
    0                                                 \
  },                                                  \
  { /* Page Guard */                                  \
    0                                                 \
  }                                                   \
}

////////////////////////////
// MM PROFILE DEFINITIONS //
////////////////////////////

//
//  A memory profile ideal for development scenarios.
//
#define MM_MEMORY_PROTECTION_SETTINGS_DEBUG  {     \
  MM_MEMORY_PROTECTION_SIGNATURE,                  \
  MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION,   \
  { /* NULL Pointer Detection */                   \
    .Enabled                             = TRUE,   \
    .NonstopModeEnabled                  = TRUE    \
  },                                               \
  { /* Heap Guard */                               \
    .PageGuardEnabled                    = TRUE,   \
    .PoolGuardEnabled                    = TRUE,   \
    .NonstopModeEnabled                  = TRUE,   \
    .GuardAlignedToTail                  = FALSE   \
  },                                               \
  { /* Pool Guard */                               \
    .EnabledForType = {                            \
      [EfiReservedMemoryType]            = FALSE,  \
      [EfiLoaderCode]                    = FALSE,  \
      [EfiLoaderData]                    = FALSE,  \
      [EfiBootServicesCode]              = FALSE,  \
      [EfiBootServicesData]              = TRUE,   \
      [EfiRuntimeServicesCode]           = FALSE,  \
      [EfiRuntimeServicesData]           = TRUE,   \
      [EfiConventionalMemory]            = FALSE,  \
      [EfiUnusableMemory]                = FALSE,  \
      [EfiACPIReclaimMemory]             = FALSE,  \
      [EfiACPIMemoryNVS]                 = FALSE,  \
      [EfiMemoryMappedIO]                = FALSE,  \
      [EfiMemoryMappedIOPortSpace]       = FALSE,  \
      [EfiPalCode]                       = FALSE,  \
      [EfiPersistentMemory]              = FALSE,  \
      [EfiUnacceptedMemoryType]          = FALSE,  \
      [OEM_RESERVED_MPS_MEMORY_TYPE]     = FALSE,  \
      [OS_RESERVED_MPS_MEMORY_TYPE]      = FALSE   \
    }                                              \
  },                                               \
  { /* Page Guard */                               \
    .EnabledForType = {                            \
      [EfiReservedMemoryType]            = FALSE,  \
      [EfiLoaderCode]                    = FALSE,  \
      [EfiLoaderData]                    = FALSE,  \
      [EfiBootServicesCode]              = FALSE,  \
      [EfiBootServicesData]              = TRUE,   \
      [EfiRuntimeServicesCode]           = FALSE,  \
      [EfiRuntimeServicesData]           = TRUE,   \
      [EfiConventionalMemory]            = FALSE,  \
      [EfiUnusableMemory]                = FALSE,  \
      [EfiACPIReclaimMemory]             = FALSE,  \
      [EfiACPIMemoryNVS]                 = FALSE,  \
      [EfiMemoryMappedIO]                = FALSE,  \
      [EfiMemoryMappedIOPortSpace]       = FALSE,  \
      [EfiPalCode]                       = FALSE,  \
      [EfiPersistentMemory]              = FALSE,  \
      [EfiUnacceptedMemoryType]          = FALSE,  \
      [OEM_RESERVED_MPS_MEMORY_TYPE]     = FALSE,  \
      [OS_RESERVED_MPS_MEMORY_TYPE]      = FALSE   \
    }                                              \
  }                                                \
}

//
//  A memory profile which disables all MM memory protection settings.
//
#define MM_MEMORY_PROTECTION_SETTINGS_OFF           \
{                                                   \
  MM_MEMORY_PROTECTION_SIGNATURE,                   \
  MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION,    \
  { /* NULL Pointer Detection */                    \
    0                                               \
  },                                                \
  { /* Heap Guard */                                \
    0                                               \
  },                                                \
  { /* Pool Guard */                                \
    0                                               \
  },                                                \
  { /* Page Guard */                                \
    0                                               \
  }                                                 \
}

////////////////////////////
// PROFILE CONFIGURATIONS //
////////////////////////////

DXE_MEMORY_PROTECTION_PROFILES  DxeMemoryProtectionProfiles[DxeMemoryProtectionSettingsMax] = {
  {
    .Name        = L"Debug",
    .Description = L"Development profile ideal for debug scenarios",
    .Settings    = DXE_MEMORY_PROTECTION_SETTINGS_DEBUG
  },
  {
    .Name        = L"Production",
    .Description = L"Production profile recommended for production scenarios",
    .Settings    = DXE_MEMORY_PROTECTION_SETTINGS_PROD_MODE
  },
  {
    .Name        = L"ProductionNoPageGuards",
    .Description = L"Production profile without page guards recommended for performance sensitive production scenarios",
    .Settings    = DXE_MEMORY_PROTECTION_SETTINGS_PROD_MODE_NO_PAGE_GUARDS
  },
  {
    .Name        = L"Off",
    .Description = L"Disables all memory protection settings",
    .Settings    = DXE_MEMORY_PROTECTION_SETTINGS_OFF
  }
};

MM_MEMORY_PROTECTION_PROFILES  MmMemoryProtectionProfiles[MmMemoryProtectionSettingsMax] = {
  {
    .Name        = L"Debug",
    .Description = L"Development profile ideal for debug scenarios",
    .Settings    = MM_MEMORY_PROTECTION_SETTINGS_DEBUG
  },
  {
    .Name        = L"Off",
    .Description = L"Disables all memory protection settings",
    .Settings    = MM_MEMORY_PROTECTION_SETTINGS_OFF
  }
};

/////////////////////////////////////
//    GET/SET SUPPORT FUNCTIONS    //
/////////////////////////////////////

/**
  Allocates and zeros memory for the memory protection settings and initializes the current
  version number for MM and DXE.

  @retval EFI_SUCCESS           The memory protection settings were successfully allocated.
  @retval EFI_OUT_OF_RESOURCES  There was insufficient memory to allocate the settings or build
                                the gMemoryProtectionSettingsGuid HOB entry.
  @retval EFI_ABORTED           Failed to build the gMemoryProtectionSettingsGuid HOB entry.
  @retval EFI_ALREADY_STARTED   The memory protection settings have already been created.
**/
STATIC
EFI_STATUS
CreateMemoryProtectionSettings (
  OUT MEMORY_PROTECTION_SETTINGS_PRIVATE  **Mpsp
  )
{
  VOID                                *Ptr;
  MEMORY_PROTECTION_SETTINGS_PRIVATE  Mps;

  if (mMpsp != NULL) {
    return EFI_ALREADY_STARTED;
  }

  ZeroMem (&Mps, sizeof (Mps));

  Ptr = BuildGuidDataHob (
          &gMemoryProtectionSettingsGuid,
          &Mps,
          sizeof (Mps)
          );

  if (Ptr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Ptr = GetFirstGuidHob (&gMemoryProtectionSettingsGuid);
  if (Ptr == NULL) {
    ASSERT (Ptr != NULL);
    return EFI_ABORTED;
  }

  *Mpsp                          =  (MEMORY_PROTECTION_SETTINGS_PRIVATE *)GET_GUID_HOB_DATA (Ptr);
  (*Mpsp)->Mps.Dxe.StructVersion = DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION;
  (*Mpsp)->Mps.Dxe.Signature     = DXE_MEMORY_PROTECTION_SIGNATURE;
  (*Mpsp)->Mps.Mm.StructVersion  = MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION;
  (*Mpsp)->Mps.Mm.Signature      = MM_MEMORY_PROTECTION_SIGNATURE;

  return EFI_SUCCESS;
}

/**
  Populates mMpsp global. If the memory protection settings have been allocated (indicated by the
  gMemoryProtectionSettingsGuid HOB), then this function will populate mMpsp with the address
  present in the gMemoryProtectionSettingsGuid HOB. If the memory protection settings have
  not been allocated, this function will allocate them.

  @retval EFI_SUCCESS               mMpsp global was populated.
  @retval EFI_OUT_OF_RESOURCES      The memory protection settings could not be allocated.
  @retval EFI_ABORTED               The version number of the DXE or MM memory protection settings was invalid.
  @retval EFI_UNSUPPORTED           NULL implementation called.
**/
STATIC
EFI_STATUS
PopulateMpsGlobal (
  VOID
  )
{
  VOID                                *Ptr;
  MEMORY_PROTECTION_SETTINGS_PRIVATE  *Mpsp;

  if (mMpsp != NULL) {
    return EFI_SUCCESS;
  }

  Ptr = GetFirstGuidHob (&gMemoryProtectionSettingsGuid);

  // If the HOB is not present, create the memory protection settings
  if ((Ptr == NULL)) {
    if (EFI_ERROR (CreateMemoryProtectionSettings (&Mpsp))) {
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    Mpsp = (MEMORY_PROTECTION_SETTINGS_PRIVATE *)GET_GUID_HOB_DATA (Ptr);
  }

  DEBUG ((DEBUG_INFO, "%a: - Memory Protection Settings Address: 0x%p\n", __func__, Mpsp));

  if (Mpsp == NULL) {
    return EFI_ABORTED;
  }

  if (Mpsp->Mps.Dxe.StructVersion != DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: - Version number of the DXE Memory Protection Settings is invalid!\n"
      "This module was compiled with version %d but the current version is %d.\n",
      __func__,
      Mpsp->Mps.Dxe.StructVersion,
      DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION
      ));
    ASSERT (Mpsp->Mps.Dxe.StructVersion == DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION);
    return EFI_ABORTED;
  }

  if (Mpsp->Mps.Mm.StructVersion != MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: - Version number of the MM Memory Protection Settings is invalid!\n"
      "This module was compiled with version %d but the current version is %d.\n",
      __func__,
      Mpsp->Mps.Mm.StructVersion,
      MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION
      ));
    ASSERT (Mpsp->Mps.Mm.StructVersion == MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION);
    return EFI_ABORTED;
  }

  mMpsp = Mpsp;

  return EFI_SUCCESS;
}

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
**/
EFI_STATUS
EFIAPI
LockMemoryProtectionSettings (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if (mMpsp == NULL) {
    Status = PopulateMpsGlobal ();
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  }

  mMpsp->MemoryProtectionSettingsLocked = TRUE;

  return Status;
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
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if ((mMpsp == NULL)) {
    Status = PopulateMpsGlobal ();
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  }

  if (mMpsp->MemoryProtectionSettingsLocked) {
    return EFI_ACCESS_DENIED;
  }

  if (DxeMps == NULL) {
    if (ProfileIndex >= DxeMemoryProtectionSettingsMax) {
      return EFI_INVALID_PARAMETER;
    }

    DxeMps = &DxeMemoryProtectionProfiles[ProfileIndex].Settings;
  } else if (DxeMps->StructVersion != mMpsp->Mps.Dxe.StructVersion) {
    ASSERT (DxeMps->StructVersion == mMpsp->Mps.Dxe.StructVersion);
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (&mMpsp->Mps.Dxe, DxeMps, sizeof (DXE_MEMORY_PROTECTION_SETTINGS));

  return Status;
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
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if ((mMpsp == NULL)) {
    Status = PopulateMpsGlobal ();
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  }

  if (mMpsp->MemoryProtectionSettingsLocked) {
    return EFI_ACCESS_DENIED;
  }

  if (MmMps == NULL) {
    if (ProfileIndex >= MmMemoryProtectionSettingsMax) {
      return EFI_INVALID_PARAMETER;
    }

    MmMps = &MmMemoryProtectionProfiles[ProfileIndex].Settings;
  } else if (MmMps->StructVersion != mMpsp->Mps.Mm.StructVersion) {
    ASSERT (MmMps->StructVersion == mMpsp->Mps.Mm.StructVersion);
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (&mMpsp->Mps.Mm, MmMps, sizeof (MM_MEMORY_PROTECTION_SETTINGS));

  return Status;
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
  EFI_STATUS  Status;

  if (Mps == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (mMpsp == NULL) {
    Status = PopulateMpsGlobal ();
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (mMpsp != NULL) {
    CopyMem (Mps, &mMpsp->Mps, sizeof (MEMORY_PROTECTION_SETTINGS));
  } else {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
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
  if (mMpsp == NULL) {
    if (EFI_ERROR (PopulateMpsGlobal ())) {
      ASSERT (mMpsp != NULL);
      return FALSE;
    }
  }

  return mMpsp->Mps.Dxe.CpuStackGuardEnabled                                                                                                     ||
         mMpsp->Mps.Dxe.StackExecutionProtectionEnabled                                                                                          ||
         mMpsp->Mps.Dxe.NullPointerDetection.Enabled                                                                                             ||
         mMpsp->Mps.Dxe.HeapGuard.FreedMemoryGuardEnabled                                                                                        ||
         mMpsp->Mps.Dxe.ImageProtection.ProtectImageFromFv                                                                                       ||
         mMpsp->Mps.Dxe.ImageProtection.ProtectImageFromUnknown                                                                                  ||
         !IsZeroBuffer (&mMpsp->Mps.Dxe.ExecutionProtection.EnabledForType, MPS_MEMORY_TYPE_BUFFER_SIZE)                                         ||
         (mMpsp->Mps.Dxe.HeapGuard.PageGuardEnabled && !IsZeroBuffer (&mMpsp->Mps.Dxe.PageGuard.EnabledForType, MPS_MEMORY_TYPE_BUFFER_SIZE))    ||
         (mMpsp->Mps.Dxe.HeapGuard.PoolGuardEnabled && !IsZeroBuffer (&mMpsp->Mps.Dxe.PoolGuard.EnabledForType, MPS_MEMORY_TYPE_BUFFER_SIZE));
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
  if (mMpsp == NULL) {
    if (EFI_ERROR (PopulateMpsGlobal ())) {
      ASSERT (mMpsp != NULL);
      return FALSE;
    }
  }

  return mMpsp->Mps.Mm.NullPointerDetection.Enabled                                                                                            ||
         (mMpsp->Mps.Mm.HeapGuard.PageGuardEnabled && !IsZeroBuffer (&mMpsp->Mps.Mm.PageGuard.EnabledForType, MPS_MEMORY_TYPE_BUFFER_SIZE))    ||
         (mMpsp->Mps.Dxe.HeapGuard.PoolGuardEnabled && !IsZeroBuffer (&mMpsp->Mps.Mm.PoolGuard.EnabledForType, MPS_MEMORY_TYPE_BUFFER_SIZE));
}
