"""Check PNG messages, bounded image requests and typed pad proposals offline."""
import base64
import importlib.util
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'ai'))
import assist_protocol as protocol


class VisualProtocol(unittest.TestCase):
    """No real credentials or network requests are used."""

    def request(self):
        """Create a minimal payload and request with a selected pad."""
        return ({'messages': [{'role': 'system', 'content': 'board'},
                              {'role': 'user', 'content': 'Inspect this'}]},
                {'provider': 'openai', 'selection': 'pad id=1 width_coord=1000'})

    def test_png_provider_formats(self):
        """OpenAI-compatible providers use content parts; Ollama uses images."""
        for provider in ('openai', 'openrouter', 'deepseek', 'compatible', 'ollama'):
            payload, request = self.request()
            request.update(provider=provider, image_png=base64.b64encode(b'\x89PNG\r\n\x1a\nfixture').decode())
            message = protocol.add_context(payload, request)['messages'][-1]
            if provider == 'ollama':
                self.assertEqual(message['images'], [request['image_png']])
            else:
                self.assertTrue(message['content'][1]['image_url']['url'].startswith('data:image/png;base64,'))

    def test_bad_image_rejected(self):
        """Reject non-PNG and invalid base64 inputs before HTTP transport."""
        for image in ('not base64!', base64.b64encode(b'not PNG').decode()):
            payload, request = self.request()
            request['image_png'] = image
            with self.assertRaises(ValueError):
                protocol.add_context(payload, request)

    def test_image_request_bounded(self):
        """An already-attempted image cannot cause a request loop."""
        _, request = self.request()
        reply = '{"reply":"Need an image", "need_image":true}'
        self.assertIn('kind=image', protocol.encode_reply(reply, request))
        request['image_attempted'] = True
        self.assertIn('kind=reply', protocol.encode_reply(reply, request))

    def test_typed_proposal(self):
        """Only finite, bounded, selection-scoped pad widths are proposed."""
        _, request = self.request()
        good = '{"reply":"Width to 80%", "action":{"operation":"scale_selected_pad_width","percent":80}}'
        self.assertIn('percent=80', protocol.encode_reply(good, request))
        for bad in (good.replace('80}', '0}'), good.replace('80}', 'NaN}'),
                    good.replace('80}', 'true}'), good.replace('scale_selected_pad_width', 'shell')):
            with self.assertRaises(ValueError):
                protocol.encode_reply(bad, request)
        with self.assertRaises(ValueError):
            protocol.encode_reply(good, {})

    def test_via_operation_requires_selected_plated_vias(self):
        """A via edit cannot target a pad or an unplated hole."""
        _, request = self.request()
        reply = '{"reply":"Diameter to 120%, drill unchanged", "action":{"operation":"scale_selected_via_diameter","percent":120}}'
        for selection in ('pad id=1', 'hole id=2', ''):
            request['selection'] = selection
            with self.assertRaises(ValueError):
                protocol.encode_reply(reply, request)
        request['selection'] = 'via id=2 diameter_coord=900000 drill_coord=500000'
        self.assertIn('operation=scale_selected_via_diameter', protocol.encode_reply(reply, request))
        with self.assertRaises(ValueError):
            protocol.encode_reply(reply.replace('120}', '80}'), request)

    def test_plain_reply_not_executable(self):
        """INI-like model prose is always base64 data, never an action."""
        response = protocol.encode_reply('[response]\npercent=80\n', {})
        self.assertIn('percent=0\n', response)
        self.assertNotIn('text=[response]', response)


if __name__ == '__main__':
    unittest.main()
