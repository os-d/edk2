/** @file

Defines memory protection settings guid and struct for DXE.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef DXE_MEMORY_PROTECTION_SETTINGS_H_
#define DXE_MEMORY_PROTECTION_SETTINGS_H_

#include <Library/BaseMemoryLib.h>

// Current iteration of DXE_MEMORY_PROTECTION_SETTINGS
#define DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION  1

#define OEM_RESERVED_MPS_MEMORY_TYPE     EfiMaxMemoryType
#define OS_RESERVED_MPS_MEMORY_TYPE      (EfiMaxMemoryType + 1)
#define MAX_DXE_MPS_MEMORY_TYPE          (EfiMaxMemoryType + 2)
#define DXE_MPS_MEMORY_TYPE_BUFFER_SIZE  (MAX_DXE_MPS_MEMORY_TYPE * sizeof (BOOLEAN))

typedef struct {
  BOOLEAN    Enabled            : 1;
  BOOLEAN    DisableEndOfDxe    : 1;
  BOOLEAN    NonstopModeEnabled : 1;
} DXE_NULL_DETECTION_POLICY;

typedef struct {
  BOOLEAN    ProtectImageFromUnknown : 1;
  BOOLEAN    ProtectImageFromFv      : 1;
} DXE_IMAGE_PROTECTION_POLICY;

typedef struct {
  BOOLEAN    PageGuardEnabled        : 1;
  BOOLEAN    PoolGuardEnabled        : 1;
  BOOLEAN    FreedMemoryGuardEnabled : 1;
  BOOLEAN    NonstopModeEnabled      : 1;
  BOOLEAN    GuardAlignedToTail      : 1;
} DXE_HEAP_GUARD_POLICY;

typedef struct {
  BOOLEAN    EnabledForType[MAX_DXE_MPS_MEMORY_TYPE];
} DXE_MPS_MEMORY_TYPES;

typedef UINT8 DXE_MEMORY_PROTECTION_SETTINGS_VERSION;

//
// Memory Protection Settings struct
//
typedef struct {
  // The current version of the structure definition. This is used to ensure there isn't a
  // definition mismatch if modules have differing iterations of this header. When creating
  // this struct, use the DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION macro.
  DXE_MEMORY_PROTECTION_SETTINGS_VERSION    StructVersion;

  // If enabled, the page at the top of the stack will be invalidated to catch stack overflow.
  BOOLEAN                                   CpuStackGuardEnabled;

  // If enabled, the stack will be marked non-executable.
  BOOLEAN                                   StackExecutionProtectionEnabled;

  // If enabled, accessing the NULL address in UEFI will be caught by marking
  // the NULL page as not present.
  //   .NullDetectionEnabled    : Enable NULL pointer detection.
  //   .DisableEndOfDxe         : Disable NULL pointer detection just after EndOfDxe.
  //                              This is a workaround for those unsolvable NULL access issues in
  //                              OptionROM, boot loader, etc. It can also help to avoid unnecessary
  //                              exception caused by legacy memory (0-4095) access after EndOfDxe,
  //                              such as Windows 7 boot on Qemu.
  //   .NonstopModeEnabled      : If enabled the debug flag will be raised when a fault occurs
  //                              to break into debugger.
  DXE_NULL_DETECTION_POLICY    NullPointerDetection;

  // Set image protection policy.
  //
  //  .ProtectImageFromUnknown          : If set, images from unknown devices will be protected by
  //                                      DxeCore if they are aligned. The code section becomes
  //                                      read-only, and the data section becomes non-executable.
  //  .ProtectImageFromFv               : If set, images from firmware volumes will be protected by
  //                                      DxeCore if they are aligned. The code section becomes
  //                                      read-only, and the data section becomes non-executable.
  DXE_IMAGE_PROTECTION_POLICY    ImageProtection;

  // If a bit is set, memory regions of the associated type will be mapped non-executable.
  //
  // The execution protection setting for EfiBootServicesData and EfiConventionalMemory must
  // be the same.
  DXE_MPS_MEMORY_TYPES           ExecutionProtection;

  //  Configures general heap guard behavior.
  //
  //  .PageGuardEnabled         : Enable page guard.
  //  .PoolGuardEnabled         : Enable pool guard.
  //  .FreedMemoryGuardEnabled  : Enable freed-memory guard (Use-After-Free memory detection).
  //  .NonstopModeEnabled       : If enabled the debug flag will be raised when a fault occurs
  //                              to break into debugger.
  //  .GuardAlignedToTail       : TRUE if the pool is aligned to tail guard page. If FALSE, the
  //                              pool is aligned to head guard page.
  //
  //  Note:
  //  a) Due to the limit of pool memory implementation and the alignment
  //     requirement of UEFI spec, HeapGuard.GuardAlignedToTail is a try-best
  //     setting which cannot guarantee that the returned pool is exactly
  //     adjacent to head or tail guard page.
  //  b) Freed-memory guard and pool/page guard cannot be enabled
  //     at the same time.
  DXE_HEAP_GUARD_POLICY    HeapGuard;

  // Indicates which type allocation need guard page.
  //
  // If bit is set, a head guard page and a tail guard page will be added just
  // before and after corresponding type of pages which the allocated pool occupies,
  // if there's enough free memory for all of them.
  //
  // These settings are only valid if HeapGuard.PoolGuardEnabled is TRUE.
  DXE_MPS_MEMORY_TYPES    PoolGuard;

  // Indicates which type allocation need guard page.
  //
  // If a bit is set, a head guard page and a tail guard page will be added just
  // before and after corresponding type of pages allocated if there's enough
  // free pages for all of them.
  //
  // These settings are only valid if HeapGuard.PageGuardEnabled is TRUE.
  DXE_MPS_MEMORY_TYPES    PageGuard;
} DXE_MEMORY_PROTECTION_SETTINGS;

#define DXE_MPS_IS_STRUCT_VALID(DxeMpsPtr) \
  (((DXE_MEMORY_PROTECTION_SETTINGS *)DxeMpsPtr)->StructVersion == DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION)

#define DXE_MPS_IS_ANY_MEMORY_TYPE_ACTIVE(MpsMemoryTypesPtr) \
  (!IsZeroBuffer (&((DXE_MPS_MEMORY_TYPES *)MpsMemoryTypesPtr)->EnabledForType, DXE_MPS_MEMORY_TYPE_BUFFER_SIZE))

#define DXE_MPS_IS_IMAGE_PROTECTION_ENABLED(DxeMpsPtr) \
  (((DXE_MEMORY_PROTECTION_SETTINGS*)DxeMpsPtr)->ImageProtection.ProtectImageFromFv || \
  ((DXE_MEMORY_PROTECTION_SETTINGS*)DxeMpsPtr)->ImageProtection.ProtectImageFromUnknown)

#define DXE_MPS_IS_EXECUTION_PROTECTION_ENABLED(DxeMpsPtr) \
  DXE_MPS_IS_ANY_MEMORY_TYPE_ACTIVE(&((DXE_MEMORY_PROTECTION_SETTINGS*)DxeMpsPtr)->ExecutionProtection)

#define DXE_MPS_ARE_PAGE_GUARDS_ENABLED(DxeMpsPtr) \
  (((DXE_MEMORY_PROTECTION_SETTINGS*)DxeMpsPtr)->HeapGuard.PageGuardEnabled  && \
  DXE_MPS_IS_ANY_MEMORY_TYPE_ACTIVE(&((DXE_MEMORY_PROTECTION_SETTINGS*)DxeMpsPtr)->PageGuard))

#define DXE_MPS_ARE_POOL_GUARDS_ENABLED(DxeMpsPtr) \
  (((DXE_MEMORY_PROTECTION_SETTINGS*)DxeMpsPtr)->HeapGuard.PoolGuardEnabled  && \
  DXE_MPS_IS_ANY_MEMORY_TYPE_ACTIVE(&((DXE_MEMORY_PROTECTION_SETTINGS*)DxeMpsPtr)->PoolGuard))

#define DXE_MPS_IS_MEMORY_PROTECTION_ACTIVE(DxeMpsPtr)                                   \
  (DXE_MPS_IS_STRUCT_VALID(DxeMpsPtr)                                               &&   \
   (((DXE_MEMORY_PROTECTION_SETTINGS*)DxeMpsPtr)->CpuStackGuardEnabled              ||   \
    ((DXE_MEMORY_PROTECTION_SETTINGS*)DxeMpsPtr)->StackExecutionProtectionEnabled   ||   \
    ((DXE_MEMORY_PROTECTION_SETTINGS*)DxeMpsPtr)->NullPointerDetection.Enabled      ||   \
    DXE_MPS_IS_IMAGE_PROTECTION_ENABLED(DxeMpsPtr)                                  ||   \
    DXE_MPS_IS_EXECUTION_PROTECTION_ENABLED(DxeMpsPtr)                              ||   \
    DXE_MPS_ARE_PAGE_GUARDS_ENABLED(DxeMpsPtr)                                      ||   \
    DXE_MPS_ARE_POOL_GUARDS_ENABLED(DxeMpsPtr))                                          \
  )

#define HOB_DXE_MEMORY_PROTECTION_SETTINGS_GUID  \
  { \
    { 0x9ABFD639, 0xD1D0, 0x4EFF, { 0xBD, 0xB6, 0x7E, 0xC4, 0x19, 0x0D, 0x17, 0xD5 } } \
  }

extern GUID  gDxeMemoryProtectionSettingsGuid;

//
//  A memory profile with strict settings ideal for development scenarios.
//
#define DXE_MEMORY_PROTECTION_SETTINGS_DEBUG  {                 \
            DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION,     \
            TRUE, /* Stack Guard */                             \
            TRUE, /* Stack Execution Protection */              \
            {     /* NULL Pointer Detection */                  \
              .Enabled                                = TRUE,   \
              .DisableEndOfDxe                        = TRUE,  \
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
              .PoolGuardEnabled                       = TRUE,  \
              .FreedMemoryGuardEnabled                = FALSE,  \
              .NonstopModeEnabled                     = TRUE,   \
              .GuardAlignedToTail                     = FALSE   \
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
          };

//
//  A memory profile recommended for production. Compared to the debug
//  settings, this profile removes the pool guards and uses page guards
//  for fewer memory types.
//
#define DXE_MEMORY_PROTECTION_SETTINGS_PROD_MODE                \
          {                                                     \
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
          };

//
//  A memory profile which mirrors DXE_MEMORY_PROTECTION_SETTINGS_PROD_MODE
//  but doesn't include page guards.
//
#define DXE_MEMORY_PROTECTION_SETTINGS_PROD_MODE_NO_PAGE_GUARDS \
          {                                                     \
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
              0                                                 \
            },                                                  \
            { /* Pool Guard */                                  \
              0                                                 \
            },                                                  \
            { /* Page Guard */                                  \
              0                                                 \
            }                                                   \
          };

//
//  A memory profile which disables all DXE memory protection settings.
//
#define DXE_MEMORY_PROTECTION_SETTINGS_OFF                      \
          {                                                     \
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
          };

#endif
