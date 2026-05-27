# User guide

A short tour. Build instructions are in [`build.md`](build.md).

## First launch

You should see the Sun, the eight planets, and a faint white grid below them, slightly depressed near each body. The grid distortion scales with mass — Jupiter and Saturn dimple it visibly; Mercury barely at all.

If you see a white sphere and a white grid only, your shaders didn't load. Run from a terminal to see `stderr` and check `build.md` → Common issues.

## Camera

| Input | Free mode | Orbital mode |
|---|---|---|
| `W` / `S` | Move forward / back | — |
| `A` / `D` | Strafe left / right | — |
| Right-click + drag | Look around | Orbit the target |
| Scroll up / down | Increase / decrease movement speed | Zoom in / out |
| Click on a planet | Smooth-fly toward it | Re-target (snap) |

There are two modes:
- **Free** — fly with WASD, look with right-drag. Scroll changes speed.
- **Orbital** — locked to a planet, you orbit it. Scroll zooms.

Switch modes from the radio buttons inside Planet Info, or via hotkeys (see below).

### Touch (web / mobile)

| Gesture | Action |
|---|---|
| One-finger drag | Look around (Free) / orbit the target (Orbital) |
| Two-finger pinch | Zoom in / out |
| Tap a planet | Select / focus it |
| Tap a control | Same as a click |

On narrow screens the top action bar collapses to **Menu · System · + · Save · Load · ⋯**; the `⋯` button opens an overflow menu with time, camera, view presets, settings, Demo Mode, and Screenshot. There's no keyboard, so the keyboard-only hotkeys (bookmarks `F5`–`F8`) aren't available on touch.

## Saving (and the web build)

Save/Load writes the whole simulation to a human-readable `.txt`. On **desktop** these live under `presets/`. On the **web build** your saves persist in the browser (IndexedDB) and survive a reload; the Load modal shows *Built-in presets* and *My saves* separately, with per-save **Export** (download) and **Delete**, plus **Import** to load a `.txt` from disk. In private-browsing mode persistence is unavailable and you'll get a warning that saves won't survive a reload.

**Rendering quality:** Settings has an **MSAA** toggle (anti-aliasing). It defaults off on mobile to save GPU; turn it on for crisper edges on a capable device.

## Hotkeys

Edge-detected: one press = one action. Suppressed while a text field has keyboard focus (so typing into Mass / Position / Name doesn't fire shortcuts).

| Key | Action |
|---|---|
| `Space` | Toggle pause |
| `R` | Reset camera (back to startup pose, FREE mode) |
| `F` | Smooth-focus the camera on the selected body |
| `1` | Switch camera to FREE mode |
| `2` | Switch camera to ORBITAL mode (requires a selected body) |
| `Esc` | If a body is selected → close Planet Info (deselect). Otherwise → close app. |
| `W` `A` `S` `D` | Held-key movement (FREE mode only) |

## Panels

### Planets menu (top bar)
Click "Planets" → pick a name → the body becomes the current selection. The Planet Info panel opens for editing.

### Planet Info
Edit any body's properties. **Edits do not apply until you click "Apply Changes"**. While you're typing, the simulation continues to run normally.

| Field | Unit |
|---|---|
| Name | string |
| Mass | kg (scientific notation, e.g. `5.97e24`) |
| Density | kg / m³ |
| Position | World Units (1 WU = 10⁹ m) |
| Velocity | km / s |

Apply triggers a full geometry rebuild (mass / density change radius, which changes the mesh).

### Solar System panel
- **FPS** — smoothed (EMA) display.
- **Paused** — checkbox to freeze physics. Render keeps running; you can still orbit.
- **Time scale** — logarithmic slider, 1× to 5 × 10⁶×. At 1× one real second simulates one second; at the default ~864 000× one real second simulates 10 days.
- **Add Planet** — spawns a ~10²⁴ kg grey body at 1.5 AU with a tangent circular velocity. Each successive add lands at a different angle on the same orbit. The new body is auto-selected, so the Planet Info panel jumps to it.

## Worked examples

### Crash Earth into the Sun
1. Planets → Earth → Planet Info opens.
2. Set Velocity to `0, 0, 0` km/s. Apply.
3. Earth detaches from its orbit and falls inward. It will swing around the Sun on a nearly degenerate hyperbolic trajectory (the softening parameter prevents an actual singularity).

### Watch Jupiter wreck the inner system
1. Planets → Jupiter → Planet Info.
2. Set Mass to `1e29` (~50× current). Apply.
3. Inner planets get perturbed within a few seconds of real time. Mercury frequently ejects.

### Time-lapse one Earth year in 10 seconds
1. Time scale slider to ~3.15 × 10⁶ (one Earth year ≈ π × 10⁷ s, divided by 10 s of real time).
2. Watch Earth complete one full lap around the Sun.

### Add a few asteroids
1. Click Add Planet several times.
2. Each new body lands at a different angle on the 1.5 AU orbit. They interact gravitationally with each other and with the gas giants.

## Troubleshooting

**Add Planet button does nothing visible** — Check the Planets menu, the new body is named "New Planet" and should appear. Default mass (~10²⁴ kg) and visual radius (~0.4 WU) mean it's a small grey dot at 1.5 AU. The auto-select should snap the Planet Info panel to it; if not, click it from the menu.

**Orbits look wrong after editing mass** — Velocity Verlet integrates from the *current* state. If you change mass, the body's orbital velocity is unchanged but the gravitational force on it changes. The orbit will evolve from there.

**Sliders fight with each other** — Time scale and Paused are independent. Paused stops physics regardless of time scale. Time scale only matters when not paused.

**FPS drops** — At default 9 bodies it shouldn't. If it does and you've added many more bodies, the N² physics is the culprit (P2 has Barnes-Hut listed).
