/** @file
  This file declares the Memory Type Information HOB Available PPI. When this HOB is available,
  the PEI Core is notified so that it can create the memory bins for the various memory types.
  This has no associated structure and just is intended as a notification mechanism.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MEMORY_TYPE_INFORMATION_HOB_AVAILABLE_H_
#define MEMORY_TYPE_INFORMATION_HOB_AVAILABLE_H_

#define MEMORY_TYPE_INFORMATION_HOB_AVAILABLE_PPI_GUID \
  { \
    0x24fd4e27, 0x6f4b, 0x4460, {0xb7, 0xa7, 0x21, 0x8b, 0xe8, 0xc3, 0x50, 0xc7} \
  }

extern EFI_GUID  gMemoryTypeInformationHobAvailablePpiGuid;

#endif // MEMORY_TYPE_INFORMATION_HOB_AVAILABLE_H_
