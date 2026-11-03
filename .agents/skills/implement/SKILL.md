---
name: implement
description: "Implement a piece of work based on a spec or set of tickets."
disable-model-invocation: true
---

Implement the work described by the user in the spec or tickets.

If the user passes a ticket reference, fetch it from the issue tracker and state its title before starting. If the reference is ambiguous, ask.

Call the Skill tool with "tdd" where possible, at pre-agreed seams.

Run typechecking regularly, single test files regularly, and the full test suite once at the end.

Once done, call the Skill tool with "code-review" to review the work. Prefer **working-tree** mode (uncommitted diff vs `HEAD`) so review runs before the commit step.

## Learning Pipeline Stage tickets

When the ticket is a Learning Pipeline Stage under `.scratch/learning-project/issues/` (audio/video capture|encode|local-transport|decode|render — not harness-only tickets):

1. Ship the experiment + acceptance **短文** as usual (`docs/learning/{audio|video}/{stage}.md`).
2. Before you stop for review, write or update the paired **课记** `docs/learning/{audio|video}/{stage}.notes.md` using `docs/learning/_notes_template.md` (all six sections). Link it from the short essay.
3. Tick the课记 Acceptance item on the issue when present.
4. If the ticket 约定/Acceptance mentions `teach/` or 教室: before done, `rg` **all** of `teach/` (MISSION, lessons, reference, learning-records) for the stage number and contradicted protocol names; fix every hit, not only `MISSION.md`.

Harness / non-stage tickets skip课记.

## Grilling leftovers

Prefer `/grill-with-docs` to commit ADR / GLOSSARY / issue / spec edits before `/implement`. If those files are still dirty for this ticket when implement starts, keep them in the working tree with the implement changes (do not leave them orphaned, and do not commit).

## Review before commit

Do not commit. Stop with the changes uncommitted and summarise the diff so the user can review. Commit only when they explicitly ask in a later message.
