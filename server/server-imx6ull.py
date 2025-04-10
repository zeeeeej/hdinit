from flask import Flask, jsonify, send_from_directory, request
import os
import hashlib
import json

app = Flask(__name__)

# 配置路径
FILE_DIR = "./files"
CONFIG_FILE = FILE_DIR+"/update_config_imx6ull.json"

os.makedirs(FILE_DIR, exist_ok=True)

def calculate_md5(filepath):
    """计算文件MD5"""
    hash_md5 = hashlib.md5()
    with open(filepath, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()

def init_config():
    """初始化配置文件"""
    if not os.path.exists(CONFIG_FILE):
        config = {
            "hdmain": {
                "version": "1.0.0",
                "filename": "hdmain-v1.0.0.bin"
            },
            "hdlog": {
                "version": "1.2.0",
                "filename": "hdlog-v1.2.0.bin"
            }
        }
        with open(CONFIG_FILE, "w") as f:
            json.dump(config, f, indent=2)
        
        # 创建示例文件
        for service in config.values():
            filepath = os.path.join(FILE_DIR, service["filename"])
            if not os.path.exists(filepath):
                with open(filepath, "wb") as f:
                    f.write(f"Sample content for {service['filename']}".encode())

init_config()

@app.route('/checkupdate/<service_name>', methods=['GET'])
def check_update(service_name):
    try:
        # 读取配置文件
        with open(CONFIG_FILE, "r") as f:
            config = json.load(f)
        
        # 检查服务是否存在
        if service_name not in config:
            return jsonify({"error": "Service not found"}), 404
        
        service_config = config[service_name]
        filename = service_config["filename"]
        filepath = os.path.join(FILE_DIR, filename)
        
        # 检查文件是否存在
        if not os.path.exists(filepath):
            return jsonify({"error": "File not found"}), 404
        
        # 计算MD5
        file_md5 = calculate_md5(filepath)
        
        # 构建完整URL
        base_url = request.host_url.rstrip('/')
        download_url = f"{base_url}/files/{filename}"
        
        return jsonify({
            "url": download_url,
            "version": service_config["version"],
            "md5": file_md5
        })
    
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route('/files/<filename>', methods=['GET'])
def download_file(filename):
    """文件下载端点"""
    return send_from_directory(FILE_DIR, filename, as_attachment=True)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5001, debug=True)
