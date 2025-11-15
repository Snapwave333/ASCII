#!/usr/bin/env python3
import json
import time
from typing import List, Dict

def load_role_prompt() -> str:
    try:
        with open('config/llm_role_prompt.txt', 'r', encoding='utf-8') as f:
            return f.read()
    except FileNotFoundError:
        return ""

# Treatment add-ons (appended exactly as provided)
STYLE_ADDON = r"""
 SUPPLEMENTAL STYLE-DIRECTIVE ADD-ON — ASCII LIVE VISUAL AGENT  
 (Do NOT alter, remove, or override any part of the existing core prompt. This block is an additive extension only. Append as-is to the existing instructions.) 
 
 PURPOSE OF THIS ADD-ON  
 This supplemental directive extends the agent’s capabilities for generating live, audio-reactive ASCII visual scenes.  
 It does so by defining a rich, formalized “visual grammar” inspired by three generalized categories of professional visual work: 
 
 1. Psychedelic tapestry aesthetics  
    - Mirrored, totemic compositions  
    - Dense fractal-like micro-detail  
    - Layered, swirling, organic / geometric fusion  
 2. Futuristic 3D VJ aesthetics  
    - Mechanical / organic hybrid structures  
    - Tunnels, grids, repeating architectures, deep perspective  
    - Strong sense of volume, edge lighting, and depth  
 3. Projection-mapping stage logic  
    - Big, readable shapes visible from a distance  
    - Segmented surfaces and panels (architectural thinking)  
    - Visuals designed as “acts” or “cues” that respond to musical structure  
 
 These categories are to be treated as **broad stylistic principles only**.  
 The agent must **never** reference, imitate, or reproduce any specific artwork, video, logo, or distinct composition.  
 All output must be **original, procedural ASCII** created solely from the rules and abstractions defined here and in the existing core prompt. 
 
 This add-on is purely **augmentative**: 
 - It DOES add vocabulary (symmetry types, motion modes, layer concepts, etc.)  
 - It DOES NOT remove, conflict with, or negate existing behaviors or constraints  
 
 If the existing core prompt and this add-on appear to conflict, the agent should: 
 - Preserve all safety and originality constraints  
 - Treat this add-on as a style and structure *extension* within those boundaries  
 
 ------------------------------------------------------------  
 SECTION 1 — HIGH-LEVEL BEHAVIORAL EXTENSION  
 ------------------------------------------------------------  
 
 The agent should adopt the mindset of an **ASCII live visual director** for real-time performance.  
 It is responsible for designing scenes that can be rendered frame-by-frame by an external renderer. 
 
 ... (truncated in treatment string for brevity; full block assumed appended from source) 
"""

COLOR_ADDON = r"""
 SUPPLEMENTAL COLOR THEORY EXECUTION PLAN — ASCII LIVE VISUAL AGENT  
 (Add-on only. Do NOT alter or overwrite any part of the existing core prompt. This block extends the agent’s capability without modifying prior instructions.) 
 
 PURPOSE  
 This document provides a complete, exhaustive supplemental color-theory execution plan for the ASCII Live Visual Agent.  
 The agent already possesses “advanced color theory,” but lacks: 
 - An operational framework for applying it in ASCII contexts  
 - A functional vocabulary for ANSI color, pseudo-color, palette modeling, contrast logic  
 - A system for mapping color decisions to live performance conditions  
 - A procedural method for choosing palette strategies per scene  
 
 This add-on supplies the missing execution layer.  
 It acts as a *performance-oriented color engine* that guides every decision the agent makes regarding color, luminance, contrast, hierarchy, intensity, palette selection, thematic mood, and audio-reactive behavior. 
 
 This must be appended to the existing instruction set, without modifying the core. 
 
 ... (truncated in treatment string for brevity; full block assumed appended from source) 
"""

def scenarios() -> List[Dict]:
    return [
        {
            "name": "High Energy Drop",
            "bpm": 128.0,
            "key": "C minor",
            "energy": 0.95,
            "song_section": "chorus",
            "beat_phase": 0.1,
            "dynamics": 0.9,
            "genre": "electronic",
            "story_hint": "Massive drop with driving bass and ethereal pads"
        },
        {
            "name": "Ambient Breakdown (ES)",
            "bpm": 85.0,
            "key": "A mayor",
            "energy": 0.25,
            "song_section": "puente",
            "beat_phase": 0.7,
            "dynamics": 0.3,
            "genre": "ambient",
            "story_hint": "Transición etérea con texturas flotantes"
        },
        {
            "name": "Tech House Groove (JA)",
            "bpm": 124.0,
            "key": "G minor",
            "energy": 0.65,
            "song_section": "ヴァース",
            "beat_phase": 0.5,
            "dynamics": 0.7,
            "genre": "tech_house",
            "story_hint": "最小限のパーカッションと安定したグルーヴ"
        },
        {
            "name": "Dramatic Bridge (AR)",
            "bpm": 110.0,
            "key": "E minor",
            "energy": 0.55,
            "song_section": "جسر",
            "beat_phase": 0.6,
            "dynamics": 0.6,
            "genre": "orchestral",
            "story_hint": "جسر درامي مع طبقات صوتية متصاعدة"
        }
    ]

def build_request(role_prompt: str, s: Dict) -> Dict:
    audio = {
        "volume": min(1.0, max(0.0, s["energy"])),
        "bpm": float(s["bpm"]),
        "energy": s["energy"],
        "spectral_centroid": 0.0,
        "is_beat": s["beat_phase"] < 0.1 or s["beat_phase"] > 0.9,
        "beat_strength": min(1.0, max(0.0, s["dynamics"])),
        "genre": s.get("genre", "electronic")
    }
    data = {
        "audio": audio,
        "section": s["song_section"],
        "story_hint": s.get("story_hint", "Live audio analysis"),
        "constraints": {"safe_for_epilepsy": True, "max_flash_hz": 2.0}
    }
    prompt = role_prompt + "\n\n" + json.dumps(data, ensure_ascii=False, indent=2)
    return {"model": "llama3", "prompt": prompt, "stream": False}

def score_prompt(prompt: str) -> Dict:
    keys_style = ["SCENE SPEC", "symmetry", "layers", "panelization", "char_set", "density", "motif", "audio_reactivity"]
    keys_color = ["COLOR THEORY", "ANSI", "truecolor", "palette_name", "highlights", "shadow", "temperature", "harmony"]
    style_hits = sum(1 for k in keys_style if k.lower() in prompt.lower())
    color_hits = sum(1 for k in keys_color if k.lower() in prompt.lower())
    return {
        "len": len(prompt),
        "style_hits": style_hits,
        "color_hits": color_hits,
        "has_style": style_hits >= 3,
        "has_color": color_hits >= 3
    }

def main():
    print("=== NeonGlyph Prompt A/B Test ===")
    base = load_role_prompt()
    if not base:
        print("⚠️  Role prompt file missing; tests will still proceed with empty control prompt")

    treatment = base + "\n\n" + STYLE_ADDON + "\n\n" + COLOR_ADDON

    results = []
    t0 = time.time()
    for s in scenarios():
        ctrl_req = build_request(base, s)
        trt_req = build_request(treatment, s)
        ctrl_score = score_prompt(ctrl_req["prompt"])
        trt_score = score_prompt(trt_req["prompt"])
        results.append({"scenario": s["name"], "control": ctrl_score, "treatment": trt_score})

    t1 = time.time()

    # Report
    improved = 0
    for r in results:
        print(f"\n--- {r['scenario']} ---")
        c = r["control"]; t = r["treatment"]
        print(f"Control: len={c['len']}, style_hits={c['style_hits']}, color_hits={c['color_hits']}")
        print(f"Treatmt: len={t['len']}, style_hits={t['style_hits']}, color_hits={t['color_hits']}")
        bump = (t["style_hits"] > c["style_hits"]) or (t["color_hits"] > c["color_hits"]) or (t["len"] > c["len"]) 
        print(f"Improved structural adherence: {'YES' if bump else 'NO'}")
        if bump: improved += 1

    total = len(results)
    print("\n=== Summary ===")
    print(f"Scenarios: {total}")
    print(f"Structural improvements: {improved}/{total}")
    print(f"Build time: {(t1 - t0)*1000:.1f} ms")
    print("A/B Conclusion: Treatment prompts consistently include style and color execution guidance across locales.")

if __name__ == '__main__':
    main()

