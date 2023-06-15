##
#    SPDX-License-Identifier: BSD-2-Clause-Patent
##

#
# Memory Protection Libraries
#
[LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.DXE_CORE, LibraryClasses.common.UEFI_APPLICATION, LibraryClasses.common.UEFI_DRIVER]
  DxeMemoryProtectionHobLib|MdeModulePkg/Library/MemoryProtectionHobLib/DxeMemoryProtectionHobLib.inf

[LibraryClasses.common.SMM_CORE, LibraryClasses.common.DXE_SMM_DRIVER]
  MmMemoryProtectionHobLib|MdeModulePkg/Library/MemoryProtectionHobLib/SmmMemoryProtectionHobLib.inf

[LibraryClasses.common.MM_CORE_STANDALONE, LibraryClasses.common.MM_STANDALONE]
  MmMemoryProtectionHobLib|MdeModulePkg/Library/MemoryProtectionHobLib/StandaloneMmMemoryProtectionHobLib.inf
