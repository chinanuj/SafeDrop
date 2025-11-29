# SafeDrop - Secure Ephemeral File Exchange

SafeDrop is a hybrid C++/Python secure file transfer system designed for high-performance, ephemeral messaging. It features a custom binary TCP protocol, AES-256 encryption, and a strict "Burn-on-Read" mechanism handled at the file-system level.

## 🚀 Features

* **Hybrid Architecture:** High-performance C++ core for encryption/IO, bridged with a Python FastAPI web layer.
* **Custom Binary Protocol:** Engineered a raw TCP header protocol (`0xBEEF` magic bytes + structured metadata) to prevent protocol collisions and ensure data integrity.
* **AES-256 Encryption:** Files are encrypted specifically for each transaction using OpenSSL with unique Initialization Vectors (IVs).
* **Burn-on-Read:** Implements secure deletion logic in C++ that physically removes files from the disk immediately after the download limit or expiry time is reached.
* **Concurrent Handling:** Multi-threaded socket server capable of handling simultaneous upload/download streams.

## 🛠️ Tech Stack

* **Core:** C++17 (OpenSSL, Native Sockets)
* **API:** Python 3.11 (FastAPI, Uvicorn, SQLAlchemy)
* **Database:** SQLite
* **Protocol:** Custom TCP Binary Packets

## ⚙️ Setup & Installation

### 1. Prerequisites
* C++ Compiler (`g++` or `clang`)
* OpenSSL Development Libraries
* Python 3.9+

### 2. Compile the Core Server
```bash
# For macOS (Apple Silicon with Homebrew OpenSSL):
arch -x86_64 g++ -std=c++17 src/Server.cpp src/FileHandler.cpp src/CryptoHandler.cpp -o server -I"$(brew --prefix openssl@3)/include" -L"$(brew --prefix openssl@3)/lib" -lssl -lcrypto

# For Linux/Standard:
g++ -std=c++17 src/Server.cpp src/FileHandler.cpp src/CryptoHandler.cpp -o server -lssl -lcrypto
````

### 3\. Install Python Dependencies

```bash
pip install fastapi uvicorn python-dotenv sqlalchemy passlib python-jose[cryptography] argon2-cffi python-multipart
```

### 4\. Configuration

Create a `.env` file in the root directory:

```ini
SECRET_KEY="your_secret_key_here"
ALGORITHM="HS256"
CPP_SERVER_IP="127.0.0.1"
CPP_SERVER_PORT=8080
```

## 🏃‍♂️ Usage

1.  **Start the Core Server:**
    ```bash
    ./server
    ```
2.  **Start the API:**
    ```bash
    python3 api.py
    ```
3.  **Access the UI:**
    Open `http://localhost:3000` in your browser.

## 🛡️ Architecture Overview

[Client] --(HTTP/JSON)--\> [Python FastAPI] --(Custom TCP Protocol)--\> [C++ Core Server]
|
[File System]
(Encrypted Storage)

````

### Step 4: Create `requirements.txt`
Run this command to generate your dependency list so others can install it easily:

```bash
pip freeze > requirements.txt
````

*(Ideally, you manually clean this list to only include `fastapi`, `uvicorn`, `python-dotenv`, `sqlalchemy`, `passlib`, `python-jose`, `argon2-cffi`, `python-multipart` to keep it clean, but `pip freeze` is fine for now).*

