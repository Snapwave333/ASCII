"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const index_js_1 = require("@modelcontextprotocol/sdk/server/index.js");
const stdio_js_1 = require("@modelcontextprotocol/sdk/server/stdio.js");
const types_js_1 = require("@modelcontextprotocol/sdk/types.js");
const axios_1 = __importDefault(require("axios"));
const cheerio = __importStar(require("cheerio"));
const BASE_URL = "https://ascii.co.uk/animated";
async function scrapeList(letter) {
    try {
        const url = letter ? `${BASE_URL}/${letter}` : BASE_URL;
        const response = await axios_1.default.get(url, {
            headers: {
                'User-Agent': 'Mozilla/5.0 (compatible; ASCII-Animated-MCP/0.1.0)'
            }
        });
        const $ = cheerio.load(response.data);
        const items = [];
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
        const uniqueItems = items.filter((item, index, self) => index === self.findIndex((t) => t.slug === item.slug));
        return uniqueItems.slice(0, 50); // Limit to prevent overwhelming responses
    }
    catch (error) {
        console.error("Error scraping animation list:", error);
        return [];
    }
}
async function scrapeMetadata(slug) {
    try {
        const url = `${BASE_URL}/${slug}`;
        const response = await axios_1.default.get(url, {
            headers: {
                'User-Agent': 'Mozilla/5.0 (compatible; ASCII-Animated-MCP/0.1.0)'
            }
        });
        const $ = cheerio.load(response.data);
        // Extract metadata from the page
        const title = $('h1, .title, .animation-title').first().text().trim() || slug;
        const description = $('.description, .meta-description, p').first().text().trim() || undefined;
        // Look for external links
        const externalLinks = {};
        $('a[href*="giphy.com"]').each((i, elem) => {
            const href = $(elem).attr('href');
            if (href)
                externalLinks.giphyUrl = href;
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
    }
    catch (error) {
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
function determineComplexity(text) {
    const lowerText = text.toLowerCase();
    if (lowerText.includes('simple') || lowerText.includes('basic') || lowerText.includes('minimal')) {
        return "simple";
    }
    else if (lowerText.includes('complex') || lowerText.includes('detailed') || lowerText.includes('advanced')) {
        return "complex";
    }
    return "medium";
}
function determineMotionPattern(text) {
    const lowerText = text.toLowerCase();
    if (lowerText.includes('rotate') || lowerText.includes('spin')) {
        return "rotation";
    }
    else if (lowerText.includes('wave') || lowerText.includes('pulse')) {
        return "wave";
    }
    else if (lowerText.includes('walk') || lowerText.includes('run')) {
        return "locomotion";
    }
    else if (lowerText.includes('explode') || lowerText.includes('burst')) {
        return "explosion";
    }
    else if (lowerText.includes('morph') || lowerText.includes('transform')) {
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
const server = new index_js_1.Server({ name: "ascii-animated", version: "0.1.0" });
// Handle tool calls
server.setRequestHandler(types_js_1.CallToolRequestSchema, async (req) => {
    const { name, arguments: args } = req.params;
    if (name === "list_animations") {
        const letter = args?.letter || undefined;
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
        const slug = args?.slug;
        if (!slug)
            throw new Error("slug is required");
        const meta = await scrapeMetadata(slug);
        return {
            content: [{ type: "text", text: JSON.stringify(meta, null, 2) }]
        };
    }
    throw new Error(`Unknown tool: ${name}`);
});
// Handle tool listing
server.setRequestHandler(types_js_1.ListToolsRequestSchema, async () => {
    return {
        tools: [listAnimationsTool, getAnimationMetadataTool, randomAnimationTool]
    };
});
// Start server
async function main() {
    const transport = new stdio_js_1.StdioServerTransport();
    await server.connect(transport);
    console.error("ASCII Animated MCP server running on stdio");
}
main().catch((error) => {
    console.error("Fatal error in main():", error);
    process.exit(1);
});
//# sourceMappingURL=server.js.map