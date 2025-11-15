# ASCII Animated MCP Server

**Document Type**: Service Overview
**Version**: 1.1.0
**Last Updated**: 2025-11-14
**Status**: Approved
**Author**: NeonGlyph Development Team

This MCP server provides reference-only access to ascii.co.uk/animated for inspiration in creating new ASCII animations. It follows strict guidelines to avoid copying content directly.

## Features

- **Reference-only inspiration**: Get ideas from existing ASCII animations without copying them
- **Three main tools**:
  - `list_animations`: Browse available animations by letter
  - `random_animation`: Get a random animation for inspiration
  - `get_animation_metadata`: Get detailed information about a specific animation

## Usage Guidelines

### For LLMs using this server:

1. **Use these tools ONLY as stylistic reference and inspiration**
2. **Do NOT reproduce or closely trace any ASCII art from the site**
3. **Do NOT output long verbatim copies of ASCII art from the site**
4. **Extract high-level patterns instead**: shapes, motion type, rhythm, camera ideas
5. **When generating new ASCII animations, your output must be clearly distinct**:
   - New compositions
   - New character patterns  
   - New timing and transitions

### Example workflow for NeonGlyph integration:

```typescript
// 1. Get inspiration
const inspiration = await mcp.call("random_animation");

// 2. Analyze the metadata
const metadata = await mcp.call("get_animation_metadata", { 
  slug: inspiration.slug 
});

// 3. Extract high-level concepts:
// - Motion pattern (rotation, wave, locomotion, etc.)
// - Complexity level (simple/medium/complex)
// - Visual style description
// - Timing/rhythm ideas

// 4. Create NEW ASCII animation based on inspiration:
// - Different character sets
// - Different compositions
// - Different motion patterns
// - Original timing
```

## Installation

```bash
cd mcp-server
npm install
npm run build
```

## Running

```bash
npm start
```

Or for development:

```bash
npm run dev
```

## Integration with NeonGlyph

This server is designed to integrate with your NeonGlyph VJ application's multi-model pipeline:

- **Qwen (Director)**: Use `random_animation` to get fresh motif ideas
- **Phi/Llama (Stylist)**: Analyze motion patterns and create new variations
- **Gemma (Story)**: Reference animation themes for narrative elements

The server provides only metadata and descriptions - no actual ASCII frames are scraped or returned, ensuring compliance with reference-only usage.

## References

- [Integration Guide](NEONGLYPH_INTEGRATION.md)
- [Agent Prompts](AGENT_PROMPTS.md)
- [Documentation Style Guide](../.trae/documents/DOCUMENTATION_STYLE_GUIDE.md)

## Change History

### Version 1.1.0 (2025-11-14)
- Added metadata header and cross-references
- Clarified compliance language

### Version 1.0.0 (2025-11-06)
- Initial MCP server documentation