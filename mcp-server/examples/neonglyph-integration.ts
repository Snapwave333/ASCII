import { Client } from "@modelcontextprotocol/sdk/client/index.js";
import { StdioClientTransport } from "@modelcontextprotocol/sdk/client/stdio.js";

/**
 * Example integration of ASCII Animated MCP server with NeonGlyph
 * This shows how to use the server for inspiration without copying content
 */

interface AnimationInspiration {
  title: string;
  motionPattern: string;
  complexity: string;
  description: string;
  concept: string;
}

class AsciiInspirationService {
  private client: Client;
  private transport: StdioClientTransport;

  constructor() {
    this.transport = new StdioClientTransport({
      command: "node",
      args: ["dist/server.js"],
      cwd: "./mcp-server"
    });
    this.client = new Client({}, this.transport);
  }

  async connect() {
    await this.client.connect();
  }

  async disconnect() {
    await this.client.close();
  }

  /**
   * Get inspiration for a new NeonGlyph motif
   * Returns high-level concepts, not actual ASCII art
   */
  async getMotifInspiration(): Promise<AnimationInspiration> {
    try {
      // Get random animation for inspiration
      const result = await this.client.callTool("random_animation", {});
      const animation = JSON.parse(result.content[0].text);
      
      // Get detailed metadata
      const metaResult = await this.client.callTool("get_animation_metadata", {
        slug: animation.slug
      });
      const metadata = JSON.parse(metaResult.content[0].text);

      // Extract high-level concepts (never copy the actual ASCII)
      return {
        title: metadata.title,
        motionPattern: metadata.motionPattern || "cyclic",
        complexity: metadata.complexity || "medium",
        description: metadata.description || "",
        concept: this.generateConcept(metadata)
      };
    } catch (error) {
      console.error("Error getting inspiration:", error);
      throw error;
    }
  }

  /**
   * Generate a high-level concept based on metadata
   * This creates new ideas inspired by, but distinct from, the reference
   */
  private generateConcept(metadata: any): string {
    const patterns = {
      rotation: "Create a flowing rotational motion with geometric primitives",
      wave: "Design an undulating wave pattern using mathematical functions",
      locomotion: "Animate a character or object moving through space",
      explosion: "Create expanding/contracting visual effects",
      morphing: "Transform shapes smoothly between states",
      cyclic: "Design repeating patterns with smooth transitions"
    };

    const complexityHints = {
      simple: "Use minimal elements and clean lines",
      medium: "Balance detail with performance",
      complex: "Layer multiple visual elements"
    };

    const pattern = metadata.motionPattern || "cyclic";
    const complexity = metadata.complexity || "medium";
    
    return `${patterns[pattern as keyof typeof patterns]} - ${complexityHints[complexity as keyof typeof complexityHints]}`;
  }

  /**
   * Example: Create a new NeonGlyph motif based on inspiration
   */
  async createNeonGlyphMotif(): Promise<any> {
    const inspiration = await this.getMotifInspiration();
    
    // Create NEW motif based on inspiration (never copy reference)
    const newMotif = {
      id: `motif_${Date.now()}`,
      name: `Inspired by ${inspiration.title}`,
      concept: inspiration.concept,
      motionPattern: inspiration.motionPattern,
      complexity: inspiration.complexity,
      asciiPalette: this.generateAsciiPalette(inspiration.motionPattern),
      timing: this.generateTiming(inspiration.motionPattern),
      // Add other NeonGlyph-specific properties
      visualState: {
        cameraMotion: this.getCameraMotion(inspiration.motionPattern),
        characterSet: this.getCharacterSet(inspiration.complexity),
        colorScheme: this.getColorScheme(inspiration.motionPattern)
      }
    };

    return newMotif;
  }

  private generateAsciiPalette(pattern: string): string[] {
    const palettes = {
      rotation: ['|', '/', '-', '\\', '+', '×'],
      wave: ['~', '≈', '∿', '〰', '∼', '≋'],
      locomotion: ['>', '<', '^', 'v', '→', '←'],
      explosion: ['*', '✦', '✧', '✩', '✪', '✫'],
      morphing: ['○', '◐', '◑', '◒', '◓', '●'],
      cyclic: ['.', '·', '•', '◦', '°', '∙']
    };
    
    return palettes[pattern as keyof typeof palettes] || palettes.cyclic;
  }

  private generateTiming(pattern: string): number {
    const timings = {
      rotation: 150,
      wave: 200,
      locomotion: 100,
      explosion: 80,
      morphing: 250,
      cyclic: 180
    };
    
    return timings[pattern as keyof typeof timings] || 180;
  }

  private getCameraMotion(pattern: string): string {
    const motions = {
      rotation: "orbit",
      wave: "sine_wave",
      locomotion: "pan_follow",
      explosion: "zoom_pulse",
      morphing: "morph_transition",
      cyclic: "smooth_loop"
    };
    
    return motions[pattern as keyof typeof motions] || "static";
  }

  private getCharacterSet(complexity: string): string {
    const sets = {
      simple: "minimal",
      medium: "extended",
      complex: "unicode"
    };
    
    return sets[complexity as keyof typeof sets] || "basic";
  }

  private getColorScheme(pattern: string): string[] {
    const schemes = {
      rotation: ["#FF0080", "#00FF80", "#8000FF"],
      wave: ["#0080FF", "#80FF00", "#FF8000"],
      locomotion: ["#FF4040", "#40FF40", "#4040FF"],
      explosion: ["#FFFF00", "#FF8000", "#FF0000"],
      morphing: ["#FF00FF", "#00FFFF", "#FFFF00"],
      cyclic: ["#FFFFFF", "#808080", "#404040"]
    };
    
    return schemes[pattern as keyof typeof schemes] || ["#FFFFFF"];
  }
}

// Usage example
async function example() {
  const service = new AsciiInspirationService();
  
  try {
    await service.connect();
    
    // Get inspiration for new motif
    const newMotif = await service.createNeonGlyphMotif();
    
    console.log("New NeonGlyph motif inspired by ASCII animations:");
    console.log(JSON.stringify(newMotif, null, 2));
    
    // The motif is completely new and original, inspired by but not copying
    // the reference ASCII animations
    
  } catch (error) {
    console.error("Error:", error);
  } finally {
    await service.disconnect();
  }
}

// Export for use in NeonGlyph project
export { AsciiInspirationService, type AnimationInspiration };