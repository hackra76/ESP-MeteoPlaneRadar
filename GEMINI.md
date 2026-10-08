# Workspace Guidelines

## 1. Execution & Build Protocol
- **Language**: Always communicate in English with the user.
- **Auto-Accept Changes**: Automatically implement, edit, and apply code changes directly without pausing to ask for approval. Only pause with a single-line query if an explicit architectural tradeoff strictly blocks execution.
- **Verification**: Proactively run `pio run` to verify and ensure zero compile regressions.
- **Flashing Firmware**: Always use `$env:PYTHONIOENCODING="utf8"; pio run -t upload` when flashing the firmware to prevent Python UnicodeEncodeError crashes in the Windows console.
- **Release Documentation Protocol**: Whenever asked to make a new release, always update the README files (`README.md`, `README_EN.md`, `README_SK.md`) and wiki pages first to keep them completely accurate before publishing the release.
- **Local Memory Persistence**: Mandatory protocol: after every change, fix, or implementation, immediately store the exact architectural and behavioral changes in local memory (local-memory MCP), and continuously consult the knowledge graph as the foundation for all future activities.

## 2. Token Efficiency & Reasoning
- **Extreme Conciseness**: Skip polite greetings, conversational transitions, summaries, and restatements of the prompt.
- **No Self-Evident Commentary**: Do not explain code that speaks for itself. Only provide commentary if a non-obvious design trade-off, edge case, or bug workaround is involved.
- **Internal Analysis**: Prioritize deep internal chain-of-thought, static analysis, and edge-case verification before executing changes. Output conclusions and action plans directly without narrating step-by-step thinking.
- **Native Inspection**: Rely on native LSP/workspace tools to inspect definitions, signatures, and types rather than asking the user to paste context.

## 3. Output Format
- **Code Changes**: Apply directly to files on disk. In chat, output only clickable file links with concise bullet points of modified functions/symbols (do not reprint diffs or file boilerplate).
- **Execution Steps**: Bulleted commands and verification status only.
- **Questions**: Single-line queries only when strictly blocking.
