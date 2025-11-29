import socket
import os
import io
import datetime
import uvicorn
import struct
import urllib.parse
from dotenv import load_dotenv
from fastapi import FastAPI, UploadFile, File, HTTPException, Form, Depends
from fastapi.responses import HTMLResponse, StreamingResponse
from fastapi.middleware.cors import CORSMiddleware
from sqlalchemy import create_engine, Column, Integer, String, Boolean, or_, and_
from sqlalchemy.orm import sessionmaker, Session, declarative_base
from passlib.context import CryptContext
from jose import jwt

load_dotenv()
SECRET_KEY = os.getenv("SECRET_KEY", "7a9c8d2e1f3b4a5c6d7e8f9a0b1c2d3e")
ALGORITHM = "HS256"
CPP_SERVER_IP = os.getenv("CPP_SERVER_IP", "127.0.0.1")
CPP_SERVER_PORT = int(os.getenv("CPP_SERVER_PORT", 8080))

SQLALCHEMY_DATABASE_URL = "sqlite:///./securechat.db"
engine = create_engine(SQLALCHEMY_DATABASE_URL, connect_args={"check_same_thread": False})
SessionLocal = sessionmaker(bind=engine)
Base = declarative_base()

class User(Base):
    __tablename__ = "users"
    id = Column(Integer, primary_key=True, index=True)
    username = Column(String, unique=True, index=True)
    hashed_password = Column(String)

class Message(Base):
    __tablename__ = "messages"
    id = Column(Integer, primary_key=True, index=True)
    sender = Column(String)
    receiver = Column(String, index=True)
    content = Column(String, nullable=True)
    file_id = Column(String, nullable=True)
    file_key = Column(String, nullable=True)
    filename = Column(String, nullable=True)
    timestamp = Column(String)
    visible_to_sender = Column(Boolean, default=True)
    visible_to_receiver = Column(Boolean, default=True)

Base.metadata.create_all(bind=engine)
pwd_context = CryptContext(schemes=["argon2"], deprecated="auto")
app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
    expose_headers=["Content-Disposition"] 
)

def get_db():
    db = SessionLocal()
    try: yield db
    finally: db.close()

def get_current_user(token: str, db: Session):
    try:
        payload = jwt.decode(token, SECRET_KEY, algorithms=[ALGORITHM])
        username = payload.get("sub")
        if not username: return None
        return db.query(User).filter(User.username == username).first()
    except: return None

def create_header(ext): return ext[:8].ljust(8).encode()

@app.post("/token")
def login(username: str = Form(...), password: str = Form(...), db: Session = Depends(get_db)):
    user = db.query(User).filter(User.username == username).first()
    if not user or not pwd_context.verify(password, user.hashed_password): raise HTTPException(400)
    return {"access_token": jwt.encode({"sub": user.username}, SECRET_KEY, algorithm=ALGORITHM)}

@app.post("/register")
def register(username: str = Form(...), password: str = Form(...), db: Session = Depends(get_db)):
    if db.query(User).filter(User.username == username).first(): raise HTTPException(400)
    db.add(User(username=username, hashed_password=pwd_context.hash(password)))
    db.commit()
    return {"status": "ok"}

@app.get("/users")
def get_users(token: str, db: Session = Depends(get_db)):
    if not get_current_user(token, db): raise HTTPException(401)
    return db.query(User).all()

@app.post("/send")
async def send(
    token: str = Form(...), receiver: str = Form(...), text: str = Form(""),
    file: UploadFile = None, 
    max_downloads: int = Form(1), 
    expiry_hours: float = Form(24.0),
    db: Session = Depends(get_db)
):
    user = get_current_user(token, db)
    if not user: raise HTTPException(401)

    msg = Message(sender=user.username, receiver=receiver, content=text, timestamp=datetime.datetime.now().strftime("%H:%M"))

    if file:
        content = await file.read()
        fname, ext = os.path.splitext(file.filename)
        if not ext: ext = ".bin"
        
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((CPP_SERVER_IP, CPP_SERVER_PORT))
                s.sendall(b'U')
                s.sendall(struct.pack('>HII', 0xBEEF, max_downloads, int(expiry_hours * 3600)))
                s.sendall(create_header(ext) + content)
                s.shutdown(socket.SHUT_WR)
                resp = s.recv(1024).decode()
            
            if "|" not in resp: raise Exception("Handshake Failed")
            msg.file_id, msg.file_key = resp.split("|")
            msg.filename = file.filename
        except Exception as e:
            print(f"UPLOAD ERROR: {e}")
            raise HTTPException(500, "Core Error")

    db.add(msg); db.commit()
    return {"status": "sent"}

@app.get("/history/{contact}")
def history(contact: str, token: str, db: Session = Depends(get_db)):
    user = get_current_user(token, db)
    if not user: raise HTTPException(401)
    
    return db.query(Message).filter(
        or_(
            and_(Message.sender == user.username, Message.receiver == contact, Message.visible_to_sender == True),
            and_(Message.sender == contact, Message.receiver == user.username, Message.visible_to_receiver == True)
        )
    ).order_by(Message.id.asc()).all()

@app.get("/download/{msg_id}")
def download(msg_id: int, token: str, db: Session = Depends(get_db)):
    user = get_current_user(token, db)
    if not user: raise HTTPException(401)

    msg = db.query(Message).filter(Message.id == msg_id).first()
    if not msg: raise HTTPException(404)

    if msg.sender == user.username:
        raise HTTPException(403, "Sender cannot download file.")

    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((CPP_SERVER_IP, CPP_SERVER_PORT))
            s.sendall(b'D' + f"{msg.file_id}{msg.file_key}".encode())
            data = b""
            while True:
                chunk = s.recv(4096)
                if not chunk: break
                data += chunk
            
            if b"ERROR" in data[:10] or len(data) < 8:
                if msg.receiver == user.username: 
                    msg.file_id = "BURNED"
                    db.commit()
                raise HTTPException(404, "Burned")

            safe_filename = urllib.parse.quote(msg.filename)
            
            return StreamingResponse(
                io.BytesIO(data[8:]), 
                media_type="application/octet-stream", 
                headers={
                    "Content-Disposition": f"attachment; filename*=UTF-8''{safe_filename}"
                }
            )
    except: raise HTTPException(500, "Core Error")

@app.get("/")
def main(): return HTMLResponse(open("index.html").read())

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=3000)