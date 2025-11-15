# ASCII Animated MCP Integration Guide for NeonGlyph

**Document Type**: Integration Guide
**Version**: 1.1.0
**Last Updated**: 2025-11-14
**Status**: Approved
**Author**: NeonGlyph Development Team

## Overview

This MCP server provides reference-only inspiration from ascii.co.uk/animated for your NeonGlyph VJ application. It enables your LLM agents to get creative ideas without copying any actual ASCII art content.

## Architecture Integration

### Multi-Model Pipeline Integration

```
Gemma (Story) → Qwen (Director) → Phi/Llama (Stylist)
                              ↓
                    ASCII Animated MCP Server
                              ↓
                    NeonGlyph Visual Engine
```

### How Each Agent Uses the Server

1. **Qwen (Director)**: Uses `random_animation` to get fresh motif ideas
   - Extracts motion patterns and complexity levels
   - Creates new `DirectorCommand` objects based on inspiration
   - Never copies actual ASCII frames

2. **Phi/Llama (Stylist)**: Analyzes motion concepts and creates variations
   - Uses metadata to understand visual themes
   - Generates new ASCII palettes and character sets
   - Applies different artistic interpretations

3. **Gemma (Story)**: References animation themes for narrative elements
   - Uses animation titles and descriptions for story beats
   - Creates thematic connections between scenes

## Implementation Examples

### Basic Usage in Your Agents

```typescript
// In Qwen Director Agent
async function generateSceneDirection() {
  const inspiration = await mcp.call("random_animation");
  const metadata = await mcp.call("get_animation_metadata", {
    slug: inspiration.slug
  });
  
  // Extract concepts, not content
  const concept = {
    motionPattern: metadata.motionPattern,
    complexity: metadata.complexity,
    theme: extractTheme(metadata.title),
    timing: getTimingFromPattern(metadata.motionPattern)
  };
  
  // Create NEW director command
  return new DirectorCommand({
    motif_id: `original_${Date.now()}`,
    camera_motion: getCameraMotion(concept.motionPattern),
    visual_state: createVisualState(concept),
    // ... other properties
  });
}
```

### Creating Original ASCII Patterns

```typescript
function createAsciiPalette(motionPattern: string, complexity: string): string[] {
  // Never use actual ASCII from reference site
  const basePatterns = {
    rotation: ['◐', '◑', '◒', '◓', '●', '○'],
    wave: ['≈', '∿', '〰', '~', '∼', '≋'],
    pulse: ['•', '·', '◦', '°', '∙', '⋅'],
    spiral: ['◧', '◨', '◩', '◪', '◫', '◈']
  };
  
  // Add complexity variations
  const complexPatterns = complexity === 'complex' 
    ? ['█', '▓', '▒', '░', '▄', '▀']
    : [];
    
  return [...basePatterns[motionPattern] || basePatterns.pulse, ...complexPatterns];
}
```

### Motion Pattern Mapping

```typescript
const motionToCamera = {
  rotation: "orbit_around_center",
  wave: "sine_wave_pan",
  explosion: "radial_expand",
  morphing: "smooth_transition",
  locomotion: "tracking_shot",
  cyclic: "looping_sequence"
};

function getCameraMotion(pattern: string): string {
  return motionToCamera[pattern] || "static_frame";
}
```

## Safety Guidelines

### For Your LLM Agents

Add this to your agent system prompts:

```
You have access to an MCP server called "ascii-animated" that provides inspiration from ascii.co.uk/animated.

CRITICAL RULES:
- Use these tools ONLY as stylistic reference and inspiration
- Do NOT reproduce or closely trace any ASCII art from the site  
- Do NOT output long verbatim copies of ASCII art from the site
- Extract high-level patterns: shapes, motion type, rhythm, camera ideas
- When generating new ASCII animations, your output must be clearly distinct:
  * New compositions using different character sets
  * New timing and transitions
  * Original artistic interpretation

VIOLATION CONSEQUENCES:
- Direct copying will result in immediate termination
- Close tracing will be rejected by the novelty gate
- All outputs are checked for originality
```

### Novelty Gate Integration

```typescript
// In your NeonGlyph engine
function checkOriginality(newMotif: VisualState, referenceInspiration: any): boolean {
  // Ensure new motif is substantially different from inspiration
  const similarity = calculateSimilarity(newMotif, referenceInspiration);
  return similarity < 0.3; // Must be 70% different
}
```

## Configuration

### MCP Client Setup

Add to your project's MCP configuration:

```json
{
  "mcpServers": {
    "ascii-animated": {
      "command": "node",
      "args": ["dist/server.js"],
      "cwd": "./mcp-server"
    }
  }
}
```

### Environment Variables

```bash
# Optional: Configure scraping behavior
ASCII_MCP_USER_AGENT="NeonGlyph-AI-Inspiration/1.0"
ASCII_MCP_RATE_LIMIT=1000  # ms between requests
ASCII_MCP_CACHE_TTL=3600   # seconds to cache results
```

## Testing Integration

```typescript
// Test script for integration
async function testIntegration() {
  const service = new AsciiInspirationService();
  await service.connect();
  
  // Test all tools
  const list = await service.mcp.call("list_animations", { letter: "a" });
  const random = await service.mcp.call("random_animation");
  const metadata = await service.mcp.call("get_animation_metadata", {
    slug: random.slug
  });
  
  console.log("Integration test passed!");
  console.log(`Found ${list.length} animations`);
  console.log(`Random pick: ${random.title}`);
  console.log(`Motion pattern: ${metadata.motionPattern}`);
  
  await service.disconnect();
}
```

## Performance Considerations

- **Caching**: Results are cached to minimize external requests
- **Rate Limiting**: Built-in delays to respect the source website
- **Error Handling**: Graceful fallbacks if site is unavailable
- **Timeout**: 5-second timeout on all external requests

## Legal Compliance

This server is designed for:
- ✓ Inspiration and creative reference
- ✓ Learning motion patterns and concepts
- ✓ Understanding animation themes
- ✓ Generating original content

This server prevents:
- ✗ Direct copying of ASCII art
- ✗ Reproducing copyrighted sequences
- ✗ Close tracing of existing works
- ✗ Mass downloading of content

## Next Steps

1. **Integrate with Qwen Director**: Add MCP calls to your director agent
2. **Test Novelty Gate**: Ensure originality checking works
3. **Train Agents**: Update system prompts with usage guidelines
4. **Monitor Usage**: Track inspiration sources for attribution
5. **Expand Palette**: Add more original ASCII character sets
6. **Optional Cloud Metrics**: Enable Supabase logging for audio analysis metrics. See `docs/Supabase.md` for environment variables, schema, and build instructions.

## References

- [MCP Server Overview](README.md)
- [Agent Prompts](AGENT_PROMPTS.md)
- [Documentation Style Guide](../.trae/documents/DOCUMENTATION_STYLE_GUIDE.md)
- [Documentation Index](../.trae/documents/DOCUMENTATION_INDEX.md)

## Change History

### Version 1.1.0 (2025-11-14)
- Added metadata header and references
- Clarified agent integration examples

### Version 1.0.0 (2025-11-06)
- Initial integration guide

The server is now ready to provide creative inspiration while maintaining originality and respecting source content.
