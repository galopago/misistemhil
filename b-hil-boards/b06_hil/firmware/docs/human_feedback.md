# Human Feedback

Human feedback enters the workflow through the tester and is transformed into
verifiable tasks. The goal is to distinguish observation, reproduction, and
action.

All agentic development methodology documentation in this firmware tree is
maintained in English.

## Input Format

```text
Date:
Person:
Context:
Firmware tested:
Hardware tested:
Observation:
Expected result:
Steps to reproduce:
Evidence:
Impact:
```

## Classification

- Bug: reproducible incorrect behavior.
- Requirement: new behavior or criteria that were not covered.
- Question: missing information needed to decide.
- Improvement: desirable change without a functional failure.

## Flow

1. The tester records the entry in `agent-workspaces/tester/feedback.md`.
2. If it is reproducible, the tester adds evidence in `test_runs.md`.
3. If it requires a design change, the tester routes it to the architect.
4. If it is a small and clear fix, it may be handed to the implementer with
   explicit acceptance criteria.

## Criteria For Moving To Implementation

- There are reproduction steps or a concrete requirement.
- The expected behavior is described.
- The affected file or component is identified or bounded.
- The hardware conditions are documented.
