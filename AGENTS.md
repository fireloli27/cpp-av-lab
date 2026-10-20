## Agent skills

### Issue tracker

Issues live as markdown files under `.scratch/<feature>/`. Spec path: `.scratch/<feature>/spec.md`. See `docs/agents/issue-tracker.md`.

### Triage labels

Canonical roles map 1:1 to tracker labels (`needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`, plus local `done`). See `docs/agents/triage-labels.md`.

### Domain docs

Single-context: root `GLOSSARY.md` + `docs/adr/`. See `docs/agents/domain.md`.

### Learning Project

Experiments, essay + **课记** layout, and the Catch2/CTest guardrail: `docs/learning/harness/README.md`. Stage tickets: short essay `{stage}.md` (run/accept) + notes `{stage}.notes.md` (understand code). Templates: `_template.md`, `_notes_template.md`. One-command check: `./scripts/check-learning.sh`. Enable commit hook: `./scripts/install-git-hooks.sh`.

### Teach classroom

Personal `/teach` workspace lives under `teach/` (not mixed with `docs/`). Map: `teach/README.md`.

### Git

`/implement` expects an existing git repository (`.git`). Initialize (and preferably make a baseline commit) before implementing; do not invent a first commit that silently dumps the whole tree mid-ticket.
