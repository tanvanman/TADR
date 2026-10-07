# Retail map-selection preview fix

TA's menu preview accidentally uses a transparent GAF copy for an opaque TNT
minimap. Its frame allocator leaves the transparency-key byte uninitialized.
Pixels matching that accidental index are skipped in an uncleared temporary
allocation; the original scaler then displays the old contents as map pixels.
Allocation history and the minimap's palette-index distribution explain why
the corruption varies between selections.

The fix replaces one temporary-copy CALL with an opaque row copy. It preserves
the stock selector, crop/scaler, black bars, palette, retained-panel rendering
and redraw path. It introduces no extra image allocation or image-copy stage.

## Executable and evidence

The investigated retail 3.1 `TotalA.exe` has SHA-256
`3b9c0fadabf3dc67ed5f05a70f1e1505a0c65deadd1a3c930adfe30e2a84995e`.
The affected installation's executable matched this hash. Analysis used the
available TA Research disassembly, cached decompilation and reconstructed
function cross-references; raw executable bytes and native replay resolve
decompiler omissions. Other executable variants require separate verification.

[Byte comparison evidence](MAP_PREVIEW_BYTE_EVIDENCE.json) records 25 selected
functions, 2,630 instruction starts and 8,044 bytes matching the hashed image,
plus the interception's 15-byte argument-setup/CALL context.

## Loading and rendering path

| Stage | Retail address and behavior |
| --- | --- |
| Skirmish selector | `0047aaf0` opens; `0047aaa0` handles selection |
| Multiplayer selector | `00444ea0` opens; `00444c40` handles selection |
| Selected map | `00435a20 -> 00435da0` resolves the OTA record |
| Preview builder | `00444a20` finds `MAPPIC`, frees/nulls its old frame, obtains the TNT path, loads/scales the new frame, then marks the GUI dirty |
| TNT path | `004356c0(mapObject, 1)` returns the map's TNT pathname |
| Minimap reader | `004295b0` reads a 64-byte TNT header, seeks to the minimap offset at header +28h, reads stored width/height and tightly packed 8-bit palette indices |
| Frame allocation | `004b8da0` allocates `24 + width*height` bytes; pixels begin at +18h and their pointer is at +10h; width/height are unsigned words at +00/+02 |
| Preview scaler | `004665d0` allocates a same-size temporary frame, copies the minimap, rewrites the original frame's dimensions, clears it to index zero, and crops/fits the temporary image through the stock quad/span mapper |
| Faulty copy | CALL `004666c0 -> 004b7f90 -> 004cbe70` treats source frame byte +08h as a transparency key |
| Surface gadget | `004a4980` draws gadget frame +c2h into the retained panel using the stock mapping/edge conventions |
| Redraw | `0049fa90` marks dirty; `004a9fd0` / `004a81e0` redraw the page; `004ab170 -> 004ab0b0` composites it |
| Presentation | `004c5e70` / `004c5fa0` manage output acquisition/release; `004c63a0` presents through the stock GDI/DirectDraw path |

The minimap is embedded in the TNT, rather than generated from rendered terrain.
Its bytes are global palette indices: every value 0..255 can be legitimate.
The scaler uses playable map dimensions (world width minus 32 and height minus
128) for the original aspect/crop calculation. The destination really is cleared;
the defect is the earlier incomplete copy into the temporary frame.

The allocator initializes compression and compound-frame state, but does not
initialize byte +08h or pixel memory. The keyed blitter always skips pixels equal
to byte +08h. Clearing the temporary allocation or choosing another fixed key
would still discard legitimate map colours. Copying opaquely preserves all of them.
There is no need for palette loss, stale preview ownership or a redraw race to
reproduce this particular failure.

## Patch contract and settings

[TABugFix.cpp](src/DDraw/TABugFix.cpp) redirects only the five-byte CALL at
`004666c0`, after verifying its original 15-byte setup/CALL context. Four arguments
and `__stdcall` match the original function's `ret 10h`. No direct CALL/JUMP
targets into those replaced bytes were found in the supplied disassembly.

The shim requires a zero-origin, uncompressed leaf frame, matching positive
dimensions and a full-clip destination. The
[copy helper](src/DDraw/MapPreviewCopy.h) copies whole rows with `memcpy`, using
actual destination pitch and tightly packed source width. Unexpected contracts
log and use the original function. Existing `SingleHook` ownership saves/restores
the original bytes; the patch introduces no allocation/free or cache ownership.

In the active patch INI's `[Preferences]` section:

```ini
MapSelectionPreviewFix = TRUE;
MapSelectionPreviewDiagnostics = FALSE;
```

These are also the defaults. Restart after changing either setting. Both FALSE
leave the original call untouched. Fix FALSE/diagnostics TRUE observes the stock
copy. Diagnostics log `[MapPreview]` installation/fallback state, accidental key,
matching-key pixel count, copy mismatches and input/output hashes. Fixed copies
have zero mismatches and equal logical-pixel hashes. The feature is independent
of the battle-radar `EnhancedBuiltInMinimap` setting.

## Validation and limits

- `ReleasePublic|Win32`, VS2022/v143, default ProTA profile: build passed.
- The user reports successful live testing of the reported preview issue.
- The [Win32 regression](src/DDraw/test/map_preview_copy_test.cpp) requires the
  exact executable hash before executing any supplied routine. It starts no game
  and includes no TA executable or assets. Controlled heap/context substitutions
  let it replay the original allocator, keyed blitter and scaler software closure.
- All 256 accidental keys reproduce the skipped-pixel defect; the shared copy
  helper preserves every index. Five dimensions, odd widths, padded pitch,
  surrounding/per-row canaries and invalid-copy refusal cases pass.
- Stock scaler replay reproduces 765 corrupt aspect/key combinations. All 768
  fixed square/wide/tall results match undamaged stock output byte for byte,
  preserving crop, centering, bars and sampling quirks.

Run from the checkout in PowerShell, with VS2022 Community installed:

```powershell
& '.\src\DDraw\test\run_map_preview_test.cmd' 'PATH\TO\PRIVATE\TotalA.exe'
```

The laboratory test uses a test shim around the shared production copy helper;
it does not exercise the DLL's installation, fallback or diagnostic logging.
The successful live report did not specify the full selection sequence or cover
all multiplayer/reopen/alt-tab cases. Source review found no blocking issue;
no approval from other maintainers or AI reviewers is implied.

Separate stock limits remain: the original in-place resize can exceed its
allocation for unusually small rasters/large canvases; short reads/allocation
failures and legacy minimap-header handling are not hardened here. They are not
demonstrated causes of the reported frequent corruption. The source's historical
`Gaf.h` Background offset comment is inaccurate: the actual field is +08h;
implementation static assertions check the relevant frame/surface layout.
