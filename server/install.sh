#!/bin/bash
# ============================================================
# LoveBeacon 服务端一键安装脚本
# 适用于: Ubuntu 22.04 (Vultr VPS 东京)
# 用法: chmod +x install.sh && sudo ./install.sh
# ============================================================

set -e  # 任何命令失败则退出

echo "=============================================="
echo "  LoveBeacon 服务端安装脚本"
echo "  目标: Ubuntu 22.04"
echo "=============================================="
echo ""

# ---- 检查是否为 root ----
if [ "$EUID" -ne 0 ]; then
    echo "请使用 sudo 运行此脚本: sudo ./install.sh"
    exit 1
fi

# ---- 配置变量（请根据实际情况修改）----
USE_SELF_SIGNED=true                # true=自签名(无域名) / false=Let's Encrypt(有域名)
SERVER_IP="198.13.56.214"           # VPS IP（自签名时需要）
DOMAIN="YOUR_DOMAIN"                # 你的域名（Let's Encrypt 时需要）
EMAIL="YOUR_EMAIL@example.com"      # 你的邮箱（Let's Encrypt 时需要）
MQTT_USER="lovebeacon"
MQTT_PASS="Tyg.2005315"             # MQTT密码（与固件 config.h 一致）

echo "[1/7] 更新系统包..."
apt update && apt upgrade -y

echo "[2/7] 安装基础依赖..."
apt install -y python3 python3-pip python3-venv mosquitto mosquitto-clients certbot ufw curl

echo "[3/7] 配置防火墙..."
ufw allow 22/tcp      # SSH
ufw allow 8000/tcp    # FastAPI
ufw allow 8883/tcp    # MQTT TLS
ufw --force enable
ufw status

echo "[4/7] 配置 Mosquitto MQTT Broker..."

# 创建 Mosquitto 用户
mosquitto_passwd -c /etc/mosquitto/passwd "$MQTT_USER" <<EOF
$MQTT_PASS
$MQTT_PASS
EOF

# 创建 ACL 文件
cat > /etc/mosquitto/acl <<ACL_EOF
# LoveBeacon MQTT ACL

# 服务端（后端）可以读写所有 topic
user $MQTT_USER
topic readwrite lovebeacon/#

# 设备只允许自己的 topic
user deviceA
topic readwrite lovebeacon/deviceA/#
topic read lovebeacon/deviceB/#

user deviceB
topic readwrite lovebeacon/deviceB/#
topic read lovebeacon/deviceA/#
ACL_EOF

# 复制 Mosquitto 配置
cp mosquitto.conf /etc/mosquitto/conf.d/lovebeacon.conf

# 替换配置中的域名占位符
sed -i "s/YOUR_DOMAIN/$DOMAIN/g" /etc/mosquitto/conf.d/lovebeacon.conf

echo "[5/7] 配置 TLS 证书..."

if [ "$USE_SELF_SIGNED" = true ]; then
    echo "使用自签名证书（无域名模式）..."

    # 创建证书目录
    mkdir -p /etc/mosquitto/certs

    # 生成 CA 私钥和根证书（10年有效期）
    openssl genrsa -out /etc/mosquitto/certs/ca.key 2048
    openssl req -new -x509 -days 3650 -key /etc/mosquitto/certs/ca.key \
        -out /etc/mosquitto/certs/ca.crt \
        -subj "/C=JP/O=Whisper/CN=Whisper-Root-CA"

    # 生成服务器私钥和证书签名请求
    openssl genrsa -out /etc/mosquitto/certs/server.key 2048
    openssl req -new -key /etc/mosquitto/certs/server.key \
        -out /etc/mosquitto/certs/server.csr \
        -subj "/C=JP/O=Whisper/CN=${SERVER_IP}"

    # 用 CA 签发服务器证书（包含 IP SAN 扩展）
    cat > /tmp/openssl-san.cnf <<EOF
[req]
distinguished_name = req_distinguished_name
[req_distinguished_name]
[v3_req]
subjectAltName = IP:${SERVER_IP}
EOF
    openssl x509 -req -days 3650 -in /etc/mosquitto/certs/server.csr \
        -CA /etc/mosquitto/certs/ca.crt -CAkey /etc/mosquitto/certs/ca.key \
        -CAcreateserial -out /etc/mosquitto/certs/server.crt \
        -extensions v3_req -extfile /tmp/openssl-san.cnf

    rm -f /etc/mosquitto/certs/server.csr /tmp/openssl-san.cnf

    echo "✅ 自签名证书已生成"
    echo ""
    echo "⚠️  重要：将下面 CA 证书内容复制到固件 firmware/src/mqtt_handler.cpp 中"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    cat /etc/mosquitto/certs/ca.crt
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

else
    echo "使用 Let's Encrypt 证书..."
    if [ "$DOMAIN" != "YOUR_DOMAIN" ]; then
        certbot certonly --standalone \
            -d "$DOMAIN" \
            --email "$EMAIL" \
            --agree-tos \
            --non-interactive
        echo "证书已获取。将设置自动续期..."
        systemctl enable certbot.timer
    else
        echo "⚠️  未配置域名，跳过。"
    fi
fi

echo "[6/7] 安装 Python 依赖..."
cd "$(dirname "$0")"
python3 -m venv venv
source venv/bin/activate
pip install --upgrade pip
pip install -r requirements.txt

echo "[7/7] 创建 systemd 服务..."
cat > /etc/systemd/system/lovebeacon.service <<SERVICE_EOF
[Unit]
Description=LoveBeacon Server
After=network.target mosquitto.service
Wants=mosquitto.service

[Service]
Type=simple
User=root
WorkingDirectory=$(pwd)
ExecStart=$(pwd)/venv/bin/python main.py
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
SERVICE_EOF

# 重启 Mosquitto 使配置生效
systemctl restart mosquitto
systemctl enable mosquitto

# 启动 LoveBeacon 服务
systemctl daemon-reload
systemctl enable lovebeacon
systemctl start lovebeacon

echo ""
echo "=============================================="
echo "  LoveBeacon 服务端安装完成! ✅"
echo "=============================================="
echo ""
echo "  检查服务状态:"
echo "    sudo systemctl status mosquitto"
echo "    sudo systemctl status lovebeacon"
echo ""
echo "  查看日志:"
echo "    sudo journalctl -u lovebeacon -f"
echo ""
echo "  测试 API:"
echo "    curl http://localhost:8000/health"
echo ""
echo "  下一步:"
echo "    1. 编辑 config.json，填入 OpenWeatherMap API Key 和设备信息"
echo "    2. 如使用自签名证书（无域名），请将 CA 证书嵌入固件 certs.h"
echo "    3. 编译固件时修改 firmware/src/config.h 中的 MQTT 地址"
echo "=============================================="
