# Firmware Architecture Notes

This document is the implementer's source of truth for the ESP32 application.
Use it to describe intended behavior, system responsibilities, implementation decisions,
technical questions, and execution notes in a way that another implementation agent can
translate into working firmware with minimal interpretation.

All agentic development methodology documentation in this firmware tree is
maintained in English.

## Writing Principles

- Describe behavior in a language-agnostic way whenever possible.
- Prefer system responsibilities, inputs, outputs, states, constraints, and acceptance criteria over programming-language details.
- Avoid naming framework-specific APIs unless the architectural decision truly depends on them.
- When hardware details are required, describe the peripheral role, signal direction, timing, electrical expectation, and failure behavior.
- Make assumptions explicit so implementation agents can preserve or challenge them safely.
- Record open questions separately from decisions so uncertainty does not become hidden behavior.
- Keep notes precise enough that two independent agents should produce substantially similar firmware behavior.

## Application Intent

The firmware will run on an ESP32-based board and provide the embedded behavior required
by this hardware target. The current skeleton only starts the application and does not
enable peripherals yet.

The architecture should evolve around clear firmware responsibilities:

- Initialize the board into a known safe state.
- Configure required peripherals only after their expected behavior is documented.
- Expose deterministic startup, runtime, error, and recovery behavior.
- Separate hardware-facing behavior from application-level decisions.
- Preserve observability through logs, status indicators, or telemetry where appropriate.

## Architectural Notes Template

```text
Date:
Context:
Goal:
Expected firmware behavior:
Inputs:
Outputs:
States and transitions:
Timing constraints:
Hardware assumptions:
Failure handling:
Implementation constraints:
Acceptance criteria:
Commands executed:
Open questions:
Result:
```

## Initial Note

The current firmware skeleton only initializes the application and does not enable any
peripherals.
