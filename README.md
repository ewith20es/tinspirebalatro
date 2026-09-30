# Balatro — TI-Nspire edition

A playable, fan-made poker roguelike for a **TI-Nspire CX II CAS with Ndless**. It is built around a mouse-like touchpad cursor, a 320×240 card table, animated dealing and scoring, and a complete run through eight antes.

## Play on your calculator

1. Download **balatro.tns** from the [releases page](https://github.com/ewith20es/tinspirebalatro/releases), or build with **Build Balatro.cmd** / **Ctrl+Shift+B** in VS Code.
2. Transfer **balatro.tns** using your TI-Nspire Student Software.
3. Open it on the calculator with Ndless installed. Your CX II CAS on OS 6.4.0.74 fits this setup.
4. Choose **New Run**, then **Play Blind**.

The `.tns` contains all game code and graphics. No additional images, fonts, ROMs, network connection, or original-game files are needed.

## Controls

| Control | Action |
|---|---|
| Slide a finger on the touchpad | Move the cursor, like a laptop trackpad |
| Press the touchpad | Click a card or button |
| Hold the pad down and move | Drag a card or Joker to reorder it |
| Space | Click at the cursor; activate a button selected with Tab |
| P or Enter | Play selected cards / start the blind / leave the shop |
| D or Delete | Discard the selected cards |
| S | Switch between rank and suit sorting |
| 1–9, then 0 | Toggle the corresponding card in your hand |
| H | Open the rules and controls |
| Tab, then Enter | Move through and activate interface controls |
| I / J / K / L | Move cursor up / left / down / right without the pad |
| Esc or Menu | Pause; press Esc again to resume |
| Ctrl+Esc | Save and exit directly |
| Click during scoring | Finish the scoring animation immediately |

Use **Menu → Save & Quit** to leave normally. Hover over cards, Jokers, or shop offers for explanations. In the shop, click an owned Joker to inspect or sell it.

## How a run works

- Start with a normal 52-card deck, $4, eight cards in hand, four plays, and three discards per blind.
- Select **one to five cards**. The best poker pattern in that selection determines base chips and Mult. Only the cards making that pattern contribute their individual chip values.
- Reach the blind's target before running out of hands. Unplayed cards remain in hand; played/discarded cards are not returned to the draw pile until the next blind.
- Beat Small, Big, and Boss blinds in each ante. The Boss adds a rule, such as debuffed suits, no discards, or a larger score target.
- After a win, earn $3/$4/$5 for the blind, $1 per hand remaining, and $1 interest per $5 already held (maximum $5). Golden Jokers add their bonus.
- Spend money in the shop on **26 Joker types**, planets that level up specific hands, or packs that permanently enhance three random cards. You can own five Jokers; their left-to-right scoring order matters.
- Beat Ante 8 to win, then optionally continue in endless mode.

Skipping a Small or Big blind gives **$4** in this version. Boss blinds cannot be skipped.

## Saves

The game saves after plays, discards, purchases, blind transitions, and when pausing or exiting. Resume is available from the title screen. Selecting a new run asks before replacing the existing run.

The save is kept beside the program as `balatro-save.tns`. A `.bak.tns` backup may also appear. Leave these files with the game; they are game data and are not documents to open manually. Checksums detect truncated/corrupted files, and the previous save is recovered when a valid backup is available.

## Included content and differences

This is a compact original implementation inspired by Balatro, with its own code and pixel art. It includes the core hand/scoring/shop loop, 12 hand types (including enhanced-deck patterns), 26 Jokers, eight boss rules, planets, four upgrade packs, a deck viewer, and animated score reveals.

It does **not** reproduce the full commercial game's 150+ Jokers, unlocks, stakes, challenges, vouchers, consumable inventory, music, shaders, or art. Upgrade packs are simplified: Strength raises three random ranks; Bonus adds 30 chips, Mult adds 4 Mult, and Glass adds X1.5 Mult to three random cards. Glass cards do not break in this edition. Fortune Teller grows from planets in this version. No real-money gambling is involved.

## Development

Open `example\balatro` in the installed VS Code. The local tasks and compiler configuration are included.

From the configured Cygwin terminal in this folder:

```sh
make -j4           # build balatro.tns
make test          # exhaustive poker and game-state/input/save tests
make desktop       # build the Windows mouse preview
./build/balatro-desktop.exe build/desktop-save.tns
```

You can also double-click **Desktop Preview.cmd**. Close the desktop preview before rebuilding that executable. The desktop build uses the exact same engine, interface, and framebuffer drawing code as the calculator. Only input, timing, and display output differ. Calculator animation timing uses a fixed step and a short sleep; desktop preview speed cannot certify physical-device performance.

Source map:

- `engine.*`: deck, hand evaluation, scoring, Jokers, shops, progression, and saves.
- `graphics.*`: original pixel font, cards, suits, Jokers, panels, and framebuffer drawing.
- `app.*`: screen layout, mouse interaction, card dragging, menus, effects, and animations.
- `platform.cpp`: Ndless touchpad/LCD integration and the Windows preview.
- `tests.cpp`: exhaustive poker frequencies, targeted gameplay/save checks, mouse event tests, and screen captures.

## Validation

The game cross-compiles to an ARM `.tns` with the installed GCC 14.2.0 Ndless toolchain. Automated tests enumerate all 2,598,960 standard five-card poker hands and check their known category totals. Additional tests cover scoring-card masks, Joker order, boss effects, shop transactions, draw-pile depletion, loss/win/endless transitions, save/recovery behavior, card clicking/dragging, pause/resume, and animation skipping.

The UI has been reviewed using screenshots generated by the same renderer and a Windows desktop preview. Physical CX II CAS execution, touchpad sensitivity, and on-device frame rate have not yet been tested.

## Credits and references

Balatro is by LocalThunk and Playstack. This is an unofficial fan project, not an official port. No original-game code or assets are included.

- [Balatro](https://www.playbalatro.com/)
- [Ndless SDK](https://github.com/ndless-nspire/Ndless)
- [TI-Nspire native development](https://www.hackspire.org/C_and_assembly_development_introduction/)
