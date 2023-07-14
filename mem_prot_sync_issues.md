# Memory Protections Synchronization Issues

This addresses github issue [#14](https://github.com/tianocore/projects/issues/14) to document the synchronization
issues with the existing edk2 memory protection implementation.

## Introduction

Today's edk2 memory protection code is multilayered with a large attack surface, inconsistent internal tracking, and
large differences in implementations between architectures. A large number of bugs have come from the current
architecture, including but certainly not limited to:

- [Bugzilla 753](https://bugzilla.tianocore.org/show_bug.cgi?id=753)
- [[PATCH v1 1/1] ArmPkg: CpuDxe: Report AARCH64 Memory Protections Attributes To GCD](https://edk2.groups.io/g/devel/topic/99095992#105872)
- [[PATCH v1 1/1] ArmPkg: CpuDxe: Sync GCD Capabilities With Page Table Attributes](https://edk2.groups.io/g/devel/topic/98505340).

Other issues included all DMA memory getting marked as executable by default regardless of memory protection settings.

This document contributes to an ongoing Tianocore workstream to offer better and more consistent memory protections in
edk2. We hope to enumerate the multitude of issues seen with the current design, so that as we shape the future design,
we can reflect on how it addresses each of these issues.

Currently, this document is scoped to the DXE boot environment which typically has the largest attack surface and
greatest opportunity for downstream consumers to accidentally break core expectations. Some of this applies to other
phases, while they also have their own unique issues.

## Lack of Synchronization Between GCD and Page Table

The Global Coherency Domain (GCD) and the page table are not kept synchronized consistently. Some drivers may
query/update the GCD, some may query/update the page table, but synchronized updates are not guaranteed.

Typically, updates to the GCD update the page table, except during the window of time when CpuDxe is not available yet.
Conversely, updates to the page table (via CpuDxe) do not update the GCD, except during the initialization of CpuDxe,
when some of the information is synchronized back to the GCD. However, even in this limited synchronization, many
errors have been seen, especially as CpuDxe is different between architectures. This leads to issue fragmentation across
architectures.

For example, a fix was made to ensure non-execute (NX), read-only (RO), and read-protect (RP) capabilities are added to
the GCD from the page table for x86, but AARCH64 was left broken and would silently drop any NX, RO, or RP bits that
were actually set in the page table. As a result, drivers were unaware they were allocating NX, RO, or RP memory and
faulted as a result.

## Memory Attributes Protocol and CpuDxe Memory Attributes Protocol Skip GCD

This issue is related to the last but deserves highlighting. The GCD, which is nominally the source of truth for memory
attributes in DXE (although as we will see it is one source of truth among many) can be bypassed by drivers using the
memory attributes protocol or CpuDxe memory attribute protocol, which will directly update the page table, but the GCD
will never have knowledge of this. This can lead to issues such as DXE believing a block of memory is safe to execute or
write or read, but in fact the page table has been updated and so a fault will occur upon the now invalid but untracked
access.

## Memory Attributes vs Memory Capabilities

Currently, the PI and UEFI specs are vague in defining the difference between memory attributes and capabilities. In the
GCD, this distinction is clear, because its descriptors have fields for both attributes and capabilities. However, in
the EFI Memory Map and Memory Attributes Table (MAT) there is only a single field labeled Attributes. The specs in some
places indicate this is actually a capabilities field, not physically set attributes in the page table, in other places
they are vague as to whether this indicates real attributes or merely capabilities. This has led to issues such as both
recent (at the time) versions of Windows and Linux failing to boot because they treated this Attributes field as
physically set Attributes in the page table to be respected, seeing all memory as NX, and being unable to execute code.

There are other instances in the codebase where the concept of attributes and capabilities get intertwined, such as in
resource descriptor HOBs, whose descriptors have a field for memory attributes which contains both attributes and
capabilities.

## EFI Memory Map Has No Memory Protection Attributes/Capabilities Set

Due to the OS boot bug listed in the section above, a decision was made to scrub the EFI Memory Map of all memory
protection attributes. This leads bootloaders and early parts of OSes to believe that no memory is capable of being NX,
RO, or RP, which can lead to periods of insecure memory before a kernel takes full control of memory.

## Memory Attributes and Free Memory Are Insecure By Default

The current PI/UEFI spec defined memory attributes leave memory insecure by default. These definitions stem from memory
management unit settings, some of which are changing to be secure by default. NX, RO, and RP are bits that must be set
to enable the protection, instead of leaving memory secure by default (e.g. if the NX bit meant 0 as NX). This leads to
the possibility of free memory getting attacker controlled payloads written to it and potentially executed from it. It
also means that every allocator of memory is relied upon to set memory protections upon code unless the core has applied
some baseline protection on it (though this is currently limited to NX, if the system has been configured for it).

## Core Code Has Limited Control Of Memory Protections

Today, core code heavily defers to platforms to decide whether to enable memory protections or not. By default, all
memory protection is disabled, and it is up to a platform to set a variety of PCDs and update their resource descriptor
HOBs to add capabilities for memory protections. This leads to the state we are in today, where platforms by and large
prioritize shipping products to security, and so leave off memory protections on production platforms. The core has a
responsibility to downstream consumers and users of edk2 derived platforms to enable security by default instead of
deferring this decision to implementors.

Even when the core has been configured to enable memory protections, they can be easily disabled by a non-core driver
or option ROM calling into the GCD or CpuDxe. This requires a user to have complete faith in every device and their
platform vendor. A different, more secure model would be to have the core enforce certain memory protections (after
going through a transition period for existing platforms to conform) and perhaps disallow memory attribute changes from
certain sources, like option ROMs.

## Multiple Sources of Truth

All of the above points boil down into one central issue: today's design suffers from no single source of truth for
memory attributes; of course, in the end, the physical bits in the MMU affect how the underlying memory is configured.
However, in edk2 our tracking of this information is inconsistent, we have the GCD, page table, the page descriptor list
(free page list), EFI Memory Map, and MAT all carrying some amount of state that needs to be synchronized and today is
often not.

The lack of a source of truth and the ability of all of these components to be independently updated, sometimes syncing
with each other, sometimes not, causes a torn state in the system on every single boot. Because drivers can choose
whether to query/update the GCD (which in turn updates the page table, usually) or to directly update the page table
through the memory attributes protocol or CpuDxe's memory attribute protocol (both of which do not in turn update the
GCD) we will always end up in this torn state. This leads to hard to debug issues such as the non-discoverable PCI
driver saving information from the GCD that did not accurately reflect the state of the system, changing memory
attributes, and upon freeing the memory, attempting to restore the old GCD attributes, which did end up syncing with the
page table and thus causing an access fault down the road.

A single source of truth allows a consistent interface for every core, platform, and silicon driver to query/update
through, naturally eliminating the synchronization issue. Furthermore, it allows for a more maintainable and extensible
memory attribute system, as only a single set of APIs need to be updated, as opposed to chasing down many, many odd and
often incorrect uses in core and non-core code (which of course edk2 cannot influence how platforms choose to interact
with it except by defining sane and maintainable APIs).
