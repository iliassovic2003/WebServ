#!/usr/bin/env python3

import sys
import os
import json
import urllib.request
import urllib.parse
import time

MODELS = [
    "gemini-2.0-flash",
    "gemini-2.0-flash-exp",
    "gemini-1.5-flash",
    "gemini-1.5-flash-8b",
    "gemini-1.5-flash-exp",
    "gemini-2.0-pro",
    "gemini-2.0-pro-exp", 
    "gemini-1.5-pro",
    "gemini-1.5-pro-001",
    "gemini-1.5-pro-exp",
    "gemini-1.0-pro",
    "gemini-1.0-pro-001",
    "gemini-1.0-pro-002",
    "gemini-pro",
    "gemini-pro-vision"
]

GEMINI_API_BASE = "https://generativelanguage.googleapis.com/v1/models"
API_KEY = "Better Luck Next TIME..."

def get_gemini_response(user_message, model):
    try:
        url = f"{GEMINI_API_BASE}/{model}:generateContent?key={API_KEY}"
        
        data = {
            "contents": [
                {
                    "parts": [
                        {"text": user_message}
                    ]
                }
            ],
            "generationConfig": {
                "maxOutputTokens": 1000,
                "temperature": 0.7
            }
        }
        
        headers = {
            'Content-Type': 'application/json'
        }
        
        req = urllib.request.Request(
            url,
            data=json.dumps(data).encode('utf-8'),
            headers=headers,
            method='POST'
        )
        
        with urllib.request.urlopen(req, timeout=30) as response:
            result = json.loads(response.read().decode('utf-8'))
            
            if 'candidates' in result and len(result['candidates']) > 0:
                return result['candidates'][0]['content']['parts'][0]['text']
            else:
                return None
            
    except urllib.error.HTTPError as e:
        if e.code == 503:
            return None
        error_data = e.read().decode()
        try:
            error_json = json.loads(error_data)
            error_msg = error_json.get('error', {}).get('message', error_data)
        except:
            error_msg = error_data
        return f"❌ Gemini API Error (HTTP {e.code}): {error_msg}"
    except Exception as e:
        return f"❌ Connection Error: {str(e)}"

def try_all_models(user_message):
    for model in MODELS:
        response = get_gemini_response(user_message, model)
        
        if response is None:
            time.sleep(1)
            continue
        elif not response.startswith("❌"):
            return response, model
    
    return "❌ All Gemini models are currently overloaded. Please try again in a few moments.", "none"

def main():
    try:
        content_length = int(os.environ.get('CONTENT_LENGTH', 0))
        
        if content_length == 0:
            print('{"status": "error", "error": "No POST data received"}')
            return
        
        post_data = sys.stdin.read(content_length)
        
        try:
            data = json.loads(post_data)
        except json.JSONDecodeError:
            print('{"status": "error", "error": "Invalid JSON format"}')
            return
        
        user_message = data.get('message', '').strip()
        
        if not user_message:
            print('{"status": "error", "error": "Empty message"}')
            return

        ai_response, used_model = try_all_models(user_message)
        
        response = {
            "status": "success" if not ai_response.startswith("❌") else "error",
            "response": ai_response,
            "model_used": used_model,
            "models_tried": MODELS,
            "timestamp": time.strftime('%Y-%m-%d %H:%M:%S'),
            "message_length": len(user_message)
        }
        
        print(json.dumps(response))
        
    except Exception as e:
        print(json.dumps({"status": "error", "error": f"Server error: {str(e)}"}))

if __name__ == "__main__":
    main()
