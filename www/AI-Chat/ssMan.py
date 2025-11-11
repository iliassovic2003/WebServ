#!/usr/bin/env python3

import os
import json
from datetime import datetime
from pathlib import Path
import sys

script_dir = Path(os.path.dirname(os.path.abspath(__file__)))
SESSIONS_DIR = script_dir / "chat_sessions"

try:
    SESSIONS_DIR.mkdir(exist_ok=True, mode=0o755)
except Exception as e:
    print("Content-type: application/json\n")
    print(json.dumps({'error': f'Directory setup failed: {str(e)}'}))
    sys.exit(1)

def get_session_file(session_id):
    return SESSIONS_DIR / f".{session_id}.json"

def load_session(session_id):
    session_file = get_session_file(session_id)
    
    if session_file.exists():
        try:
            with open(session_file, 'r', encoding='utf-8') as f:
                data = json.load(f)
                return data
        except Exception:
            return None
    
    return None

def save_session(session_id, session_data):
    session_file = get_session_file(session_id)
    try:
        with open(session_file, 'w', encoding='utf-8') as f:
            json.dump(session_data, f, indent=2, ensure_ascii=False)
        return True
    except Exception:
        return False

def create_new_session(session_id, expiry_date=None):
    session_data = {
        'session_id': session_id,
        'created_at': datetime.now().isoformat(),
        'updated_at': datetime.now().isoformat(),
        'expires_at': expiry_date,
        'messages': [],
        'message_count': 0
    }
    save_session(session_id, session_data)
    return session_data

def send_response(response_data):
    print("Content-type: application/json\n")
    print(json.dumps(response_data, ensure_ascii=False))

def send_error(error_message):
    print("Content-type: application/json\n")
    print(json.dumps({'error': error_message}))

def main():
    try:
        content_length = int(os.environ.get('CONTENT_LENGTH', 0))
        
        if content_length > 0:
            post_data = sys.stdin.read(content_length)
            data = json.loads(post_data)
        else:
            data = {}
    except Exception as e:
        send_error(f'Failed to parse request: {str(e)}')
        return
    
    session_id = data.get('session_id')
    action = data.get('action', 'chat')
    
    if not session_id or len(session_id) < 5:
        send_error('Invalid session ID')
        return
    
    session_data = load_session(session_id)
    
    if not session_data:
        expiry_date = data.get('expiry_date')
        session_data = create_new_session(session_id, expiry_date)
    
    if action == 'save_message':
        user_message = data.get('user_message', '')
        ai_response = data.get('ai_response', '')
        
        if user_message and ai_response:
            message_entry = {
                'user': user_message,
                'ai': ai_response,
                'timestamp': datetime.now().isoformat()
            }
            session_data['messages'].append(message_entry)
            session_data['message_count'] += 1
            session_data['updated_at'] = datetime.now().isoformat()
            
            if 'expiry_date' in data:
                session_data['expires_at'] = data['expiry_date']
            
            if not save_session(session_id, session_data):
                send_error('Failed to save message')
                return
        
        response_data = {
            'status': 'saved',
            'session_id': session_id,
            'message_count': session_data['message_count']
        }
        
    elif action == 'load_history':
        chat_history = session_data.get('messages', [])[-10:]
        
        response_data = {
            'session_id': session_id,
            'message_count': session_data.get('message_count', 0),
            'chat_history': chat_history
        }
        
    else:
        user_message = data.get('message', '').strip()
        
        if user_message:
            ai_response = f"Session response to: '{user_message}'"
            
            message_entry = {
                'user': user_message,
                'ai': ai_response,
                'timestamp': datetime.now().isoformat()
            }
            session_data['messages'].append(message_entry)
            session_data['message_count'] += 1
            session_data['updated_at'] = datetime.now().isoformat()
            
            if not save_session(session_id, session_data):
                send_error('Failed to save session')
                return
        else:
            ai_response = "Welcome!"
        
        chat_history = session_data.get('messages', [])[-10:]
        
        response_data = {
            'response': ai_response,
            'session_id': session_id,
            'message_count': session_data.get('message_count', 0),
            'chat_history': chat_history
        }
    
    send_response(response_data)

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        send_error(f'Internal server error: {str(e)}')