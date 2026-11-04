# Implementer backlog — `b03_hil`

- [x] Adapt-copy base firmware scaffold (`B03_PRODUCT_BRINGUP`)
- [x] Board pins + identity `03`
- [x] `relay_ctrl` OE-safe init + set/get (`B03_RELAY_CONTROL`) — **init order
      superseded by `B03_RELAY_OE_LEVEL_BEFORE_DIR`**
- [x] `app_core` early init + facades
- [x] Host API relay commands + capabilities (`B03_HOST_API_RELAY`)
- [x] **Patch `relay_ctrl_init` level-before-direction** (`B03_RELAY_OE_LEVEL_BEFORE_DIR`)
- [ ] Retest handoff to tester (power-cycle no short chatter)
