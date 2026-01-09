/** @file
  Shared logic between cores to work with memory bins for S4 resume stability.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef BASE_MEMORY_BIN_LIB_
#define BASE_MEMORY_BIN_LIB_

#include <Guid/MemoryTypeInformation.h>

//
// Entry for tracking the memory regions for each memory type to coalesce similar memory types
//
typedef struct {
  EFI_PHYSICAL_ADDRESS    BaseAddress;
  EFI_PHYSICAL_ADDRESS    MaximumAddress;
  UINT64                  CurrentNumberOfPages;
  UINT64                  NumberOfPages;
  UINTN                   InformationIndex;
  BOOLEAN                 Special;
  BOOLEAN                 Runtime;
  BOOLEAN                 DefaultBin;
} EFI_MEMORY_TYPE_STATISTICS;

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
  );

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
  );

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
  );

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
  );

extern EFI_MEMORY_TYPE_INFORMATION  gMemoryTypeInformation[EfiMaxMemoryType + 1];
extern EFI_MEMORY_TYPE_STATISTICS  mMemoryTypeStatistics[EfiMaxMemoryType + 1];
extern BOOLEAN                      mMemoryTypeInformationInitialized;
#endif // BASE_MEMORY_BIN_LIB_
