# PR draft: custom window chrome v1

**Branch:** `feature/custom-window-chrome-v1` → `master`  
**Create URL:** https://github.com/truth/ai-win-ui/pull/new/feature/custom-window-chrome-v1

```powershell
gh auth login
gh pr create --base master --head feature/custom-window-chrome-v1 \
  --title "feat: custom window chrome with refined caption buttons" \
  --body-file doc/pr-custom-window-chrome-v1.md
```

## Summary

- Optional borderless custom window chrome (`WindowChrome`) with `WM_NCCALCSIZE` / `WM_NCHITTEST` / `WM_GETMINMAXINFO` and DWM rounded corners
- UI title bar via layout attrs (`chrome`, `hitTest`, `role=titleBar`) and geometric caption buttons (`caption-minimize` / `caption-maximize` / `caption-close`)
- Maximize switches to restore glyph; inactive window dims caption band; custom windows center on the work area and draw a 1px edge
- **v2 Layered irregular windows**: `PresentMode::Layered` (Direct2D DC RT + DIB + `UpdateLayeredWindow`), alpha pass-through hits, soft card shadow, maximize collapses floating margins
- Demo layouts + launcher scripts; default remains system title bar (opt-in)
- Fix Skia Release linking to use per-config library lists (avoid Debug-only `spvtools`)

## Test plan

- [ ] `.\build.ps1 -Configuration Release` succeeds
- [ ] Default launch (no env) still uses system chrome + existing layouts
- [ ] `.\scripts\run_custom_chrome_demo.ps1 -Configuration Release` shows custom title bar, buttons top-right
- [ ] Drag title bar, resize edges, double-click caption maximize/restore
- [ ] Min / Max-Restore / Close work; close hover is crimson; maximize icon becomes restore when maximized
- [ ] Click another window: caption dims; focus back restores full brightness
- [ ] Content TextInput and action buttons remain clickable
- [ ] `.\scripts\run_layered_chrome_demo.ps1` shows rounded floating card with desktop in corners + soft shadow
- [ ] Layered: clicks on transparent corners pass through; maximize fills work area

## Docs

- `doc/plan/2026-07-15-custom-chrome-irregular-window-v1.md`
- `doc/plan/2026-07-15-layered-irregular-window-v2.md`
- `doc/window-chrome.md`
- README quick start section
