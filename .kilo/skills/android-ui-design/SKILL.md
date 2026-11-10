---
name: android-ui-design
description: Material Design, mobile UI, dark/light themes, adaptive layouts, accessibility
---

# Android UI Design Skill

## Purpose
Design and implement modern, polished Android UI/UX following Material Design principles with dark/light themes, adaptive layouts, and accessibility compliance.

## Activation Conditions
- Modifying UI layouts, themes, styles
- Implementing dark/light theme support
- Creating adaptive/responsive layouts
- Adding animations and transitions
- Accessibility improvements (WCAG AA)
- Material 3 migration evaluation

## Technical Reference

### Current Theme System
- Base: `Theme.TermuxApp.DayNight.DarkActionBar` (AppCompat DayNight)
- Dark theme: `res/values-night/themes.xml`
- Light theme: `res/values/themes.xml`
- Colors: `res/values/colors.xml`, `res/values-night/colors.xml`
- Styles: `res/values/styles.xml`

### Key Layouts
- `activity_termux.xml` — main terminal activity
- `activity_settings.xml` — settings activity
- `item_terminal_sessions_list.xml` — session list item
- `view_terminal_toolbar_extra_keys.xml` — extra keys toolbar
- `view_terminal_toolbar_text_input.xml` — text input toolbar

### Drawables
- Session indicators: `current_session.xml`, `session_background_selected.xml`
- Icons: `ic_new_session.xml`, `ic_settings.xml`, `ic_service_notification.xml`
- Ripple effects: `session_ripple.xml`, `session_ripple_black.xml`

### Navigation Destinations (Proposed)
1. Terminal — existing terminal session view
2. Workspaces — project/workspace directory management
3. Projects — recent/favorite projects with git detection
4. Commands — command snippets and history
5. AI Tools — coding assistant detection and launching
6. Device — device diagnostics
7. Settings — existing + workstation preferences

### Material 3 Evaluation
- Current: Material Components 1.12.0 (`com.google.android.material:material:1.12.0`)
- Target: Material 3 (Material You) without full rewrite
- Dynamic colors (Android 12+)
- Updated component library

### Accessibility Requirements
- WCAG AA contrast ratios
- 48dp minimum touch targets
- Scalable fonts (sp units)
- Content descriptions for icons
- Focus navigation order

## Required Tools
- Android Studio / Layout Inspector
- Accessibility Scanner
- Theme preview in IDE

## Reusable Workflow
1. Design in XML layouts with Material components
2. Define colors in `colors.xml` (light) and `colors.xml` (night)
3. Update themes in `themes.xml` / `themes.xml (night)`
4. Test on multiple screen sizes (phone, tablet, foldable)
5. Test dark/light theme switching
6. Run Accessibility Scanner
7. Verify 48dp touch targets

## Verification Procedure
```bash
./gradlew :app:assembleDebug
./gradlew lintDebug
# Manual: test on device, theme switch, accessibility
```

## Common Failure Modes
- Hardcoded colors breaking dark theme
- Fixed dp sizes breaking on different screens
- Missing content descriptions
- Touch targets < 48dp
- Theme flicker on rotation
- Material 3 migration breaking existing UI
