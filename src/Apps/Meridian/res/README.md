The icon is generated locally from the ten resource payloads of the pinned PC
`Meridian.exe` (SHA-256 `631a05531e382556cab3544d228b72171eff089b81696269b4d89963bced5fa2`).
Its nine DIB payloads are unchanged. The ICO container is reconstructed from
the named `IDC_ICON1` group under ignored `build/gen/rsrc/res/meridian.ico`.
No original icon payload is distributed in the source tree or source export.
`hobbit rsrc check` verifies the full executable size and SHA-256 before
extracting the icon, then compiles the staged resource script
and compares every type, name, language, payload byte, and payload order.
The original resource-script path is not established; this source is housed
with the application, without asserting original translation-unit ownership.

The local extraction mechanism adapts HoMM1's `scripts/homm1/tool/rc.py`
(`icon_container`) and `scripts/homm1/clean/template/build.py` (`icon_group`)
at revision `8d3ae6c96fc9b06d9c709fbdbfa181d78997b787`. Hobbit-specific group,
language and image-count checks come from the pinned Hobbit image.
