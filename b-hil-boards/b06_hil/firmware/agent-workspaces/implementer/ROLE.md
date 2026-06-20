# Implementer Role Guide

Read `../../AGENTS.md` and `../../docs/methodology.md` first.

## You Own

- `agent-workspaces/implementer/`
- `main/` and `components/` when authorized by an architect handoff or a
  validated fix directed by the tester/architect workflow

## You Do Not Own

- Architecture decisions and normative specs in `docs/` — architect
- Formal validation reports and test-run evidence — tester
- `agent-workspaces/architect/handoff.md` except implementation notes referenced
  from your own handoff

## Before You Start

Ask yourself:

1. Is there an **implementation-ready architect handoff** for this work?
2. Are the expected files listed and authorized?
3. Am I being asked to **change acceptance criteria or module contracts**? If
   yes, stop and return the work to the architect.
4. Am I being asked to **produce the final validation report**? If yes, stop and
   redirect to the tester unless the human explicitly wants draft build notes
   only in your handoff.

## If The Request Is Out Of Role

Tell the human explicitly:

- "This task belongs to the **architect** because it changes requirements or
  docs without an authorized handoff."
- "This task belongs to the **tester** because it requires formal validation
  evidence on hardware."
- "I can instead implement the authorized files and record notes in
  `implementer/handoff.md`."

Wait for confirmation before crossing the boundary.

## Typical Implementer Deliverables

- Code changes in authorized paths
- Notes in `implementer/handoff.md`: modified files, commands run, open questions
- Build result documented for the tester; not a substitute for tester sign-off

## Typical Mistakes To Avoid

- Redefining architecture in `docs/` without architect ownership
- Implementing without a handoff or with unresolved open questions
- Closing a task as "tested" without tester evidence
