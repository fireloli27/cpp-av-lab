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
2. Before commit, write or update the paired **课记** `docs/learning/{audio|video}/{stage}.notes.md` using `docs/learning/_notes_template.md` (all six sections). Link it from the short essay.
3. Tick the课记 Acceptance item on the issue when present.

Harness / non-stage tickets skip课记.

Commit your work to the current branch.
