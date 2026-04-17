import json
import unittest
from pathlib import Path

from backend.pipeline import run_debugging_pipeline


class PipelineTests(unittest.TestCase):
    def test_curated_cases_match_expected_root_causes(self):
        data_path = Path(__file__).resolve().parents[1] / "data" / "test_cases.json"
        test_cases = json.loads(data_path.read_text(encoding="utf-8"))

        for case in test_cases:
            with self.subTest(case=case["name"]):
                result = run_debugging_pipeline(case["payload"])
                self.assertEqual(result["root_cause"], case["expected_root_cause"])
                self.assertIn("summary", result)
                self.assertIn("suggested_fixes", result)


if __name__ == "__main__":
    unittest.main()
