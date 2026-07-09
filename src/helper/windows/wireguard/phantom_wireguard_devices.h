#pragma once

namespace PhantomWireGuardDevices
{

// Windows feature updates re-register drivers from the driver store as ROOT\WireGuard devnodes, which
// wireguard-nt (SWD\WireGuard) never accounts for; they keep the old wireguard.sys loaded at every boot and
// permanently block the in-service driver upgrade ("Waiting for existing driver to unload from kernel").
// Removes both PnP-registered and registry-only ROOT\WireGuard device nodes.
//
// Ported from wireguard-nt's CleanupOrphanedDevices (upstream commit 523d33f1, "api: cleanup phantom
// devices added by windows updates"), which the wireguard-nt build we bundle predates. Real adapters only
// ever live under SWD\WireGuard, so every ROOT\WireGuard node is bogus by definition: removal is
// deliberately unconditional (present devnodes included), and the raw RegDeleteTreeW fallback needs no
// guard on why the PnP path failed. Removal also never requires a reboot: a bogus ROOT node has no started
// device stack to veto or defer DIF_REMOVE, which is why upstream has no reboot handling anywhere.
// The one intentional deviation from upstream is snapshotting the
// registry key names before deleting, so a removal that leaves its key behind cannot stall the enumeration
// (upstream re-reads the same index forever in that case; in the helper that would hang all IPC).
void remove();

}
