/** @file

Defines memory protection settings guid and struct for MM.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MM_MEMORY_PROTECTION_SETTINGS_H_
#define MM_MEMORY_PROTECTION_SETTINGS_H_

#include <Library/BaseMemoryLib.h>

// Current iteration of MM_MEMORY_PROTECTION_SETTINGS
#define MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION  1

#define OEM_RESERVED_MPS_MEMORY_TYPE    EfiMaxMemoryType
#define OS_RESERVED_MPS_MEMORY_TYPE     (EfiMaxMemoryType + 1)
#define MAX_MM_MPS_MEMORY_TYPE          (EfiMaxMemoryType + 2)
#define MM_MPS_MEMORY_TYPE_BUFFER_SIZE  (MAX_MM_MPS_MEMORY_TYPE * sizeof (BOOLEAN))

typedef struct {
  BOOLEAN    Enabled            : 1;
  BOOLEAN    NonstopModeEnabled : 1;
} MM_NULL_DETECTION_POLICY;

typedef struct {
  BOOLEAN    PageGuardEnabled   : 1;
  BOOLEAN    PoolGuardEnabled   : 1;
  BOOLEAN    NonstopModeEnabled : 1;
  BOOLEAN    GuardAlignedToTail : 1;
} MM_HEAP_GUARD_POLICY;

typedef struct {
  BOOLEAN    EnabledForType[MAX_MM_MPS_MEMORY_TYPE];
} MM_MPS_MEMORY_TYPES;

typedef UINT8 MM_MEMORY_PROTECTION_SETTINGS_VERSION;

//
// Memory Protection Settings struct
//
typedef struct {
  // The current version of the structure definition. This is used to ensure there isn't a
  // definition mismatch if modules have differing iterations of this header. When creating
  // this struct, use the MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION macro.
  MM_MEMORY_PROTECTION_SETTINGS_VERSION    StructVersion;

  // If enabled, accessing the NULL address in MM will be caught by marking
  // the NULL page as not present.
  //   .NullDetectionEnabled    : Enable NULL pointer detection.
  //   .NonstopModeEnabled      : If enabled the debug flag will be raised when a fault occurs
  //                              to break into debugger.
  MM_NULL_DETECTION_POLICY                 NullPointerDetection;

  //  Configures general heap guard behavior.
  //
  // Note:
  //  a) Due to the limit of pool memory implementation and the alignment
  //     requirement of UEFI spec, HeapGuard.GuardAlignedToTail is a try-best
  //     setting which cannot guarantee that the returned pool is exactly
  //     adjacent to head or tail guard page.
  //
  //  .PageGuardEnabled          : Enable page guard.
  //  .PoolGuardEnabled          : Enable pool guard.
  //  .NonstopModeEnabled        : If enabled the debug flag will be raised when a fault occurs
  //                               to break into debugger.
  //  .GuardAlignedToTail        : TRUE if the pool is aligned to tail guard page. If FALSE, the
  //                               pool is aligned to head guard page.
  MM_HEAP_GUARD_POLICY    HeapGuard;

  // Indicates which type allocation need guard page.
  //
  // If bit is set, a head guard page and a tail guard page will be added just
  // before and after corresponding type of pages which the allocated pool occupies,
  // if there's enough free memory for all of them.
  //
  // These settings are only valid if PoolGuardEnabled is TRUE in HeapGuard.
  MM_MPS_MEMORY_TYPES    PoolGuard;

  // Indicates which type allocation need guard page.
  //
  // If a bit is set, a head guard page and a tail guard page will be added just
  // before and after corresponding type of pages allocated if there's enough
  // free pages for all of them.
  //
  // This bitfield is only valid if PageGuardEnabled is TRUE in HeapGuard.
  MM_MPS_MEMORY_TYPES    PageGuard;
} MM_MEMORY_PROTECTION_SETTINGS;

#define MM_MPS_IS_STRUCT_VALID(MmMpsPtr) \
  (((MM_MEMORY_PROTECTION_SETTINGS *)MmMpsPtr)->StructVersion == MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION)

#define MM_MPS_IS_ANY_MEMORY_TYPE_ACTIVE(MpsMemoryTypesPtr) \
  (!IsZeroBuffer (&((MM_MPS_MEMORY_TYPES *)MpsMemoryTypesPtr)->EnabledForType, MM_MPS_MEMORY_TYPE_BUFFER_SIZE))

#define MM_MPS_IS_IMAGE_PROTECTION_ENABLED(MmMpsPtr) \
  (((MM_MEMORY_PROTECTION_SETTINGS*)MmMpsPtr)->ImageProtection.ProtectImageFromFv || \
  ((MM_MEMORY_PROTECTION_SETTINGS*)MmMpsPtr)->ImageProtection.ProtectImageFromUnknown)

#define MM_MPS_IS_EXECUTION_PROTECTION_ENABLED(MmMpsPtr) \
  MM_MPS_IS_ANY_MEMORY_TYPE_ACTIVE(&((MM_MEMORY_PROTECTION_SETTINGS*)MmMpsPtr)->ExecutionProtection)

#define MM_MPS_ARE_PAGE_GUARDS_ENABLED(MmMpsPtr) \
  (((MM_MEMORY_PROTECTION_SETTINGS*)MmMpsPtr)->HeapGuard.PageGuardEnabled  && \
  MM_MPS_IS_ANY_MEMORY_TYPE_ACTIVE(&((MM_MEMORY_PROTECTION_SETTINGS*)MmMpsPtr)->PageGuard))

#define MM_MPS_ARE_POOL_GUARDS_ENABLED(MmMpsPtr) \
  (((MM_MEMORY_PROTECTION_SETTINGS*)MmMpsPtr)->HeapGuard.PoolGuardEnabled  && \
  MM_MPS_IS_ANY_MEMORY_TYPE_ACTIVE(&((MM_MEMORY_PROTECTION_SETTINGS*)MmMpsPtr)->PoolGuard))

#define MM_MPS_IS_MEMORY_PROTECTION_ACTIVE(MmMpsPtr)                                   \
  (MM_MPS_IS_STRUCT_VALID(MmMpsPtr)                                               &&   \
   (((MM_MEMORY_PROTECTION_SETTINGS*)MmMpsPtr)->CpuStackGuardEnabled              ||   \
    ((MM_MEMORY_PROTECTION_SETTINGS*)MmMpsPtr)->StackExecutionProtectionEnabled   ||   \
    ((MM_MEMORY_PROTECTION_SETTINGS*)MmMpsPtr)->NullPointerDetection.Enabled      ||   \
    MM_MPS_IS_IMAGE_PROTECTION_ENABLED(MmMpsPtr)                                  ||   \
    MM_MPS_IS_EXECUTION_PROTECTION_ENABLED(MmMpsPtr)                              ||   \
    MM_MPS_ARE_PAGE_GUARDS_ENABLED(MmMpsPtr)                                      ||   \
    MM_MPS_ARE_POOL_GUARDS_ENABLED(MmMpsPtr))                                          \
  )

#define HOB_MM_MEMORY_PROTECTION_SETTINGS_GUID \
  { \
    { 0x0CF445DD, 0xA67C, 0x4F8C, { 0x81, 0x9B, 0xB7, 0xB6, 0x86, 0xED, 0x7C, 0x75 } } \
  }

extern GUID  gMmMemoryProtectionSettingsGuid;

//
//  A memory profile ideal for development scenarios.
//
#define MM_MEMORY_PROTECTION_SETTINGS_DEBUG  {               \
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
#define MM_MEMORY_PROTECTION_SETTINGS_OFF  {                 \
            MM_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION,   \
            { /* NULL Pointer Detection */                   \
              0                                              \
            },                                               \
            { /* Heap Guard */                               \
              0                                              \
            },                                               \
            { /* Pool Guard */                               \
              0                                              \
            },                                               \
            { /* Page Guard */                               \
              0                                              \
            }                                                \
          }

#endif
