import logging
import re
import os
from telegram import Update
from telegram.ext import Application, CommandHandler, MessageHandler, filters, ContextTypes

# Enable logging
logging.basicConfig(
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s", level=logging.INFO
)
logger = logging.getLogger(__name__)

# Define the path to the C++ source file
CPP_FILE_PATH = "funLoader/load.cpp"

async def start(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    """Send a message when the command /start is issued."""
    await update.message.reply_html(
        "Hi! Send me an .exe file and I will convert it to shellcode and update the loader.",
    )

async def handle_document(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    """Handle incoming .exe files."""
    document = update.message.document
    if not document.file_name.endswith(".exe"):
        await update.message.reply_text("Please send an .exe file.")
        return

    file = await context.bot.get_file(document.file_id)
    file_path = "downloaded.exe"
    await file.download_to_drive(file_path)

    try:
        shellcode = generate_shellcode(file_path)
        update_cpp_file(shellcode)
        await update.message.reply_text("Shellcode updated successfully in funLoader/load.cpp!")
    except Exception as e:
        logger.error(f"Error processing file: {e}")
        await update.message.reply_text("An error occurred while processing the file.")
    finally:
        os.remove(file_path)

def generate_shellcode(file_path: str) -> str:
    """Reads a binary file and returns it as a C-style hex string."""
    with open(file_path, "rb") as f:
        content = f.read()

    shellcode = ""
    for byte in content:
        shellcode += f"\\x{byte:02x}"

    return shellcode

def update_cpp_file(shellcode: str) -> None:
    """Updates the payload in the C++ file."""
    with open(CPP_FILE_PATH, "r", encoding="utf-8") as f:
        content = f.read()

    # Use a non-greedy regex and a lambda to prevent re.sub from interpreting the shellcode.
    # This correctly writes the \x escaped shellcode to the file.
    new_content = re.sub(
        r'(char payload\[\] = )".*?";',
        lambda m: m.group(1) + f'"{shellcode}";',
        content,
        flags=re.DOTALL,
    )

    with open(CPP_FILE_PATH, "w", encoding="utf-8") as f:
        f.write(new_content)

def main() -> None:
    """Start the bot."""
    # IMPORTANT: Replace "YOUR_TELEGRAM_BOT_TOKEN" with your actual bot token
    application = Application.builder().token("YOUR_TELEGRAM_BOT_TOKEN").build()

    # on different commands - answer in Telegram
    application.add_handler(CommandHandler("start", start))

    # on non command i.e message - handle the document
    application.add_handler(MessageHandler(filters.Document.ALL, handle_document))

    # Run the bot until the user presses Ctrl-C
    application.run_polling()

if __name__ == "__main__":
    main()