# Contributing to SolarSystemGL

Thanks for your interest. This is a hobby / portfolio project, contributions are welcome.

## Quick start

1. Fork the repo and create a feature branch from `master`.
2. Build locally — see [`docs/build.md`](docs/build.md) (or `docs/build-guide.md` until the docs rewrite lands).
3. Make your change. Keep the diff focused — one concern per PR.
4. Verify build is clean under MSVC `/W4 /permissive-` (no new warnings).
5. Open a PR with a clear title and a one-paragraph "why".

## Code style

- C++17, no third-party additions without discussion.
- Indent: 4 spaces, no tabs.
- Naming: `PascalCase` for types, `camelCase` for functions/variables, `UPPER_SNAKE` for constants.
- Headers: `#pragma once`, group includes (std, third-party, project).
- Comments only when the *why* isn't obvious — names should carry intent.
- No emojis in source.

## Scope guidelines

- **Bug fixes** — always welcome. Include a brief repro.
- **Features** — check [`TODO.md`](TODO.md) first. P1 items are good first targets. Open an issue if you want to claim a P2 item.
- **Refactors** — discuss in an issue before opening a PR; large rewrites should be motivated by a concrete pain point.
- **Docs** — currently being rewritten from scratch. See `TODO.md` → P1 → Documentation rewrite. The existing `docs/*` is being treated as draft.

## Architecture pointers

- `src/objects/CelestialBody.h` — single source of truth for bodies (physics + visual identity).
- `src/objects/PlanetMesh.h` — GL state (VAO/VBO/EBO) with Rule of 5.
- `src/physics/PhysicsSystem.h` — N-body Newtonian integrator. Currently Euler-Cromer; Verlet is on the roadmap.
- `src/ui/UIManager.h` — ImGui panels.
- `src/main.cpp` — owns the loop, the bodies vector, and the camera.

## Reporting bugs

Open an issue with:
- OS + compiler (MSVC2022 / MinGW / clang).
- What you did (steps), what you expected, what happened.
- Screenshot if visual; stderr output if logical.

## License

By contributing you agree your work is released under the MIT license (see [`LICENSE`](LICENSE)).
