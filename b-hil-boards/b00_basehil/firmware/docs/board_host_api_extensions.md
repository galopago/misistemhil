# Board Host API Extensions — `b00_basehil`

```text
Base contract: docs/external_host_api_architecture.md
Compatibility: must not break base methods/fields required by the family host API
Extensions:
  - none for this base reference board in v1
Non-extensions (forbidden):
  - Replacing base discovery or WiFi onboarding protocols
  - Shadow APIs that duplicate base host semantics under new names without cause
```

When the family host API is specified, this base board implements the shared
surface. Product boards add extensions only in their own copy of this file.
