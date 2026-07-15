# Repository Guidelines

## Project Structure & Module Organization

This repository is a Markdown-based Linux learning notebook. The root currently contains `README.md` and chapter files such as `第一章 应用编程概念.md`. Keep learning notes, command examples, and chapter content in Markdown files at the repository root unless the collection grows large enough to justify subdirectories. Use clear chapter-oriented names, for example `第二章 文件IO.md`, and keep related images or diagrams in an `assets/` directory if added later.

## Build, Test, and Development Commands

There is no build system or application runtime for this repository. Useful local commands are:

- `git status` — review changed and untracked files before committing.
- `git diff` — inspect content changes before opening a pull request.
- `Get-ChildItem` or `ls` — list repository files and confirm chapter organization.

If tooling is added later, document exact commands here and in `README.md`.

## Coding Style & Naming Conventions

Write documents in Markdown with one top-level `#` heading per file. Use `##` and `###` headings to organize concepts, commands, and examples. Prefer fenced code blocks with language hints where useful, such as `bash` or `c`. Keep command examples copyable and include short explanations. Preserve Chinese filenames and content where appropriate; use consistent chapter prefixes for ordered material.

## Testing Guidelines

No automated tests are currently configured. For documentation changes, manually verify that Markdown renders cleanly, headings are ordered logically, and code blocks are complete. When adding scripts or source code examples, include a short note showing how to compile or run them, and add tests only if executable project code is introduced.

## Commit & Pull Request Guidelines

The Git history currently only shows `Initial commit`, so no detailed convention is established. Use concise, imperative commit messages, for example `Add notes on file I/O` or `Fix README encoding`. Pull requests should include a brief summary, list changed files or chapters, mention any related issue, and include screenshots only when visual rendering or diagrams are affected.

## Agent-Specific Instructions

Before creating or editing repository guidance files, check whether `AGENTS.md` already exists and avoid overwriting it unless explicitly requested. Keep future automated edits small, focused, and easy to review.


## Communication Boundaries

- Use only English characters and punctuation.
- Express responses in Chinese.
- Keep wording accurate and concise.
- Avoid repeated statements.
- Avoid empty adjectives and low-value descriptions.
