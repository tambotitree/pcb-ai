#!/usr/bin/env python3
"""Run real GTK/core AI pad and image regression on a disposable board copy."""
from pathlib import Path
import argparse
import base64
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import hashlib
import threading
import os
import shlex
import shutil
import subprocess
import tempfile


class Provider(BaseHTTPRequestHandler):
    """Respond to text with an image request, then validate the PNG continuation."""
    calls = []

    def log_message(self, *args):
        """Keep request headers and payloads out of test logs."""

    def do_POST(self):
        """Only serve a deterministic local chat response."""
        payload = json.loads(self.rfile.read(int(self.headers['Content-Length'])))
        message = payload['messages'][-1]['content']
        self.calls.append(payload)
        if len(self.calls) == 3:
            board = payload['messages'][0]['content'].split('Current PCB layout snapshot:\n', 1)[1]
            number, line = next((number, line) for number, line in enumerate(board.splitlines(True), 1) if 'square' in line)
            proposal = {'reply': 'Round the first pad ends; preserve position and width.',
                        'patches': [{'path': 'current.pcb', 'base_sha256': hashlib.sha256(board.encode()).hexdigest(),
                                     'replacements': [{'start_line': number, 'end_line': number,
                                                       'text': line.replace('square', '', 1)}]}]}
            answer = 'Proposed edit:\n<pcb-ai-proposal>' + json.dumps(proposal) + '</pcb-ai-proposal>\nReview before implementing.'
        elif isinstance(message, list):
            image = message[1]['image_url']['url']
            assert image.startswith('data:image/png;base64,')
            assert base64.b64decode(image.split(',', 1)[1]).startswith(b'\x89PNG\r\n\x1a\n')
            answer = {'reply': 'Proposed width to 80%, endpoints fixed.',
                      'action': {'operation': 'scale_selected_pad_width', 'percent': 80}}
        else:
            answer = {'reply': 'Please send the viewport.', 'need_image': True}
        body = json.dumps({'choices': [{'message': {'content': answer if isinstance(answer, str) else json.dumps(answer)}}]}).encode()
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)


def main():
    """Build a private test plugin and launch only an isolated PCB process."""
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, default=root / 'upgrade_data/scratch/build-gtk3')
    parser.add_argument('--board', type=Path, required=True)
    parser.add_argument('--preferences', type=Path, help='Copy this preference file into the disposable HOME')
    args = parser.parse_args()
    build = args.build.resolve()
    flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'gtk+-3.0'], text=True))
    with tempfile.TemporaryDirectory(prefix='pcb-ai-native-') as directory:
        work = Path(directory)
        (work / 'plugins').mkdir()
        shutil.copyfile(args.board, work / 'fixture.pcb')
        if args.preferences:
            (work / '.pcb').mkdir()
            shutil.copyfile(args.preferences, work / '.pcb/preferences')
        subprocess.run(['cc', '-std=gnu23', '-Wall', '-Wextra', '-Werror', '-Wpedantic',
                        '-shared', '-fPIC', '-Wl,-undefined,dynamic_lookup',
                        '-I' + str(build), '-I' + str(root), '-I' + str(root / 'src'),
                        '-I' + str(root / 'src/hid/gtk'),
                        str(root / 'tools/test_ai_visual_native.c'),
                        '-o', str(work / 'plugins/ai_visual.so'), *flags], check=True)
        server = ThreadingHTTPServer(('127.0.0.1', 0), Provider)
        threading.Thread(target=server.serve_forever, daemon=True).start()
        env = os.environ.copy()
        env.update(HOME=str(work), XDG_CONFIG_HOME=str(work / 'config'),
                   PCB_AI_TEST_ENDPOINT=f'http://127.0.0.1:{server.server_port}/v1/chat/completions')
        subprocess.run([str(build / 'src/pcb'), '--font-path', str(root / 'src'),
                        '--lib-path', str(root / 'lib'), '--lib-newlib', str(root / 'newlib'),
                        '--lib-command-dir', str(build / 'lib'), '--action-string',
                        'AiVisualRegression()', str(work / 'fixture.pcb')],
                       cwd=work, env=env, check=True, timeout=30)
        server.shutdown()
        server.server_close()
        assert len(Provider.calls) == 3, "Expected one image continuation followed by one tagged patch request"


if __name__ == '__main__':
    main()
