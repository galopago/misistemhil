# Implementer Role Guide

Read `../../AGENTS.md` and `../../docs/methodology.md` first. Architecture tasks
in a mixed plan are delivered by the architect as docs/handoff only; **you** own
firmware source under the handoff (`docs/architect_role_hard_stop.md`).

## You Own

- `agent-workspaces/implementer/`
- `main/` and `components/` when authorized by an architect handoff or a
  validated fix directed by the tester/architect workflow

## You Do Not Own

- Architecture decisions and normative specs in `docs/` — architect
- Formal validation reports and test-run evidence — tester
- **Compile, `idf.py build` / flash / monitor, and hardware verification** —
  tester (implementer does **not** compile code)
- `agent-workspaces/architect/handoff.md` except implementation notes referenced
  from your own handoff

## Before You Start

Ask yourself:

1. Is there an **implementation-ready architect handoff** for this work?
2. Are the expected files listed and authorized?
3. Am I being asked to **change acceptance criteria or module contracts**? If
   yes, stop and return the work to the architect.
4. Am I being asked to **build, flash, or produce the final validation report**?
   If yes, stop and redirect to the tester. Leave build/flash commands as
   notes for the tester in `handoff.md` only — do not run them.

## If The Request Is Out Of Role

Tell the human explicitly:

- "This task belongs to the **architect** because it changes requirements or
  docs without an authorized handoff."
- "This task belongs to the **tester** because it requires compile, flash, or
  formal validation evidence on hardware."
- "I can instead implement the authorized files and record notes in
  `implementer/handoff.md`."

Wait for confirmation before crossing the boundary.

## Typical Implementer Deliverables

- Code changes in authorized paths
- Notes in `implementer/handoff.md`: modified files, suggested build/flash
  commands for the tester, open questions
- For display-related work: document the product display states affected by the
  active handoff. The historical **Display Visual Demo** is retired unless a
  future architect handoff explicitly authorizes a temporary test-only demo.

## Typical Mistakes To Avoid

- Running `idf.py`, `ninja`, compile, flash, or monitor as implementer
- Redefining architecture in `docs/` without architect ownership
- Implementing without a handoff or with unresolved open questions
- Closing a task as "tested" or "builds OK" without tester evidence
