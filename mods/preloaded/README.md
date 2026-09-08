# Preloaded mods

Ship reviewed, default-disabled packages here:

```text
packages/<package-id>/<version>/
  manifest.toml
  ...
```

Build wiring copies `mods/preloaded/packages` next to the game executable as
`mods/bundled/`. That tree is build output: every build wipes and re-stages it,
so nothing you place there by hand survives.

Player-installed `.psxmod` archives live in `mods/installed/`, which the
launcher owns and no build ever touches. Install them through the launcher Mods
manager rather than committing them here.

See `psxrecomp/docs/MOD_PACKAGES.md`.
