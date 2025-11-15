#!/usr/bin/env python3
import json
import os
import time
import sys
import urllib.request
import urllib.error
from typing import List, Dict, Any

def load_role_prompt() -> str:
    try:
        with open('config/llm_role_prompt.txt', 'r', encoding='utf-8') as f:
            return f.read()
    except FileNotFoundError:
        return ""

def load_config() -> Dict[str, Any]:
    try:
        with open('config/default.json', 'r', encoding='utf-8') as f:
            return json.load(f)
    except Exception:
        return {"llm": {"endpoint": "http://localhost:11434", "model": "llama3"}}

STYLE_ADDON = (
" SUPPLEMENTAL STYLE-DIRECTIVE ADD-ON — ASCII LIVE VISUAL AGENT  \n"
" (Do NOT alter, remove, or override any part of the existing core prompt. This block is an additive extension only. Append as-is to the existing instructions.) \n"
)

COLOR_ADDON = (
" SUPPLEMENTAL COLOR THEORY EXECUTION PLAN — ASCII LIVE VISUAL AGENT  \n"
" (Add-on only. Do NOT alter or overwrite any part of the existing core prompt. This block extends the agent’s capability without modifying prior instructions.) \n"
)

def scenarios() -> List[Dict]:
    return [
        {"name": "EN Drop", "bpm": 128.0, "key": "C minor", "energy": 0.95, "song_section": "chorus", "beat_phase": 0.1, "dynamics": 0.9, "genre": "electronic", "story_hint": "Massive drop"},
        {"name": "ES Puente", "bpm": 85.0, "key": "A mayor", "energy": 0.25, "song_section": "puente", "beat_phase": 0.7, "dynamics": 0.3, "genre": "ambient", "story_hint": "Transición etérea"},
        {"name": "JA Verse", "bpm": 124.0, "key": "G minor", "energy": 0.65, "song_section": "ヴァース", "beat_phase": 0.5, "dynamics": 0.7, "genre": "tech_house", "story_hint": "安定したグルーヴ"},
        {"name": "AR Bridge", "bpm": 110.0, "key": "E minor", "energy": 0.55, "song_section": "جسر", "beat_phase": 0.6, "dynamics": 0.6, "genre": "orchestral", "story_hint": "طبقات صوتية متصاعدة"}
    ]

def build_request(role_prompt: str, s: Dict, model: str) -> Dict:
    audio = {
        "volume": min(1.0, max(0.0, s["energy"])),
        "bpm": float(s["bpm"]),
        "energy": s["energy"],
        "spectral_centroid": 0.0,
        "is_beat": s["beat_phase"] < 0.1 or s["beat_phase"] > 0.9,
        "beat_strength": min(1.0, max(0.0, s["dynamics"])),
        "genre": s.get("genre", "electronic")
    }
    data = {"audio": audio, "section": s["song_section"], "story_hint": s.get("story_hint", ""), "constraints": {"safe_for_epilepsy": True, "max_flash_hz": 2.0}}
    prompt = role_prompt + "\n\n" + json.dumps(data, ensure_ascii=False, indent=2)
    return {"model": model, "prompt": prompt, "stream": False}

def post_json(endpoint: str, path: str, body: Dict) -> Any:
    url = endpoint.rstrip('/') + path
    data = json.dumps(body).encode('utf-8')
    req = urllib.request.Request(url, data=data, headers={"Content-Type": "application/json"}, method="POST")
    with urllib.request.urlopen(req, timeout=10) as resp:
        return resp.read().decode('utf-8')

def extract_scene_json(text: str) -> Dict:
    try:
        j = json.loads(text)
        return j if isinstance(j, dict) else {}
    except Exception:
        pass
    start = text.find('{')
    end = text.rfind('}')
    if start != -1 and end != -1 and end > start:
        candidate = text[start:end+1]
        try:
            j = json.loads(candidate)
            return j if isinstance(j, dict) else {}
        except Exception:
            return {}
    return {}

def score_response(resp: str) -> Dict:
    j = extract_scene_json(resp)
    has_canvas = isinstance(j.get("canvas"), dict)
    has_style = isinstance(j.get("style"), dict)
    has_layers = isinstance(j.get("layers"), list) and len(j.get("layers")) > 0
    color = j.get("color")
    has_color = isinstance(color, dict)
    color_mode_ok = has_color and color.get("mode") in ("mono", "ansi", "truecolor")
    color_keys_ok = has_color and all(k in color for k in ("palette_name", "primary", "highlights", "shadow"))
    keys_present = sum(int(x) for x in [has_canvas, has_style, has_layers, has_color, color_mode_ok, color_keys_ok])
    return {"has_canvas": has_canvas, "has_style": has_style, "has_layers": has_layers, "has_color": has_color, "color_mode_ok": color_mode_ok, "color_keys_ok": color_keys_ok, "keys_present": keys_present}

def main():
    cfg = load_config()
    endpoint_default = cfg.get("llm", {}).get("endpoint", "http://localhost:11434")
    endpoint_a = os.environ.get('AB_ENDPOINT_A') or endpoint_default
    endpoint_b = os.environ.get('AB_ENDPOINT_B') or os.environ.get('AB_ENDPOINT') or endpoint_default
    model = cfg.get("llm", {}).get("model", "llama3")
    base = load_role_prompt()
    treatment = base + "\n\n" + STYLE_ADDON + "\n\n" + COLOR_ADDON
    print("=== NeonGlyph LLM A/B Online Test ===")
    results = []
    t0 = time.time()
    for s in scenarios():
        try:
            ctrl_req = build_request(base, s, model)
            trt_req = build_request(treatment, s, model)
            t_ctrl_0 = time.time()
            ctrl_raw = post_json(endpoint_a, "/api/generate", ctrl_req)
            t_ctrl_1 = time.time()
            t_trt_0 = time.time()
            trt_raw = post_json(endpoint_b, "/api/generate", trt_req)
            t_trt_1 = time.time()
            ctrl_resp = json.loads(ctrl_raw)
            trt_resp = json.loads(trt_raw)
            ctrl_txt = ctrl_resp.get("response", ctrl_raw)
            trt_txt = trt_resp.get("response", trt_raw)
            ctrl_score = score_response(ctrl_txt)
            trt_score = score_response(trt_txt)
            ctrl_score["latency_ms_client"] = int((t_ctrl_1 - t_ctrl_0) * 1000)
            trt_score["latency_ms_client"] = int((t_trt_1 - t_trt_0) * 1000)
            improved = trt_score["keys_present"] > ctrl_score["keys_present"]
            results.append({"scenario": s["name"], "control": ctrl_score, "treatment": trt_score, "improved": improved})
        except urllib.error.URLError as e:
            print("Network error", e)
            results.append({"scenario": s["name"], "error": "network"})
        except Exception as e:
            print("Error", e)
            results.append({"scenario": s["name"], "error": "runtime"})
    t1 = time.time()
    total = len(results)
    improved = sum(1 for r in results if r.get("improved"))
    # Persist results
    try:
        with open('ab_results.json', 'w', encoding='utf-8') as f:
            json.dump({"endpoint_a": endpoint_a, "endpoint_b": endpoint_b, "model": model, "ts": int(time.time()), "results": results}, f, ensure_ascii=False, indent=2)
    except Exception as e:
        print("Failed to write ab_results.json", e)

    print("\n=== Summary ===")
    print(f"Scenarios: {total}")
    print(f"Improvements: {improved}/{total}")
    print(f"Duration: {(t1 - t0)*1000:.1f} ms")
    for r in results:
        if "error" in r:
            print(f"{r['scenario']}: ERROR {r['error']}")
        else:
            c = r["control"]; t = r["treatment"]
            print(f"{r['scenario']}: C={c['keys_present']}({c.get('latency_ms_client', '-') }ms) T={t['keys_present']}({t.get('latency_ms_client', '-') }ms) {'IMP' if r['improved'] else ''}")
    return 0

if __name__ == '__main__':
    sys.exit(main())
