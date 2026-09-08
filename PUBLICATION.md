# Public release boundary

Distribute the audited Windows x64 setup ZIP and its SHA-256 checksum only.
The setup host is built with PSXRECOMP_FORCE_SETUP_HOST=ON: it contains neither
compiled game functions nor compiled BIOS backends. OpenBIOS is included as
open-source firmware and regenerated locally. Emitters and required modified
framework/UI sources are bundled. Players supply the disc and build locally.

Never upload build-win64, build-release, generated, disc, prepared_disc, bios
retail references, memory cards, captures, runtime dumps or analysis databases.
GitHub automatic source archives are not the setup kit. Public repository
publication also requires preserving framework changes rather than pointing
only at unmodified submodule revisions.

The initial scope is Windows 10/11 x86-64. The publication destination is NJH-1001/Wipeout2K97XL. Release upload follows
package verification. Framework changes are preserved in patches/psxrecomp.patch
against the exact gitlink revision; the setup kit includes them already applied.
