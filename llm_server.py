#!/usr/bin/env python3
import json
import time
import logging
from http.server import HTTPServer, BaseHTTPRequestHandler
import os

LOG_FILE = 'llm_server.log'
logging.basicConfig(filename=LOG_FILE, level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')

def build_control_response(prompt: str) -> str:
    # Minimal, non-structured response (no scene spec/color sections)
    return (
        "ASCII ANIMATION DIRECTIVE\n\n"
        "VISUAL SUMMARY: Ambient flow with minimal structure.\n"
        "LAYERS: background only.\n"
        "NOTE: No structured scene spec or color model included."
    )

def build_treatment_response(prompt: str) -> str:
    # Structured SCENE SPEC with color section for improved adherence
    spec = {
        "canvas": {"width": 120, "height": 48},
        "style": {
            "symmetry": "vertical_mirror",
            "panelization": {"enabled": True, "columns": 3, "rows": 2},
            "palette_name": "NeonMech"
        },
        "layers": [
            {"name": "background", "char_set": " .:-", "density": 0.08, "motif": "slow_waves", "motion": "slow_scroll", "audio_reactivity": "highs"},
            {"name": "midground",  "char_set": "=+*xo", "density": 0.22, "motif": "tunnels",    "motion": "tunnel_zoom",  "audio_reactivity": "mids"},
            {"name": "foreground", "char_set": "#%@&",  "density": 0.35, "motif": "totem",      "motion": "breathing_totem", "audio_reactivity": "bass"}
        ],
        "focus": {"has_central_totem": True, "totem_width_fraction": 0.15, "totem_description": "Abstract energy column"},
        "motion": {"global_mode": "tunnel_zoom", "phase_0_1": 0.3, "beat_sync": {"bpm": 128, "on_beat_emphasis": True}},
        "audio": {"bass_level": 0.6, "mids_level": 0.4, "highs_level": 0.2},
        "color": {
            "mode": "truecolor",
            "palette_name": "NeonMech",
            "primary": [16, 240, 255],
            "secondary": [[120, 200, 255]],
            "accents": [[255, 80, 180], [255, 220, 40]],
            "shadow": [12, 20, 28],
            "highlights": [255, 255, 255],
            "temperature": "mixed",
            "harmony": "complementary",
            "behavior": {
                "bass_reactivity": "foreground highlight pulse",
                "mids_reactivity": "midground color cycling",
                "highs_reactivity": "background shimmer",
                "beat_emphasis": "accent flash on-beat",
                "transition_strategy": "palette morph across sections"
            }
        }
    }
    return json.dumps(spec)

class Handler(BaseHTTPRequestHandler):
    server_version = "NeonGlyphLLM/1.0"

    def _send_json(self, code: int, payload: dict):
        body = json.dumps(payload).encode('utf-8')
        self.send_response(code)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, fmt, *args):
        logging.info("%s - - %s", self.client_address[0], fmt % args)

    def do_POST(self):
        t_start = time.time()
        try:
            if self.path != "/api/generate":
                self._send_json(404, {"error": "not_found"})
                return
            length = int(self.headers.get('Content-Length', '0'))
            data = self.rfile.read(length).decode('utf-8') if length > 0 else ""
            req = json.loads(data)
            model = req.get("model", "")
            prompt = req.get("prompt", "")
            stream = req.get("stream", False)
            if not isinstance(model, str) or not isinstance(prompt, str):
                self._send_json(400, {"error": "invalid_request"})
                return
            # Treatment detection: presence of both add-on headers
            is_treatment = ("SUPPLEMENTAL STYLE-DIRECTIVE ADD-ON" in prompt) or ("SUPPLEMENTAL COLOR THEORY EXECUTION PLAN" in prompt)
            # Build response content
            try:
                resp_text = build_treatment_response(prompt) if is_treatment else build_control_response(prompt)
            except Exception as e:
                logging.error("response_build_error: %s", e)
                self._send_json(500, {"error": "response_build_error"})
                return
            t_end = time.time()
            self._send_json(200, {"response": resp_text, "latency_ms": int((t_end - t_start) * 1000)})
        except json.JSONDecodeError:
            self._send_json(400, {"error": "invalid_json"})
        except Exception as e:
            logging.exception("server_error: %s", e)
            self._send_json(500, {"error": "server_error"})

def run(addr: str = "0.0.0.0", port: int = 11434):
    logging.info("Starting NeonGlyph LLM server on %s:%d", addr, port)
    httpd = HTTPServer((addr, port), Handler)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        logging.info("Stopping server")
        httpd.server_close()

if __name__ == '__main__':
    addr = os.environ.get('LLM_ADDR', '0.0.0.0')
    try:
        port = int(os.environ.get('LLM_PORT', '11434'))
    except ValueError:
        port = 11434
    run(addr, port)
