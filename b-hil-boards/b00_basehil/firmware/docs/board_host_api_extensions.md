# Board Host API Extensions — `b00_basehil`

Human / implementer checklist. **Runtime discovery for agents is on-device
`capabilities`**, not this file.

```text
Base contracts:
  - docs/external_host_api_architecture.md
  - docs/external_host_command_api.md
  - docs/external_host_stream_api.md
Compatibility: must not change /v1 paths, Base64 frame limits, or mandatory
  ping / info / capabilities
Mandatory base commands on this board: ping, info, capabilities
  (when host API firmware ships)
Extra commands (reference demos — not required of sibling boards):
  - name: echo
    Purpose: Round-trip a short text string (command-path smoke test)
    Args: { "text": <string, required, max 128 bytes> }
    Result: { "text": <same string> }
    Errors: bad_request
Streams (reference demos — not required of sibling boards):
  - stream_id: loopback
    Purpose: Echo written frames back on read (stream-path smoke test; no DUT UART)
    params schema: {} (empty object)
    Payload semantics (host→board / board→host): opaque bytes; write enqueues
      identical payload for subsequent read (FIFO); max 1024 bytes/frame
    Hardware backend: none (in-RAM queue)
Expected capabilities.summary (English):
  Family base HIL bench with host API demos (echo, loopback)
Expected info.streams_supported: true (once loopback ships)
Non-extensions (forbidden):
  - Replacing base discovery or WiFi onboarding protocols
  - Shadow APIs that duplicate base host semantics under new names without cause
  - Teaching agents to read this Markdown instead of calling capabilities
```

This base tree owns the **family** host API contracts. Product boards copy this
file into their repo and list extra `name` / `stream_id` values there — and MUST
embed the same catalog in firmware `capabilities`.
