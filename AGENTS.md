## Agent skills

### Issue tracker

Issues live as markdown files under `.scratch/<feature>/`. Spec path: `.scratch/<feature>/spec.md`. See `docs/agents/issue-tracker.md`.

### Triage labels

Canonical roles map 1:1 to tracker labels (`needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`, plus local `done`). See `docs/agents/triage-labels.md`.

### Domain docs

Single-context: root `GLOSSARY.md` + `docs/adr/`. See `docs/agents/domain.md`.

### Doc language

Prose in **Chinese**. Keep **English** for GLOSSARY terms, API/type names, paths, commands, and format tokens (e.g. PCM, WAV, AudioQueue, s16le). Do not hard-translate those into Chinese as the primary label.

### Learning Project

Experiments, essay + **课记** layout, and the Catch2/CTest guardrail: `docs/learning/harness/README.md`（含 agent 跑护栏/commit 须非沙箱、Opus CAF 导航）. Stage tickets: short essay `{stage}.md` (run/accept) + notes `{stage}.notes.md` (understand code). Templates: `_template.md`, `_notes_template.md`. One-command check: `./scripts/check-learning.sh`. Enable commit hook: `./scripts/install-git-hooks.sh`. Review judgement: root `CODING_STANDARDS.md`. Follow Doc language above.

### Teach classroom

Personal `/teach` workspace lives under `teach/` (not mixed with `docs/`). Map: `teach/README.md`. Same Doc language rule for lessons and reference HTML.

### Git

`/implement` expects an existing git repository (`.git`). Initialize (and preferably make a baseline commit) before implementing; do not invent a first commit that silently dumps the whole tree mid-ticket.
