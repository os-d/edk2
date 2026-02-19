//------------------------------------------------------------------------------
// AArch64/StackCookieInterrupt.S
//
// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: BSD-2-Clause-Patent
//------------------------------------------------------------------------------

    .text

//------------------------------------------------------------------------------
// Calls an interrupt using the vector specified by PcdStackCookieExceptionVector
// and passes the ExceptionAddress through in x0.
//
// VOID
// EFIAPI
// TriggerStackCookieInterrupt (
//   EFI_PHYSICAL_ADDRESS ExceptionAddress
//   );
//------------------------------------------------------------------------------
.global ASM_PFX(TriggerStackCookieInterrupt)
ASM_PFX(TriggerStackCookieInterrupt):
    svc FixedPcdGet8 (PcdStackCookieExceptionVector)
    ret
