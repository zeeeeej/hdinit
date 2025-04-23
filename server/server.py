from flask import Flask, jsonify, send_from_directory, request
import os
import hashlib
import json
import base64
import re
from urllib.parse import unquote  # 添加这行导入

app = Flask(__name__)

# 配置路径
FILE_DIR = "./files"
CONFIG_FILE = FILE_DIR+"/update_config.json"

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


# 配置
UPLOAD_FOLDER = 'uploads'
ALLOWED_EXTENSIONS = {'png', 'jpg', 'jpeg', 'gif', 'bmp'}
MAX_CONTENT_LENGTH = 16 * 1024 * 1024  # 16MB 最大上传大小

app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER
app.config['MAX_CONTENT_LENGTH'] = MAX_CONTENT_LENGTH

def allowed_file(filename):
    """检查文件扩展名是否合法"""
    return '.' in filename and \
           filename.rsplit('.', 1)[1].lower() in ALLOWED_EXTENSIONS

@app.route('/upload/base64/<path:filename>', methods=['POST'])
def upload_base64(filename):
    """处理图片Base64上传"""
    # 解码URL编码的文件名
    filename = unquote(filename)
    
    # 检查文件名是否合法
    if not allowed_file(filename):
        return jsonify({
            'status': 'error',
            'message': 'File extension not allowed'
        }), 400
    
    # 确保上传目录存在
    if not os.path.exists(app.config['UPLOAD_FOLDER']):
        os.makedirs(app.config['UPLOAD_FOLDER'])
    
    try:
        # 获取请求数据
        if request.is_json:
            # 如果客户端发送的是JSON格式
            data = request.get_json()
            image_data = data.get('image')
            if not image_data:
                raise ValueError("Missing 'image' field in JSON")
        else:
            # 如果客户端直接发送Base64字符串
            image_data = request.data.decode('utf-8')
        
        # 处理 data:image/png;base64, 格式
        if image_data.startswith('data:image'):
            header, image_data = image_data.split(',', 1)
        
        # 解码Base64数据
        image_bytes = base64.b64decode(image_data)
        
        # 保存文件
        filepath = os.path.join(app.config['UPLOAD_FOLDER'], filename)
        with open(filepath, 'wb') as f:
            f.write(image_bytes)
        
        # 返回成功响应
        return jsonify({
            'status': 'success',
            'message': 'Image uploaded successfully',
            'filename': filename,
            'path': filepath
        }), 201
    
    except Exception as e:
        # 返回错误响应
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 400

@app.route('/', methods=['GET'])
def index():
    """返回简单的使用说明"""
    return """
    <h1>图片Base64上传API</h1>
    <p>使用POST方法上传图片:</p>
    <pre>POST /upload/base64/&lt;filename&gt;</pre>
    <p>请求体可以是:</p>
    <ul>
        <li>直接发送Base64字符串</li>
        <li>JSON格式: {"image": "base64字符串"}</li>
    </ul>
    """

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5002, debug=True)
