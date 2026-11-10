---
name: ai-cli-integration
description: AI coding assistant CLI integration, detection, provider routing, vibe coding workflows
---

# AI CLI Integration Skill

## Purpose
Integrate modern AI coding assistants into Termux — detection, launching, provider routing, and vibe coding workflows.

## Activation Conditions
- Implementing AI tool detection
- Adding coding assistant launch functionality
- Provider routing (OmniRoute, OpenAI-compatible endpoints)
- Vibe coding workflow design
- Authentication/configuration handling

## Technical Reference

### Supported AI Tools
| Tool | CLI Command | Config Location |
|------|-------------|-----------------|
| KiloCode | `kilo` | `~/.config/kilo/` |
| OpenCode | `opencode` | `~/.config/opencode/` |
| Gemini CLI | `gemini` | `~/.config/gemini/` |
| Codex CLI | `codex` | `~/.config/codex/` |
| Qwen CLI | `qwen` | `~/.config/qwen/` |

### Integration Requirements
1. **Detect** installed coding assistants (executable paths, versions)
2. **Identify** configuration existence
3. **Launch** assistants in selected project directory
4. **Display** installation status
5. **Offer** installation instructions
6. **Provide** quick-launch commands
7. **Handle** missing dependencies
8. **Detect** auth/config errors without printing secrets

### Provider Routing
- Support externally configured AI gateways (OmniRoute, OpenAI-compatible)
- Do NOT hardcode a specific gateway or provider
- Do NOT store API keys in APK, source, or project config
- Preserve environment-variable-based authentication
- Support `OPENAI_API_BASE`, `OPENAI_API_KEY`, etc.

### Vibe Coding Workflow
1. User selects a project
2. User launches configured coding assistant
3. Assistant inspects project
4. Assistant creates implementation plan
5. Specialized agents perform delegated work
6. Changes reviewed
7. Tests executed
8. Developer receives clear summary

### AI Tools Must Remain Optional
- Terminal works fully without network access
- Terminal works fully without AI subscription
- No forced AI integration

## Required Tools
- Android file system access (for config detection)
- Process execution (for CLI launching)
- Environment variable reading

## Reusable Workflow
1. Scan common install locations for AI CLIs
2. Execute `--version` or equivalent to verify
3. Check config directory existence
4. Present status UI to user
5. Launch selected CLI with project directory as cwd
6. Handle errors gracefully (missing binary, auth failure, etc.)

## Verification Procedure
```bash
# Manual: test each CLI detection
which kilo opencode gemini codex qwen
# Manual: test launch in project directory
# Manual: verify no secrets printed on auth error
```

## Common Failure Modes
- Hardcoded paths (use `which` and common locations)
- Printing secrets on auth failure
- Assuming specific provider/gateway
- Breaking terminal when AI unavailable
- Network calls blocking UI thread
