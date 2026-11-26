# Contributing to TADR

Pull requests are reviewed automatically, and each merged change that alters what players run gets its own release. A person can always be reached by mentioning @Axle1975.

## What happens to your pull request

1. About 24 hours after your last push, a review is posted. Keep amending until then; drafts are never reviewed.
2. If changes are needed, push them or reply. The next review follows about 24 hours after your last push or reply. Replying without pushing is fine if you disagree.
3. Once approved, it is merged and released. Some changes also wait for the maintainer: a first contribution, changes to workflows, build settings, scripts or binaries, new file, process, network or registry access, and changes to what is sent between players or recorded.

Changes under `src/Recorder` are reviewed but never merged automatically.

## Don't edit these

The release step writes `src/DDraw/tdraw.txt`, the version in `totala.ini` and the version in `ddraw.rc`. Edits to them are dropped when your change is merged. Put this in the pull request description instead:

- **Release note:** one line for players.
- **Detail:** only if players, modders or hosts need to know how to use something, such as a command, key, setting, unit or weapon tag, or GUI gadget.

## Gate it per mod

Anything that can change simulation results needs a switch, even a bug fix. So does every feature or assistance to the player. Display-only fixes, crash fixes that leave healthy games unchanged, and pure refactors don't. Nor does a change that only takes effect while a recording is played back or watched, as long as it does nothing in a live game, including one played later in the same process. Nor does an interface change that only offers another way to use the game's existing controls, issues no orders the player couldn't already issue the same way, and reveals nothing, such as mouse-wheel scrolling in a native list. Anything that issues orders for the player, automates what they would do by hand, or shows them information is assistance, and needs a switch.

- Define the switch explicitly, as 0 or 1, in **all seven** `config_*.h` files. Each mod's file lists what that mod enables.
- Don't add a header per feature that decides which mods get it, infer a default from `TDRAW_CONFIG_*`, give a new switch a fallback in `config.h`, or add an override through MSBuild properties or `-D`. `config.h` may check a switch with `#error`.
- Set a new switch to 0 for every mod you don't speak for, and leave other mods' existing values alone. If you maintain a mod and aren't recorded as speaking for it, say so in the pull request.
- Only a `config_*.h` switch counts. An ini key, menu option, environment variable or checkbox is not a gate, even if it defaults to off: each player controls their own, and a mod must be able to guarantee that a feature is off for everyone. You may add an ini key as well, so that a player can turn off for themselves what the build enables.

## Patching the engine

- Check the bytes before patching, and disable your feature with a log line if they don't match.
- Look for other patches nearby, including in the recorder sources, where a splice is 5 bytes plus its `extra` argument. Say in your description what you checked.
- Use the fields in `tamem.h` rather than your own offsets, wherever practical. If the field you need is hidden inside a generic `data` member, split it out in `tamem.h` with a real name and type, keep the layout, and add a `static_assert` on its offset.
- Nothing local may affect the simulation: settings, locale, wall-clock time, rendering, uninitialised memory or who the local player is.
- No Windows 8+ imports. `tools/check_win7_imports.py` runs in CI.
- Keep each file's line endings.

## Network and recordings

- The `.tad` format is frozen. Don't change packet or status record sizes, or what an existing packet means. If you use spare bytes of an existing packet, list every reader you checked; a maintainer has to agree to it.
- New traffic goes in a chat-hijack packet with an ID reserved in `ChatHijackIds.h` and a size byte the receiver checks. Clients and parsers that don't know it must be able to ignore it.
- Packet handlers never send.

## Mixed versions

Players will be on different releases. Your change must work in a game with the previous release and with older recorders, or be 0 for every mod. It must not need another program, such as gpgnet4ta, the recorder, the replayer or the TAF client, to be updated at the same moment.

## If you played it

This isn't required, but if you played your change in multiplayer or replayed a recording, say so in the description, and on which commit. It helps the mod developers, who test before they release.

## The configurations table

`tools/config_table.py` generates the table in `tdraw.txt` from the headers. If you add a switch, you can add its row to `tools/config_table.json`, or leave that to the release step.
