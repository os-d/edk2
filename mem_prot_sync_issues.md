# Memory Protections Synchronization Issues

This addresses github issue [#14](https://github.com/tianocore/projects/issues/14) to document the synchronization
issues with the existing edk2 memory protection implementation.

## Introduction

Today's edk2 memory protection code is multilayered with a large attack surface, inconsistent internal tracking, and differences
in implementations across architectures. A large number of bugs have come from the current architecture such as:

- [Bugzilla 753](https://bugzilla.tianocore.org/show_bug.cgi?id=753)
- [[PATCH v1 1/1] ArmPkg: CpuDxe: Report AARCH64 Memory Protections Attributes To GCD](https://edk2.groups.io/g/devel/topic/99095992#105872)
- [[PATCH v1 1/1] ArmPkg: CpuDxe: Sync GCD Capabilities With Page Table Attributes](https://edk2.groups.io/g/devel/topic/98505340).

Other issues included all DMA memory getting marked as executable by default regardless of memory protection settings.

This document contributes to an ongoing Tianocore workstream to offer better and more consistent memory protections in
edk2. We hope to enumerate the multitude of issues seen with the current design, so that as we shape the future design,
we can reflect on how it addresses each of these issues.

Currently, this document is scoped to the DXE boot environment which typically has the largest attack surface and
greatest opportunity for downstream consumers to accidentally break core expectations. Some of this applies to other
phases, while they also have their own unique issues. Note, many of the links in this document point to x86 versions of
CpuDxe, but other architectures have their own CpuDxe implementations with similar (and different!) approaches to handling memory
protections.

## Lack of Synchronization Between GCD and Page Table

The Global Coherency Domain (GCD) and the page table are not kept synchronized consistently. Some drivers may
[query/update the GCD](https://github.com/tianocore/edk2/blob/ff3382a51ca726a90f49623a2b2d2e8ad8459ce2/MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciHostBridge.c#L551-L555),
some may
[query/update the page table](https://github.com/tianocore/edk2/blob/ff3382a51ca726a90f49623a2b2d2e8ad8459ce2/OvmfPkg/Fdt/HighMemDxe/HighMemDxe.c#L157),
but synchronized updates are not guaranteed.

Typically, updates to the GCD update the page table, except during the
[window of time when CpuDxe is not yet available](https://github.com/tianocore/edk2/blob/ff3382a51ca726a90f49623a2b2d2e8ad8459ce2/MdeModulePkg/Core/Dxe/Gcd/Gcd.c#L912C3-L921).
Conversely, updates to the page table
([via CpuDxe](https://github.com/tianocore/edk2/blob/ff3382a51ca726a90f49623a2b2d2e8ad8459ce2/UefiCpuPkg/CpuDxe/CpuDxe.c#L371))
do not update the GCD, except during the initialization of CpuDxe, when some of the information is
[synchronized back to the GCD](https://github.com/tianocore/edk2/blob/ff3382a51ca726a90f49623a2b2d2e8ad8459ce2/UefiCpuPkg/CpuDxe/CpuDxe.c#L921).
However, even in this limited synchronization, many errors have been discovered leading to fragmentation between architectures.

For example, [a fix](https://bugzilla.tianocore.org/show_bug.cgi?id=753) was made to ensure non-execute (NX),
read-only (RO), and read-protect (RP) capabilities are added to the GCD from the page table for x86, but AARCH64 was
left broken and would silently drop any NX, RO, or RP bits that were actually set in the page table. As a result,
drivers were unaware they were allocating NX, RO, or RP memory and faulted as a result.

## Memory Attributes Protocol and CpuDxe Memory Attributes Protocol Skip GCD

This issue is related to the last but deserves highlighting. The GCD, which is nominally the source of truth for memory
attributes in DXE (although as we will see it is one source of truth among many) can be bypassed by drivers using the
[memory attributes protocol](https://github.com/tianocore/edk2/blob/ff3382a51ca726a90f49623a2b2d2e8ad8459ce2/ArmPkg/Drivers/CpuDxe/MemoryAttribute.c#L179)
or [CpuDxe memory attribute protocol](https://github.com/tianocore/edk2/blob/ff3382a51ca726a90f49623a2b2d2e8ad8459ce2/UefiCpuPkg/CpuDxe/CpuDxe.c#L371),
which will directly update the page table, but the GCD will never have knowledge of this. This can lead to issues such
as DXE believing a block of memory is safe to execute or write or read, but in fact the page table has been updated and
so a fault will occur upon the now invalid but untracked access.

## Memory Attributes vs Memory Capabilities

Currently, the PI and UEFI specs are vague in defining the difference between memory attributes and capabilities. In the
GCD this distinction is clear because its descriptors have fields for both attributes and capabilities. However, in
the EFI Memory Map and Memory Attributes Table (MAT) there is only a single field labeled Attributes. The specs in some
places indicate this field should be treated as capabilities and not physically set attributes in the page table while
other places are vague on what the field represents. This has led to issues such as both
recent (at the time) versions of Windows and Linux failing to boot because they treated this Attributes field as
reflective of the page table and, seeing all memory as NX, were unable to execute code.
See this [bugzilla](https://bugzilla.tianocore.org/show_bug.cgi?id=753) for more details on this example.

There are other instances in the codebase where the concept of attributes and capabilities are intertwined such as
resource descriptor HOBs whose descriptors have a
[field for memory attributes](https://github.com/tianocore/edk2/blob/ff3382a51ca726a90f49623a2b2d2e8ad8459ce2/MdePkg/Include/Pi/PiHob.h#L330)
which contains both attributes and capabilities.

## The EFI Memory Map Has No Memory Protection Attributes/Capabilities Set

Due to the OS boot bug listed in the above section, a decision was made to
[scrub the EFI Memory Map of all memory protection attributes](https://github.com/tianocore/edk2/blob/ff3382a51ca726a90f49623a2b2d2e8ad8459ce2/MdeModulePkg/Core/Dxe/Mem/Page.c#L2015-L2030).
This leads bootloaders and early OS execution environments to believe that no memory is capable of being NX, RO, or RP, which can
cause periods of insecure memory before the kernel takes full control of memory.

## Memory Attributes and Free Memory Are Insecure By Default

The current PI/UEFI spec defined memory attributes leave memory insecure by default. These definitions stem from memory
management unit settings, some of which are changing to be secure by default. EFI_MEMORY_NX, EFI_MEMORY_RO, and EFI_MEMORY_RP are EFI attributes that
[must be set](https://github.com/tianocore/edk2/blob/ff3382a51ca726a90f49623a2b2d2e8ad8459ce2/MdePkg/Include/Uefi/UefiSpec.h#L74C1-L77)
to enable the protection instead of leaving memory secure by default. This requires the allocator of memory to
apply protection attriubtes on memory containing code unless the core has applied some baseline protection (though this is currently
[limited to NX](https://github.com/tianocore/edk2/blob/ff3382a51ca726a90f49623a2b2d2e8ad8459ce2/MdeModulePkg/Core/Dxe/Misc/MemoryProtection.c#L661-L664),
if the system has been configured for it).

## Core Code Has Limited Control Of Memory Protections

Today, core code heavily defers to platforms to decide whether to enable memory protections or not. By default, memory protection is disabled
and it is up to a platform to set a variety of PCDs which update their resource descriptor HOBs to add capabilities for memory protections. The core has a
responsibility to downstream consumers and users of edk2 derived platforms to switch to a model where protection is enabled by default and the
platform must make an effort to reduce security instead of the other way around.

Even when the core has been configured to enable memory protections, they can be easily disabled by a non-core driver
or option ROM calling into the GCD or CpuDxe. This requires a user to have complete faith in every device and their
platform vendor. A different, more secure model would be to have the core enforce certain memory protections (after
going through a transition period for existing platforms to conform) and perhaps disallow memory attribute changes from
certain sources, like option ROMs.

## Multiple Sources of Truth

All of the above points boil down into one central issue: today's design suffers from no single source of truth for
memory attributes. In the end, the physical bits in the page/translation table affect how the underlying memory is configured but the
tracking of this information is inconsistent. In EDK2, there exists the GCD, page table, page descriptor list
(free page list), EFI Memory Map, and MAT all carrying some amount of unsynchronized state information.

A single source of truth enables a consistent interface for drivers to query and a more maintainable memory attribute system with a reduced
chance of inconsistent behavior across architectures.
