# Public release boundary

Distribute the audited Windows x64, Linux x64, macOS x64 and macOS arm64
setup ZIPs and their SHA-256 checksums on the same release page.
The setup host is built with PSXRECOMP_FORCE_SETUP_HOST=ON: it contains neither
compiled game functions nor compiled BIOS backends. OpenBIOS is included as
open-source firmware and regenerated locally. Emitters and required modified
framework/UI sources are bundled. Players supply the disc and build locally.

Never upload build-win64, build-release, generated, disc, prepared_disc, bios
retail references, memory cards, captures, runtime dumps or analysis databases.
GitHub automatic source archives are not the setup kit. Public repository
publication also requires preserving framework changes rather than pointing
only at unmodified submodule revisions.

Windows 10/11 x86-64 gameplay is tested. Linux/macOS kits retain their
explicit gameplay-unverified status approved by the owner. The publication destination is NJH-1001/Wipeout2K97XL. Release upload follows
package verification. Framework changes are preserved in patches/psxrecomp.patch
against the exact gitlink revision; the setup kit includes them already applied.

Metadata-only release refreshes may update documentation and declarative mod
catalog labels while retaining every other archive entry and permission exactly.
Record original/new hashes and the exact allowlist in a verification manifest.
Never substitute a newly compiled runtime under an old verification claim.

The Steam Deck AppImage is also an allowed setup distribution. It embeds the
audited Linux kit and portable Python, never owner-generated code or assets.
Runtime payload identities and content checks are recorded in the AppImage
verification manifest; generated files stay in the user data directory.
Steam Deck, Gaming Mode and Steam Controller (2026) physical input remain unverified.
