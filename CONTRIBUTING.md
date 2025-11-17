# Contributing to NeonGlyph

**Document Type**: Contribution Guide
**Version**: 2.0.1
**Last Updated**: 2025-11-13
**Status**: Approved
**Author**: NeonGlyph Development Team

## Overview
- Use GitHub pull requests for all changes.
- Follow the [Documentation Style Guide](docs/meta/documentation-style-guide.md) for any docs updates.
- Keep code changes consistent with existing patterns and Config structures.
- Refer to the [Documentation Index](docs/meta/documentation-index.md) for cross-references and document locations.

## Workflow
- Fork the repository and create a feature branch.
- Write tests where applicable and run `ctest`.
- Update documentation and change history with version bump.
- Update the project [CHANGELOG](CHANGELOG.md) with notable changes.
- Open a pull request linking related issues.
- Request technical and editorial review.

## Code Standards
- C++20, MSVC v143 or later.
- Use `Result` for API return codes and avoid exceptions in real-time paths.
- Maintain thread-safety boundaries noted in headers.

## Documentation Standards
- Apply headers with metadata (type, version, last updated, status, author).
- Ensure cross-references resolve and anchors exist.
- Add change history entries per update.
- Validate links against the Documentation Index.

## Commit Message Format
- Use imperative mood: `Fix`, `Add`, `Update`.
- Reference issue IDs when applicable.

## Change History
 - Version 2.0.2 (2025-11-14): Add links to Style Guide, Index, and CHANGELOG.
 - Version 2.0.1 (2025-11-13): Initial contribution guide.

