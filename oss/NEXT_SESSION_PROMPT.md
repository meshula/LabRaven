# LabRaven OSS Framework - Next Session Prompt

Copy and paste this prompt to continue work on the LabRaven OSS framework transformation.

---

## Prompt for Next Claude Session

```
I'm continuing work on transforming LabRaven into an open-source C++ application framework.

PROJECT CONTEXT:
- Location: /Users/nick/dev/Lab/LabRaven/oss/LabRaven
- Goal: Extract core framework from domain-specific LabRaven codebase
- Architecture: Studio-Activity-Provider pattern
- Platforms: macOS, iOS, Windows, Linux, WebAssembly

COMPLETED WORK (Phases 1-3):
✅ Phase 1: Repository Setup - Complete directory structure, git initialized
✅ Phase 2: Core Framework Extraction (11 files, ~2,650 lines)
   - Core headers: StudioCore.hpp, Transaction.hpp, Orchestrator.hpp, CSP.hpp, App.h
   - Platform abstraction: macOS template + comprehensive guide
   - UI utilities: ImGuiHelpers.hpp, Themes.hpp
   - Core utilities: Logging.hpp
   - All domain dependencies removed (USD, etc.)
   - Comprehensive documentation

✅ Phase 3: Platform Support & Build System (7 files, ~785 lines)
   - Complete CMake build system (CMakeLists.txt)
   - Dependency management (ext/CMakeLists.txt)
   - Build presets (CMakePresets.json)
   - ZeroMQ auto-fetch via FetchContent
   - Platform detection (macOS/iOS/Windows/Linux/WASM)
   - Build scripts (setup.sh, build.sh)

CURRENT STATUS:
- Phases Complete: 3 of 9 (33%)
- Total Files: 22 files, ~3,500 lines
- Git Commits: 4 commits, clean history
- Next Phase: Phase 4 - Sample Components

DOCUMENTATION TO READ:
Please read these files for full context (in order):
1. oss/STATUS.md - Current progress and detailed status
2. specification/OSS_FRAMEWORK_SPECIFICATION.md - Target architecture
3. specification/OSS_TRANSFORMATION_PLAN.md - Complete roadmap

KEY FILES LOCATIONS:
- Framework core: oss/LabRaven/src/LabRaven/Core/
- Platform code: oss/LabRaven/src/LabRaven/Platform/
- Build system: oss/LabRaven/CMakeLists.txt
- Status tracking: oss/STATUS.md

PHASE 4 TASKS (Sample Components):
Create minimal but representative components to demonstrate the framework:

Activities (5 total):
1. AppMenu - Application menu and window management
2. Preferences - User preferences UI
3. Console - Interactive console (s7 Scheme integration)
4. Journal - Transaction history viewer
5. Demo - ImGui demo wrapper

Providers (2 total):
1. CameraProvider - 3D camera state management
2. SelectionProvider - Selection state management

Studios (2 total):
1. MinimalStudio - Bare essentials (AppMenu + Preferences)
2. DemoStudio - Full showcase (all Activities + all Providers)

IMPORTANT GUIDELINES:
- Keep code header-only where possible (no .cpp until needed)
- Follow existing documentation style (comprehensive inline docs)
- Use LabRaven namespace consistently
- No domain-specific code (framework only)
- Document every public API
- Include usage examples in comments

NEXT ACTIONS:
1. Review oss/STATUS.md to understand current state
2. Create Activities in src/Activities/ (one subdirectory per Activity)
3. Create Providers in src/Providers/ (one subdirectory per Provider)
4. Create Studios in src/Studios/ (one subdirectory per Studio)
5. Add to src/CMakeLists.txt as needed
6. Test integration
7. Commit with clear message
8. Update oss/STATUS.md

Let's continue with Phase 4: Sample Components. Start by creating the AppMenu Activity.
```

---

## Alternative Short Prompt

If you need a more concise version:

```
Continue LabRaven OSS framework transformation at /Users/nick/dev/Lab/LabRaven/oss/LabRaven

COMPLETED: Phases 1-3 (Repository, Core Framework, Build System)
CURRENT: Phase 4 - Sample Components (Activities, Providers, Studios)

Read for context:
1. oss/STATUS.md
2. specification/OSS_FRAMEWORK_SPECIFICATION.md

TASK: Create 5 sample Activities (AppMenu, Preferences, Console, Journal, Demo), 2 Providers (Camera, Selection), and 2 Studios (Minimal, Demo). Keep header-only, comprehensive docs, LabRaven namespace.

Start with AppMenu Activity in src/Activities/AppMenu/
```

---

## Usage Notes

1. **First Time**: Use the full prompt above
2. **If Claude asks for more context**: Point to oss/STATUS.md
3. **If stuck**: Reference specification/OSS_TRANSFORMATION_PLAN.md
4. **Quick continuation**: Use the short prompt

The full prompt gives complete context while the short prompt is for quick continuations when you know the session will pick up context from the files.
