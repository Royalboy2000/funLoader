import unittest
import os
import re
import sys

# Add the root directory to the Python path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from telegram_bot import generate_shellcode, update_cpp_file


class TestBot(unittest.TestCase):

    def setUp(self):
        """Set up for tests."""
        self.test_exe_path = "test.exe"
        with open(self.test_exe_path, "wb") as f:
            f.write(b"\xDE\xAD\xBE\xEF")

        self.cpp_file_path = "funLoader/load.cpp"
        # Ensure the original file exists before reading
        if os.path.exists(self.cpp_file_path):
            with open(self.cpp_file_path, "r", encoding="utf-8") as f:
                self.original_cpp_content = f.read()
        else:
            # Create a dummy file if it doesn't exist, with the payload line
            self.original_cpp_content = 'char payload[] = "";'
            with open(self.cpp_file_path, "w", encoding="utf-8") as f:
                f.write(self.original_cpp_content)


    def tearDown(self):
        """Tear down after tests."""
        if os.path.exists(self.test_exe_path):
            os.remove(self.test_exe_path)
        # Restore the original content of the C++ file
        with open(self.cpp_file_path, "w", encoding="utf-8") as f:
            f.write(self.original_cpp_content)

    def test_generate_shellcode(self):
        """Test the shellcode generation (no XOR)."""
        expected_shellcode = r"\xde\xad\xbe\xef"
        generated = generate_shellcode(self.test_exe_path)
        self.assertEqual(generated, expected_shellcode)

    def test_update_cpp_file(self):
        """Test updating the C++ file with non-greedy regex."""
        shellcode = r"\xde\xad\xbe\xef"
        update_cpp_file(shellcode)

        with open(self.cpp_file_path, "r", encoding="utf-8") as f:
            content = f.read()

        # Use a non-greedy regex to find the payload
        match = re.search(r'char payload\[\] = "(.*?)"', content, re.DOTALL)
        self.assertTrue(match, "Payload not found in C++ file.")
        self.assertEqual(match.group(1), shellcode)

if __name__ == '__main__':
    unittest.main()