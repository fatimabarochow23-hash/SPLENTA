# SPLENTA - AI SOP (Source of Truth)

## Project Facts (must not guess)
- Repo: https://github.com/fatimabarochow23-hash/SPLENTA.git
- Default branch: main
- Local path (macOS): /Users/MediaStorm/Desktop/NewProject
- Tech: JUCE, C++, Xcode (macOS), VS2022 (Windows)

## Source of Truth (Task)
- Current Task Pointer: TASKS/CURRENT_TASK.md
- You MUST open and follow the task file referenced inside TASKS/CURRENT_TASK.md
- Implement ONLY what the current task asks (no extra refactors)

## Non-negotiable Rules
1. Never claim “done/pushed” without evidence:
   - Provide `git log -1 --oneline`
   - Provide `git status` (must be clean after commit)
   - List modified/added files
2. If something is not implemented, say so explicitly and do NOT imply completion.
3. If you add new .h/.cpp files:
   - You MUST add them in `NewProject.jucer` (Projucer → Files tree → Add Existing Files)
   - Then Save Project and re-export/refresh Xcode project
4. Do not change `createParameterLayout()` unless the task explicitly requests it.
5. Timebox: if you cannot finish within limits, stop and output:
   - what is done
   - what remains
   - exact next steps

## The Loop (do in this order every session)
1. `cd /Users/MediaStorm/Desktop/NewProject`
2. `git pull origin main`
3. Read: `AI_PROMPT.md` then `TASKS/CURRENT_TASK.md` then the referenced task file
4. Implement exactly the task scope
5. Build (Xcode): at least one configuration (Debug or Release)
6. Minimal host sanity check (Reaper): launch plugin, verify the task’s acceptance criteria
7. Versioning + commit:
   - Commit message format: `V[Major].[Minor] - YYYYMMDD.BB`
   - Also update file headers if the task requests it
8. `git push origin main`
9. Report with evidence (see “Non-negotiable Rules”)

## Notes
- If UI files are missing in Xcode after adding them on disk: update `.jucer` and re-export.
- Do not delete or rename parameters lightly (host session compatibility).
