import http.server
import socketserver
from datetime import datetime, timedelta

class CacheHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Cache-Control', 'public, max-age=2592000')
        self.send_header('Expires', (datetime.utcnow() + timedelta(days=30)).strftime("%a, %d %b %Y %H:%M:%S GMT"))
        http.server.SimpleHTTPRequestHandler.end_headers(self)

PORT = 3000

with socketserver.TCPServer(("", PORT), CacheHTTPRequestHandler) as httpd:
    print(f"Serving at port {PORT}")
    httpd.serve_forever()
