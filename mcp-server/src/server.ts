import { Server } from "@modelcontextprotocol/sdk/server/index.js";
import { StdioServerTransport } from "@modelcontextprotocol/sdk/server/stdio.js";
import { CallToolRequestSchema, ListToolsRequestSchema } from "@modelcontextprotocol/sdk/types.js";
import axios from "axios";
import * as cheerio from "cheerio";

interface AnimationItem {
  title: string;
  slug: string;
  url: string;
  description?: string;
  tags?: string[];
}

interface AnimationMetadata {
  title: string;
  slug: string;
  url: string;
  description?: string;
  tags?: string[];
  externalLinks?: {
    giphyUrl?: string;
    originalSourceUrl?: string;
  };
  motionPattern?: string;
  complexity?: "simple" | "medium" | "complex";
}

const BASE_URL = "https://ascii.co.uk/animated";

async function scrapeList(letter?: string): Promise<AnimationItem[]> {
  try {
    const url = letter ? `${BASE_URL}/${letter}` : BASE_URL;
    const response = await axios.get(url, {
      headers: {
        'User-Agent': 'Mozilla/5.0 (compatible; ASCII-Animated-MCP/0.1.0)'
      }
    });
    
    const $ = cheerio.load(response.data);
    const items: AnimationItem[] = [];
    
    // Look for animation entries on the page
    $('a[href*="/animated/"]').each((i, elem) => {
      const $elem = $(elem);
      const href = $elem.attr('href');
      const title = $elem.text().trim();
      
      if (href && title && href.includes('/animated/') && !href.endsWith('/animated/')) {
        const slug = href.split('/animated/')[1]?.replace(/\/$/, '') || '';
        if (slug && slug !== letter) {
          items.push({
            title,
            slug,
            url: `https://ascii.co.uk${href}`,
            description: $elem.attr('title') || undefined
          });
        }
      }
    });
    
    // Remove duplicates
    const uniqueItems = items.filter((item, index, self) => 
      index === self.findIndex((t) => t.slug === item.slug)
    );
    
    return uniqueItems.slice(0, 50); // Limit to prevent overwhelming responses
  } catch (error) {
    console.error("Error scraping animation list:", error);
    return [];
  }
}

async function scrapeMetadata(slug: string): Promise<AnimationMetadata> {
  try {
    const url = `${BASE_URL}/${slug}`;
    const response = await axios.get(url, {
      headers: {
        'User-Agent': 'Mozilla/5.0 (compatible; ASCII-Animated-MCP/0.1.0)'
      }
    });
    
    const $ = cheerio.load(response.data);
    
    // Extract metadata from the page
    const title = $('h1, .title, .animation-title').first().text().trim() || slug;
    const description = $('.description, .meta-description, p').first().text().trim() || undefined;
    
    // Look for external links
    const externalLinks: AnimationMetadata['externalLinks'] = {};
    $('a[href*="giphy.com"]').each((i, elem) => {
      const href = $(elem).attr('href');
      if (href) externalLinks.giphyUrl = href;
    });
    
    // Determine complexity based on title/description
    const complexity = determineComplexity(title + ' ' + (description || ''));
    const motionPattern = determineMotionPattern(title + ' ' + (description || ''));
    
    return {
      title,
      slug,
      url,
      description,
      complexity,
      motionPattern,
      externalLinks: Object.keys(externalLinks).length > 0 ? externalLinks : undefined
    };
  } catch (error) {
    console.error(`Error scraping metadata for ${slug}:`, error);
    return {
      title: slug,
      slug,
      url: `${BASE_URL}/${slug}`,
      complexity: "medium",
      motionPattern: "unknown"
    };
  }
}

function determineComplexity(text: string): "simple" | "medium" | "complex" {
  const lowerText = text.toLowerCase();
  
  if (lowerText.includes('simple') || lowerText.includes('basic') || lowerText.includes('minimal')) {
    return "simple";
  } else if (lowerText.includes('complex') || lowerText.includes('detailed') || lowerText.includes('advanced')) {
    return "complex";
  }
  
  return "medium";
}

function determineMotionPattern(text: string): string {
  const lowerText = text.toLowerCase();
  
  if (lowerText.includes('rotate') || lowerText.includes('spin')) {
    return "rotation";
  } else if (lowerText.includes('wave') || lowerText.includes('pulse')) {
    return "wave";
  } else if (lowerText.includes('walk') || lowerText.includes('run')) {
    return "locomotion";
  } else if (lowerText.includes('explode') || lowerText.includes('burst')) {
    return "explosion";
  } else if (lowerText.includes('morph') || lowerText.includes('transform')) {
    return "morphing";
  }
  
  return "cyclic";
}

// Tool definitions
const listAnimationsTool = {
  name: "list_animations",
  description: "Get a list of animated ASCII items from ascii.co.uk/animated for inspiration (reference only, do not copy)",
  inputSchema: {
    type: "object",
    properties: {
      letter: {
        type: "string",
        description: "Optional letter to filter animations (a-z)"
      }
    },
    required: [],
    additionalProperties: false
  }
};

const getAnimationMetadataTool = {
  name: "get_animation_metadata",
  description: "Get metadata for a specific animated ASCII item (reference only, do not copy)",
  inputSchema: {
    type: "object",
    properties: {
      slug: {
        type: "string",
        description: "The slug of the animation (from list_animations)"
      }
    },
    required: ["slug"],
    additionalProperties: false
  }
};

const randomAnimationTool = {
  name: "random_animation",
  description: "Pick a random animated ASCII item from ascii.co.uk/animated for inspiration (reference only, do not copy)",
  inputSchema: {
    type: "object",
    properties: {},
    required: [],
    additionalProperties: false
  }
};

// Create server
const server = new Server(
  { name: "ascii-animated", version: "0.1.0" }
);

// Handle tool calls
server.setRequestHandler(CallToolRequestSchema, async (req) => {
  const { name, arguments: args } = req.params;

  if (name === "list_animations") {
    const letter = (args?.letter as string | undefined) || undefined;
    const items = await scrapeList(letter);
    return {
      content: [{ type: "text", text: JSON.stringify(items, null, 2) }]
    };
  }

  if (name === "random_animation") {
    const items = await scrapeList();
    if (!items.length) {
      throw new Error("No animations found");
    }
    const pick = items[Math.floor(Math.random() * items.length)];
    return {
      content: [{ type: "text", text: JSON.stringify(pick, null, 2) }]
    };
  }

  if (name === "get_animation_metadata") {
    const slug = args?.slug as string | undefined;
    if (!slug) throw new Error("slug is required");
    const meta = await scrapeMetadata(slug);
    return {
      content: [{ type: "text", text: JSON.stringify(meta, null, 2) }]
    };
  }

  throw new Error(`Unknown tool: ${name}`);
});

// Handle tool listing
server.setRequestHandler(ListToolsRequestSchema, async () => {
  return {
    tools: [listAnimationsTool, getAnimationMetadataTool, randomAnimationTool]
  };
});

// Start server
async function main() {
  const transport = new StdioServerTransport();
  await server.connect(transport);
  console.error("ASCII Animated MCP server running on stdio");
}

main().catch((error) => {
  console.error("Fatal error in main():", error);
  process.exit(1);
});