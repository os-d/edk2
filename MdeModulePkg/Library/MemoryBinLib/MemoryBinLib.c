/** @file

  Shared logic between cores to work with memory bins for S4 resume stability.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiMultiPhase.h>

#include <Guid/MemoryTypeInformation.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryBinLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>

#include <Pi/PiHob.h>

#define MEMORY_ATTRIBUTE_MASK  (EFI_RESOURCE_ATTRIBUTE_PRESENT             |        \
                                       EFI_RESOURCE_ATTRIBUTE_INITIALIZED         | \
                                       EFI_RESOURCE_ATTRIBUTE_TESTED              | \
                                       EFI_RESOURCE_ATTRIBUTE_READ_PROTECTED      | \
                                       EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTED     | \
                                       EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTED | \
                                       EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTED | \
                                       EFI_RESOURCE_ATTRIBUTE_16_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_32_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_64_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_PERSISTENT          | \
                                       EFI_RESOURCE_ATTRIBUTE_SPECIAL_PURPOSE     )

#define TESTED_MEMORY_ATTRIBUTES  (EFI_RESOURCE_ATTRIBUTE_PRESENT     |     \
                                       EFI_RESOURCE_ATTRIBUTE_INITIALIZED | \
                                       EFI_RESOURCE_ATTRIBUTE_TESTED      )

/**
  Allocates pages from the memory map.

  @param  Type                   The type of allocation to perform
  @param  MemoryType             The type of memory to turn the allocated pages
                                 into
  @param  NumberOfPages          The number of pages to allocate
  @param  Memory                 A pointer to receive the base allocated memory
                                 address

  @return Status. On success, Memory is filled in with the base address allocated
  @retval EFI_INVALID_PARAMETER  Parameters violate checking rules defined in
                                 spec.
  @retval EFI_NOT_FOUND          Could not allocate pages match the requirement.
  @retval EFI_OUT_OF_RESOURCES   No enough pages to allocate.
  @retval EFI_SUCCESS            Pages successfully allocated.

**/
EFI_STATUS
EFIAPI
AllocateBinPages (
  IN  EFI_MEMORY_TYPE       MemoryType,
  IN  UINTN                 NumberOfPages,
  OUT EFI_PHYSICAL_ADDRESS  *Memory
  );

/**
  Frees previous allocated pages.

  @param  Memory                 Base address of memory being freed
  @param  NumberOfPages          The number of pages to free

  @retval EFI_NOT_FOUND          Could not find the entry that covers the range
  @retval EFI_INVALID_PARAMETER  Address not aligned
  @return EFI_SUCCESS         -Pages successfully freed.

**/
EFI_STATUS
EFIAPI
FreeBinPages (
  IN EFI_PHYSICAL_ADDRESS  Memory,
  IN UINTN                 NumberOfPages
  );

EFI_PHYSICAL_ADDRESS  mDefaultMaximumAddress = MAX_ALLOC_ADDRESS;
EFI_PHYSICAL_ADDRESS  mDefaultBaseAddress    = MAX_ALLOC_ADDRESS;

EFI_MEMORY_TYPE_INFORMATION  gMemoryTypeInformation[EfiMaxMemoryType + 1] = {
  { EfiReservedMemoryType,      0 },
  { EfiLoaderCode,              0 },
  { EfiLoaderData,              0 },
  { EfiBootServicesCode,        0 },
  { EfiBootServicesData,        0 },
  { EfiRuntimeServicesCode,     0 },
  { EfiRuntimeServicesData,     0 },
  { EfiConventionalMemory,      0 },
  { EfiUnusableMemory,          0 },
  { EfiACPIReclaimMemory,       0 },
  { EfiACPIMemoryNVS,           0 },
  { EfiMemoryMappedIO,          0 },
  { EfiMemoryMappedIOPortSpace, 0 },
  { EfiPalCode,                 0 },
  { EfiPersistentMemory,        0 },
  { EfiUnacceptedMemoryType,    0 },
  { EfiMaxMemoryType,           0 }
};

EFI_MEMORY_TYPE_STATISTICS  mMemoryTypeStatistics[EfiMaxMemoryType + 1] = {
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  FALSE, FALSE },  // EfiReservedMemoryType
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE, FALSE },  // EfiLoaderCode
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE, FALSE },  // EfiLoaderData
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE, FALSE },  // EfiBootServicesCode
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE, FALSE },  // EfiBootServicesData
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  TRUE,  FALSE },  // EfiRuntimeServicesCode
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  TRUE,  FALSE },  // EfiRuntimeServicesData
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE, FALSE },  // EfiConventionalMemory
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE, FALSE },  // EfiUnusableMemory
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  FALSE, FALSE },  // EfiACPIReclaimMemory
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  FALSE, FALSE },  // EfiACPIMemoryNVS
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE, FALSE },  // EfiMemoryMappedIO
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE, FALSE },  // EfiMemoryMappedIOPortSpace
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  TRUE,  FALSE },  // EfiPalCode
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE, FALSE },  // EfiPersistentMemory
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  FALSE, FALSE },  // EfiUnacceptedMemoryType
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE, FALSE }   // EfiMaxMemoryType
};

BOOLEAN  mMemoryTypeInformationInitialized = FALSE;

/**
  Calculate total memory bin size needed.

  @return The total memory bin size needed.

**/
UINT64
CalculateTotalMemoryBinSizeNeeded (
  VOID
  )
{
  UINTN   Index;
  UINT64  TotalSize;
  UINTN   Granularity;

  Granularity = DEFAULT_PAGE_ALLOCATION_GRANULARITY;

  //
  // Loop through each memory type in the order specified by the gMemoryTypeInformation[] array
  //
  TotalSize = 0;
  for (Index = 0; gMemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
    if ((gMemoryTypeInformation[Index].Type == EfiReservedMemoryType) ||
        (gMemoryTypeInformation[Index].Type == EfiACPIMemoryNVS) ||
        (gMemoryTypeInformation[Index].Type == EfiRuntimeServicesCode) ||
        (gMemoryTypeInformation[Index].Type == EfiRuntimeServicesData))
    {
      Granularity = RUNTIME_PAGE_ALLOCATION_GRANULARITY;
    }

    // gMemoryTypeInformation[Index].NumberOfPages is already aligned to the allocation granularity
    TotalSize += LShiftU64 (gMemoryTypeInformation[Index].NumberOfPages, EFI_PAGE_SHIFT);
  }

  return TotalSize;
}

/**
  Get the Memory Type Information HOB if it exists and populate gMemoryTypeInformation.

  @return EFI_STATUS                      On EFI_SUCCESS, gMemoryTypeInformation points to the
                                          Memory Type Information.
  @return EFI_NOT_FOUND                   No valid Memory Type Information HOB found.
**/
EFI_STATUS
EFIAPI
PopulateMemoryTypeInformation (
  VOID
  )
{
  UINTN                        DataSize;
  EFI_MEMORY_TYPE_INFORMATION  *EfiMemoryTypeInformation;
  EFI_HOB_GUID_TYPE            *GuidHob;
  UINTN                        Index;
  UINT32                       Granularity;

  DEBUG ((DEBUG_ERROR, "OSDDEBUG: %a: Populating Memory Type Information from HOB\n", __func__));
  GuidHob = GetFirstGuidHob (&gEfiMemoryTypeInformationGuid);
  DEBUG ((DEBUG_ERROR, "OSDDEBUG: %a: GuidHob = 0x%p\n", __func__, GuidHob));
  if (GuidHob != NULL) {
    EfiMemoryTypeInformation = GET_GUID_HOB_DATA (GuidHob);
    DataSize                 = GET_GUID_HOB_DATA_SIZE (GuidHob);
    if ((EfiMemoryTypeInformation != NULL) && (DataSize > 0) && (DataSize <= (EfiMaxMemoryType + 1) * sizeof (EFI_MEMORY_TYPE_INFORMATION))) {
      // gMemoryTypeInformation[0] = EfiMemoryTypeInformation[0];
      DEBUG ((DEBUG_ERROR, "OSDDEBUG: %a: Found valid Memory Type Information HOB data gMemoryTypeinformation %llx EfiMemoryTypeInformation %llx DataSize: %llx\n", __func__, gMemoryTypeInformation, EfiMemoryTypeInformation, DataSize));
      CopyMem (&gMemoryTypeInformation, EfiMemoryTypeInformation, DataSize);

      for (Index = 0; gMemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
        //
        // Make sure the memory type in the gMemoryTypeInformation[] array is valid
        //
        if (gMemoryTypeInformation[Index].Type > EfiMaxMemoryType) {
          continue;
        }

        if (gMemoryTypeInformation[Index].NumberOfPages != 0) {
          if ((gMemoryTypeInformation[Index].Type == EfiReservedMemoryType) ||
              (gMemoryTypeInformation[Index].Type == EfiACPIMemoryNVS) ||
              (gMemoryTypeInformation[Index].Type == EfiRuntimeServicesCode) ||
              (gMemoryTypeInformation[Index].Type == EfiRuntimeServicesData))
          {
            Granularity = RUNTIME_PAGE_ALLOCATION_GRANULARITY;
          } else {
            Granularity = DEFAULT_PAGE_ALLOCATION_GRANULARITY;
          }

          // Align the number of pages to the allocation granularity
          gMemoryTypeInformation[Index].NumberOfPages = (UINT32)RShiftU64 (ALIGN_VALUE (LShiftU64 (gMemoryTypeInformation[Index].NumberOfPages, EFI_PAGE_SHIFT), Granularity), EFI_PAGE_SHIFT);
        }
      }

      DEBUG ((DEBUG_ERROR, "OSDDEBUG: %a: Memory Type Information populated\n", __func__));
      return EFI_SUCCESS;
    }

    DEBUG ((DEBUG_ERROR, "%a: Invalid Memory Type Information HOB data\n", __func__));
    ASSERT (FALSE);
  }

  DEBUG ((DEBUG_ERROR, "%a: No Memory Type Information HOB found\n", __func__));

  return EFI_NOT_FOUND;
}

/**
 Look for Resource Descriptor HOB with a ResourceType of System Memory
 and an Owner GUID of gEfiMemoryTypeInformationGuid. If more than 1 is
 found, then set return NULL.

  @param HobStart                         Pointer to the start of the HOB list.
  @param MemoryTypeInformationResourceHob Pointer to the Resource Descriptor HOB if found and valid,
                                          otherwise NULL.

  @return EFI_STATUS                      On EFI_SUCCESS, MemoryTypeInformationResourceHob points to the singular
                                          MemoryTypeInformation Resource Descriptor HOB.
  @return EFI_NOT_FOUND                   No valid MemoryTypeInformation Resource Descriptor HOB found.
  @return EFI_INVALID_PARAMETER           HobStart or MemoryTypeInformationResourceHob is NULL.
**/
EFI_STATUS
EFIAPI
GetMemoryTypeInformationResourceHob (
  IN  VOID                         **HobStart,
  OUT EFI_HOB_RESOURCE_DESCRIPTOR  **MemoryTypeInformationResourceHob
  )
{
  UINTN                        Count;
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceHob;

  if ((HobStart == NULL) || (MemoryTypeInformationResourceHob == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // See if a Memory Type Information HOB is available
  //
  *MemoryTypeInformationResourceHob = NULL;

  Count = 0;
  for (Hob.Raw = *HobStart; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    if (GET_HOB_TYPE (Hob) != EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      continue;
    }

    ResourceHob = Hob.ResourceDescriptor;
    if (!CompareGuid (&ResourceHob->Owner, &gEfiMemoryTypeInformationGuid)) {
      continue;
    }

    Count++;
    if (ResourceHob->ResourceType != EFI_RESOURCE_SYSTEM_MEMORY) {
      continue;
    }

    if ((ResourceHob->ResourceAttribute & MEMORY_ATTRIBUTE_MASK) != TESTED_MEMORY_ATTRIBUTES) {
      continue;
    }

    if (ResourceHob->ResourceLength >= CalculateTotalMemoryBinSizeNeeded ()) {
      *MemoryTypeInformationResourceHob = ResourceHob;
    }

    DEBUG ((DEBUG_ERROR, "%a: Found Memory Type Information Resource Descriptor HOB size too small: 0x%llx vs 0x%llx\n", __func__, ResourceHob->ResourceLength, CalculateTotalMemoryBinSizeNeeded ()));
  }

  if (Count > 1) {
    *MemoryTypeInformationResourceHob = NULL;
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

// STATIC
// VOID
// UpdateStatsFromMemoryAllocationHobs (
//   VOID
//   )
// {
//   EFI_PEI_HOB_POINTERS         Hob;
//   EFI_HOB_MEMORY_ALLOCATION    *MemoryAllocationHob;
//   EFI_MEMORY_TYPE              Type;

//   //
//   // Loop through all Memory Allocation HOBs and update the statistics
//   //
//   for (Hob.Raw = GetHobList (); !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
//     if (GET_HOB_TYPE (Hob) != EFI_HOB_TYPE_MEMORY_ALLOCATION) {
//       continue;
//     }

//     MemoryAllocationHob = Hob.MemoryAllocation;
//     Type                = (EFI_MEMORY_TYPE)(MemoryAllocationHob->AllocDescriptor.MemoryType);
//     if ((UINT32)Type > EfiMaxMemoryType) {
//       continue;
//     }

//     if ((MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress >= mMemoryTypeStatistics[Type].BaseAddress) &&
//         (MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress <= mMemoryTypeStatistics[Type].MaximumAddress))
//     {
//       mMemoryTypeStatistics[Type].CurrentNumberOfPages +=
//         (UINTN)(RShiftU64 (MemoryAllocationHob->AllocDescriptor.MemoryLength, EFI_PAGE_SHIFT));
//     }

//     if (NewType < EfiMaxMemoryType) {
//     if (((Start >= mMemoryTypeStatistics[NewType].BaseAddress) && (Start <= mMemoryTypeStatistics[NewType].MaximumAddress)) ||
//         ((Start >= mDefaultBaseAddress) && (Start <= mDefaultMaximumAddress)))
//     {
//       mMemoryTypeStatistics[NewType].CurrentNumberOfPages += NumberOfPages;
//       if (mMemoryTypeStatistics[NewType].CurrentNumberOfPages > gMemoryTypeInformation[mMemoryTypeStatistics[NewType].InformationIndex].NumberOfPages) {
//         gMemoryTypeInformation[mMemoryTypeStatistics[NewType].InformationIndex].NumberOfPages = (UINT32)mMemoryTypeStatistics[NewType].CurrentNumberOfPages;
//       }
//     }
//   }
//   }
// }

/**
  Sets the preferred memory range to use for the Memory Type Information bins.
  This service must be called before fist call to CoreAddMemoryDescriptor().

  If the location of the Memory Type Information bins has already been
  established or the size of the range provides is smaller than all the
  Memory Type Information bins, then the range provides is not used.

  @param  Start   The start address of the Memory Type Information range.
  @param  Length  The size, in bytes, of the Memory Type Information range.
**/
VOID
EFIAPI
CoreSetMemoryTypeInformationRange (
  IN EFI_PHYSICAL_ADDRESS  Start,
  IN UINT64                Length
  )
{
  EFI_PHYSICAL_ADDRESS  Top;
  EFI_MEMORY_TYPE       Type;
  UINTN                 Index;
  UINTN                 Size;

  //
  // Return if Memory Type Information bin locations have already been set
  //
  if (mMemoryTypeInformationInitialized) {
    DEBUG ((DEBUG_ERROR, "%a: Ignored. Bins already set.\n", __func__));
    return;
  }

  //
  // Return if size of the Memory Type Information bins is greater than Length
  //
  Size = CalculateTotalMemoryBinSizeNeeded ();

  if (Size > Length) {
    return;
  }

  //
  // Loop through each memory type in the order specified by the
  // gMemoryTypeInformation[] array
  //
  Top = Start + Length;
  for (Index = 0; gMemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
    //
    // Make sure the memory type in the gMemoryTypeInformation[] array is valid
    //
    Type = (EFI_MEMORY_TYPE)(gMemoryTypeInformation[Index].Type);
    if ((UINT32)Type > EfiMaxMemoryType) {
      continue;
    }

    if (gMemoryTypeInformation[Index].NumberOfPages != 0) {
      mMemoryTypeStatistics[Type].MaximumAddress = Top - 1;
      Top                                       -= LShiftU64 (gMemoryTypeInformation[Index].NumberOfPages, EFI_PAGE_SHIFT);
      mMemoryTypeStatistics[Type].BaseAddress    = Top;

      DEBUG ((DEBUG_ERROR, "OSDDEBUG45 %a: Memory Type %d assigned bin 0x%llx - 0x%llx\n", __func__, Type, mMemoryTypeStatistics[Type].BaseAddress, mMemoryTypeStatistics[Type].MaximumAddress));

      //
      // If the current base address is the lowest address so far, then update
      // the default maximum address
      //
      if (mMemoryTypeStatistics[Type].BaseAddress < mDefaultMaximumAddress) {
        mDefaultMaximumAddress = mMemoryTypeStatistics[Type].BaseAddress - 1;
      }

      mMemoryTypeStatistics[Type].NumberOfPages   = gMemoryTypeInformation[Index].NumberOfPages;
      gMemoryTypeInformation[Index].NumberOfPages = 0;
    }
  }

  //
  // If the number of pages reserved for a memory type is 0, then all
  // allocations for that type should be in the default range.
  //
  for (Type = (EFI_MEMORY_TYPE)0; Type < EfiMaxMemoryType; Type++) {
    for (Index = 0; gMemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
      if (Type == (EFI_MEMORY_TYPE)gMemoryTypeInformation[Index].Type) {
        mMemoryTypeStatistics[Type].InformationIndex = Index;
      }
    }

    mMemoryTypeStatistics[Type].CurrentNumberOfPages = 0;
    if (mMemoryTypeStatistics[Type].MaximumAddress == MAX_ALLOC_ADDRESS) {
      mMemoryTypeStatistics[Type].MaximumAddress = mDefaultMaximumAddress;
      mMemoryTypeStatistics[Type].DefaultBin     = TRUE;
    }
  }

  // UpdateStatsFromMemoryAllocationHobs ();

  mMemoryTypeInformationInitialized = TRUE;
}

/**
  Allocate memory bins for each memory type as specified in gMemoryTypeInformation.

  If all the memory types cannot be allocated, then all previously allocated
  memory types are freed and the function returns. If this function fails, it will log and expect to be called
  again when more memory is added to the system.

  @param  CreateHob   TRUE to create Memory Type Information Resource HOB after successful allocation. This is used
                      for PEI Core to report the bins to DXE Core. FALSE if HOB creation is not needed.
**/
VOID
EFIAPI
AllocateMemoryTypeInformationBins (
  BOOLEAN  CreateHob
  )
{
  UINTN                 Index;
  EFI_STATUS            Status;
  EFI_MEMORY_TYPE       Type;
  EFI_PHYSICAL_ADDRESS  BaseAddress;
  EFI_PHYSICAL_ADDRESS  LastBinAddress;
  UINTN                 RequiredSize;

  //
  // Check to see if the statistics for the different memory types have already been established
  //
  if (mMemoryTypeInformationInitialized) {
    return;
  }

  BaseAddress  = 0;
  RequiredSize = CalculateTotalMemoryBinSizeNeeded ();

  DEBUG ((DEBUG_ERROR, "%a: Attempting to allocate 0x%llx bytes for all memory bins\n", __func__, RequiredSize));

  // To ensure we get a contiguous range of memory for our bins, we will attempt to allocate
  // all of the memory needed in one go. If that works, we can then carve it up into the individual bins.
  Status = AllocateBinPages (
             EfiBootServicesData,
             RShiftU64 (RequiredSize, EFI_PAGE_SHIFT),
             &BaseAddress
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Could not allocate contiguous pages for all memory bins\n", __func__));
    ASSERT (FALSE);
    return;
  }

  LastBinAddress         = BaseAddress + RequiredSize;
  mDefaultMaximumAddress = BaseAddress - 1;

  //
  // Loop through each memory type in the order specified by the gMemoryTypeInformation[] array
  //
  for (Index = 0; gMemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
    //
    // Make sure the memory type in the gMemoryTypeInformation[] array is valid
    //
    Type = (EFI_MEMORY_TYPE)(gMemoryTypeInformation[Index].Type);
    if ((UINT32)Type > EfiMaxMemoryType) {
      continue;
    }

    if (gMemoryTypeInformation[Index].NumberOfPages != 0) {
      mMemoryTypeStatistics[Type].BaseAddress    = LastBinAddress - LShiftU64 (gMemoryTypeInformation[Index].NumberOfPages, EFI_PAGE_SHIFT);
      mMemoryTypeStatistics[Type].MaximumAddress = LastBinAddress - 1;
      LastBinAddress                             = mMemoryTypeStatistics[Type].BaseAddress;
      DEBUG ((DEBUG_ERROR, "OSDDEBUG5 %a: Memory Type %d assigned bin 0x%llx - 0x%llx\n", __func__, Type, mMemoryTypeStatistics[Type].BaseAddress, mMemoryTypeStatistics[Type].MaximumAddress));
    }
  }

  //
  // There was enough system memory for all the the memory types were allocated.  So,
  // those memory areas can be freed for future allocations, and all future memory
  // allocations can occur within their respective bins
  //
  FreeBinPages (
    BaseAddress,
    RShiftU64 (RequiredSize, EFI_PAGE_SHIFT)
    );
  for (Index = 0; gMemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
    //
    // Make sure the memory type in the gMemoryTypeInformation[] array is valid
    //
    Type = (EFI_MEMORY_TYPE)(gMemoryTypeInformation[Index].Type);
    if ((UINT32)Type > EfiMaxMemoryType) {
      continue;
    }

    if (gMemoryTypeInformation[Index].NumberOfPages != 0) {
      mMemoryTypeStatistics[Type].NumberOfPages   = gMemoryTypeInformation[Index].NumberOfPages;
      gMemoryTypeInformation[Index].NumberOfPages = 0;
    }
  }

  //
  // If the number of pages reserved for a memory type is 0, then all allocations for that type
  // should be in the default range.
  //
  for (Type = (EFI_MEMORY_TYPE)0; Type < EfiMaxMemoryType; Type++) {
    for (Index = 0; gMemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
      if (Type == (EFI_MEMORY_TYPE)gMemoryTypeInformation[Index].Type) {
        mMemoryTypeStatistics[Type].InformationIndex = Index;
      }
    }

    mMemoryTypeStatistics[Type].CurrentNumberOfPages = 0;
    if (mMemoryTypeStatistics[Type].MaximumAddress == MAX_ALLOC_ADDRESS) {
      mMemoryTypeStatistics[Type].MaximumAddress = mDefaultMaximumAddress;
      mMemoryTypeStatistics[Type].DefaultBin     = TRUE;
    }
  }

  if (CreateHob) {
    DEBUG ((DEBUG_ERROR, "OSDDEBUG %a: Creating Memory Type Information Resource HOB at 0x%llx for length 0x%llx\n", __func__, BaseAddress, RequiredSize));
    //
    // Create a Resource Descriptor HOB to report the Memory Type Information bins to DXE Core
    //
    BuildResourceDescriptorWithOwnerHob (
      EFI_RESOURCE_SYSTEM_MEMORY,
      TESTED_MEMORY_ATTRIBUTES,
      BaseAddress,
      RequiredSize,
      &gEfiMemoryTypeInformationGuid
      );
  }

  mMemoryTypeInformationInitialized = TRUE;
}

/**
  Update memory type statistics upon memory allocation and free.

  @param OldType          The original memory type of the memory region.
  @param NewType          The new memory type of the memory region.
  @param Start            The starting physical address of the memory region.
  @param NumberOfPages    The number of pages in the memory region.
**/
VOID
EFIAPI
UpdateMemoryStatistics (
  IN EFI_MEMORY_TYPE       OldType,
  IN EFI_MEMORY_TYPE       NewType,
  IN EFI_PHYSICAL_ADDRESS  Start,
  IN UINTN                 NumberOfPages
  )
{
  //
  // Update counters for the number of pages allocated to each memory type
  //
  if (OldType < EfiMaxMemoryType) {
    if (NumberOfPages > mMemoryTypeStatistics[OldType].CurrentNumberOfPages) {
      mMemoryTypeStatistics[OldType].CurrentNumberOfPages = 0;
    } else {
      mMemoryTypeStatistics[OldType].CurrentNumberOfPages -= NumberOfPages;
    }
  }

  // DEBUG ((DEBUG_ERROR, "OSDDEBUG201 Update memory statistics for allocated memory: OldType=%d, NewType=%d, Start=%llx, Pages=%llx, mDefaultBaseAddress=%llx, mDefaultMaximumAddress=%llx\n", OldType, NewType, Start, NumberOfPages, mDefaultBaseAddress, mDefaultMaximumAddress));
  if (NewType < EfiMaxMemoryType) {
    mMemoryTypeStatistics[NewType].CurrentNumberOfPages += NumberOfPages;
    if (mMemoryTypeStatistics[NewType].CurrentNumberOfPages > gMemoryTypeInformation[mMemoryTypeStatistics[NewType].InformationIndex].NumberOfPages) {
      gMemoryTypeInformation[mMemoryTypeStatistics[NewType].InformationIndex].NumberOfPages = (UINT32)mMemoryTypeStatistics[NewType].CurrentNumberOfPages;
    }
  }
}
