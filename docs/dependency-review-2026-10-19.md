# Dependency and release review — 2026-10-19

Extended Draw Distance and Extended Enemy Ship Distance are now regular optional
features, following the owner's completed race testing. Their identifiers,
settings, default-off state and behavior are unchanged. All four platform kits
are available on the [main release page](https://github.com/NJH-1001/Wipeout2K97XL/releases/tag/v0.1.0).
Linux/macOS retain their gameplay-unverified status.

## Runtime dependencies

| Component | Shipped version | Reviewed current upstream | Decision |
| --- | --- | --- | --- |
| PSXRecomp | `8d56cf659fd84367c3f778e17851674ac7896371` plus `patches/psxrecomp.patch` | [`61e18aeea7892eab831ab5295bf762b4013bdfd3`](https://github.com/mstan/psxrecomp/commit/61e18aeea7892eab831ab5295bf762b4013bdfd3) | Retain tested integration; migrate and revalidate before upgrading |
| recomp-ui | `773155ae7d3be80a21d40851b58f99c79e003de1` | [`ef13e3f1a00e2e03f91e4b74343fd77213f8a234`](https://github.com/mstan/recomp-ui/commit/ef13e3f1a00e2e03f91e4b74343fd77213f8a234) | Retain paired UI until framework migration |
| SDL3 | Framework source fallback pins 3.4.10; original Windows verification used 3.4.10 | [3.4.18](https://github.com/libsdl-org/SDL/releases/tag/release-3.4.18) | Existing release binaries retained; upgrade requires rebuild and controller/rendering regression |
| Portable toolchain | Wizard obtains platform-specific `cmake-clang-v1` pack | [v1.0.14](https://github.com/TechnicallyComputers/retcomm-toolchains/releases/tag/v1.0.14) | Current published pack supports Windows x64, Linux x64 and macOS universal |

The reviewed PSXRecomp head is 1,529 commits and 1,360 changed files beyond our
pin. The recomp-ui head is 242 commits and 126 changed files beyond its pin.
These are observed differences at the exact reviewed commits, not a statement
about future upstream versions.

## Compatibility findings

`git apply --check patches/psxrecomp.patch` against a clean archive of the reviewed
PSXRecomp head fails in the BIOS configuration, code generator, runtime, GPU/SIO,
mod headers, tests and packaging script. The latest mod header also lacks
`PSXModNegconInput`, `PSXModNegconOutput` and `psx_mod_set_negcon_mapping`, which
our configurable dual-stick plugin consumes. Replacing the gitlink alone would
leave this title unbuildable.

The launcher header and integration module also changed substantially. A paired
framework/UI migration must review the host ABI and settings integration, rather
than assuming that a UI pin update independently preserves title behavior.

Before shipping upgraded runtime dependencies:

1. Reconcile the title patch with upstream class-level fixes and preserve the
   verified mod/input callback contracts.
2. Apply the resulting patch to clean pinned sources and compile the emitters,
   setup host and locally generated game.
3. Verify OpenBIOS boot and a race, default and customized NeGcon controls,
   scenery/ship/exhaust distance, widescreen effects and transitions.
4. Repeat native platform build, dependency and launcher checks; retain explicit
   gameplay limits until those systems receive full race verification.

No runtime dependency upgrade is claimed in this metadata maintenance release.
The previously verified binaries remain byte-identical.

## Build tooling

The release workflows now pin these official current action releases by commit:

- [checkout v7.0.1](https://github.com/actions/checkout/releases/tag/v7.0.1): `3d3c42e5aac5ba805825da76410c181273ba90b1`.
- [setup-python v7.0.0](https://github.com/actions/setup-python/releases/tag/v7.0.0): `5fda3b95a4ea91299a34e894583c3862153e4b97`.
- [upload-artifact v7.0.2](https://github.com/actions/upload-artifact/releases/tag/v7.0.2): `cf430e030ddbb5b0abf93d22962f4752f3646cd9`.

Build workflow verification is recorded in the maintenance manifest. Newly built
CI artifacts do not replace the gameplay-tested release binaries automatically.

## Package maintenance

All four archives receive the same current instructions, final owner-supplied
controller diagram, dependency review and regular draw-distance catalog labels.
The Windows archive previously lacked the final diagram and native instructions.
Both the runtime catalog and the catalog used by local rebuilds are updated.
Every other archive entry and its executable permissions are checked against the
previously published ZIP; each replacement receives a new SHA-256 checksum.
