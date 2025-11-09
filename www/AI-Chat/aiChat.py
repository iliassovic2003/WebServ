#!/usr/bin/env python3

import sys
import os
import json
import urllib.request
import urllib.parse
import time

# Gemini API Configuration - USING EXACT MODEL NAMES FROM LIST
GEMINI_API_URL = "https://generativelanguage.googleapis.com/v1/models/gemini-2.5-flash:generateContent"
API_KEY = "REMEMBER KIDS, DONT PUSH YOUR API KEY"

def get_gemini_response(user_message):
    try:
        url = f"{GEMINI_API_URL}?key={API_KEY}"
        
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
        
        with urllib.request.urlopen(req) as response:
            result = json.loads(response.read().decode('utf-8'))
            
            # Extract the response text from Gemini's response structure
            if 'candidates' in result and len(result['candidates']) > 0:
                return result['candidates'][0]['content']['parts'][0]['text']
            else:
                return "❌ No response generated from Gemini"
            
    except urllib.error.HTTPError as e:
        error_data = e.read().decode()
        try:
            error_json = json.loads(error_data)
            error_msg = error_json.get('error', {}).get('message', error_data)
        except:
            error_msg = error_data
        return f"❌ Gemini API Error (HTTP {e.code}): {error_msg}"
    except Exception as e:
        return f"❌ Connection Error: {str(e)}"

def main():
    """Main CGI handler - receives POST data and responds"""
    try:
        # Read Content-Length from environment
        content_length = int(os.environ.get('CONTENT_LENGTH', 0))
        
        if content_length == 0:
            print('{"status": "error", "error": "No POST data received"}')
            return
        
        # Read POST body from stdin
        post_data = sys.stdin.read(content_length)
        
        # Parse JSON
        try:
            data = json.loads(post_data)
        except json.JSONDecodeError:
            print('{"status": "error", "error": "Invalid JSON format"}')
            return
        
        # Get user message
        user_message = data.get('message', '').strip()
        
        if not user_message:
            print('{"status": "error", "error": "Empty message"}')
            return
        
        # Get AI response from Gemini API
        ai_response = get_gemini_response(user_message)
        
        # Prepare success response
        response = {
            "status": "success",
            "response": ai_response,
            "timestamp": time.strftime('%Y-%m-%d %H:%M:%S'),
            "message_length": len(user_message),
            "model": "gemini-2.5-flash"
        }
        
        # Output JSON
        print(json.dumps(response))
        
    except Exception as e:
        print(json.dumps({"status": "error", "error": f"Server error: {str(e)}"}))

if __name__ == "__main__":
    main()
