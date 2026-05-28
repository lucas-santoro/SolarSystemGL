# ADR 0003 — Apply button instead of live editing

**Status:** Accepted
**Date:** 2026-05

## Context

When wiring the Planet Info edit panel to the physics state, three UI patterns were on the table:

1. **Live editing, simulation running** — each ImGui field change writes into the body. Physics keeps advancing.
2. **Live editing with implicit pause** — when the Planet Info window has focus, physics auto-pauses; values stick under the user's fingers.
3. **Apply button** — fields write to a buffer; an explicit click copies buffer → body. Simulation continues during editing.

The bug being fixed (Apply does nothing, see [ADR 0001](0001-celestialbody-unified-state.md)) was orthogonal to this choice; any of the three would have fixed it. The question was which UX to ship.

## Decision

Keep the existing **Apply button** (option 3).

## Rationale

**Position edits during a running simulation are weird in live mode.** With `timeScale = 864 000` (default), Earth moves about 30 km per real second of UI time. That's invisible in the render but the *number* in the position field is shifting. If the user is typing "1.5", the value they're typing fights the simulation. Auto-pause (option 2) avoids that, but introduces non-obvious behavior: "why is the sim frozen? Oh right, I clicked into a text field."

**Apply gives atomic updates.** All five fields commit together. The user controls the moment of change. The reset button reverts unchanged buffer state. This matches users' mental model of "I'm editing a record."

**The global Pause checkbox is there for users who do want to stop time while editing.** The user can pause, edit, apply, unpause — explicit and discoverable.

## Trade-offs accepted

- The simulation isn't paused while the panel is open, so the body the user is editing keeps moving. The user has to either pause manually or accept that positions are advisory until apply.
- The buffer pattern requires the populate/repopulate logic in `render()` that compares `selectedPlanetIndex` to `lastSelectedIndex`. Not free, but contained.
- Velocity Verlet integrator (now in `PhysicsSystem`) makes position edits work cleanly: applying a teleport (new `pos_m`) at any moment is correct on the next sub-step. No special invalidation step needed.

## Future: pivot if needed

If a future user research session reveals that live editing is the expected pattern (e.g., for sliders, dragging fields), revisit. Easy path forward: change `InputFloat` to `DragFloat` and write into `body` directly on change. The current Apply pattern doesn't lock us out of switching.
