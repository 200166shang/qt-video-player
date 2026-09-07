# PlayerLab desktop editing prototype

Throwaway prototype for deciding the first desktop editing experience and information architecture.

Run it with:

```bash
python3 prototype/desktop-editor/serve.py
```

Then open <http://127.0.0.1:8121>. Switch among the three structures with the floating bar or the left/right arrow keys:

- `?variant=A` — Classic three-pane editor
- `?variant=B` — Story-first studio with an always-visible multitrack timeline
- `?variant=C` — Guided focus workflow

The controls are intentionally local stubs. Click media, clips, section tabs, save, restore, editing actions, and export to expose the interaction model without connecting a media engine.
