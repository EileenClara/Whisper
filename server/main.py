"""
LoveBeacon 服务端入口
FastAPI应用 + MQTT桥接 + 天气服务
启动命令: python main.py
"""

import asyncio
import json
import logging
import sys
from contextlib import asynccontextmanager

from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware

from database import init_db, get_weather_cache
from mqtt_bridge import MQTTBridge
from weather_service import WeatherService

# ============================================================
# 日志配置
# ============================================================

def setup_logging():
    """配置日志格式"""
    config = load_config()
    level = getattr(logging, config["server"].get("log_level", "INFO"))

    formatter = logging.Formatter(
        "[%(asctime)s] [%(name)s] %(levelname)s: %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
    )

    handler = logging.StreamHandler(sys.stdout)
    handler.setFormatter(formatter)

    root = logging.getLogger()
    root.setLevel(level)
    root.handlers.clear()
    root.addHandler(handler)

    # 减少第三方库的日志噪音
    logging.getLogger("gmqtt").setLevel(logging.WARNING)
    logging.getLogger("httpx").setLevel(logging.WARNING)
    logging.getLogger("httpcore").setLevel(logging.WARNING)


def load_config():
    with open("config.json", "r", encoding="utf-8") as f:
        return json.load(f)


# ============================================================
# 全局服务实例
# ============================================================

mqtt_bridge: MQTTBridge | None = None
weather_service: WeatherService | None = None
logger = logging.getLogger("LoveBeacon")


@asynccontextmanager
async def lifespan(app: FastAPI):
    """FastAPI生命周期：启动/关闭MQTT桥接和天气服务"""
    global mqtt_bridge, weather_service

    setup_logging()
    logger.info("=" * 50)
    logger.info("  LoveBeacon 服务端启动中...")
    logger.info("=" * 50)

    # 初始化数据库
    await init_db()

    # 启动MQTT桥接
    config = load_config()
    mqtt_bridge = MQTTBridge()

    # 重试连接MQTT（最多5次）
    for attempt in range(5):
        try:
            await mqtt_bridge.start()
            break
        except Exception as e:
            logger.warning(f"MQTT连接失败 ({attempt + 1}/5): {e}")
            if attempt < 4:
                await asyncio.sleep(3)
            else:
                logger.error("MQTT连接最终失败，服务将继续运行但MQTT不可用")

    # 启动天气服务
    weather_service = WeatherService(mqtt_bridge=mqtt_bridge)
    await weather_service.start()

    logger.info("LoveBeacon 服务端已就绪 ✅")

    yield  # 应用运行中...

    # 关闭
    logger.info("正在关闭服务...")
    if weather_service:
        await weather_service.stop()
    if mqtt_bridge:
        await mqtt_bridge.stop()
    logger.info("LoveBeacon 服务端已停止")


# ============================================================
# FastAPI 应用
# ============================================================

app = FastAPI(
    title="LoveBeacon",
    description="异地情侣通讯设备 — 服务端 API",
    version="1.0.0",
    lifespan=lifespan,
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


# ============================================================
# API 路由
# ============================================================

@app.get("/")
async def root():
    """服务状态"""
    return {
        "service": "LoveBeacon",
        "version": "1.0.0",
        "status": "running",
    }


@app.get("/health")
async def health_check():
    """健康检查"""
    return {
        "status": "ok",
        "mqtt_connected": mqtt_bridge._connected if mqtt_bridge else False,
    }


@app.get("/weather/{device_id}")
async def get_device_weather(device_id: str):
    """获取指定设备的天气数据（从缓存）"""
    config = load_config()
    if device_id not in config["devices"]:
        raise HTTPException(status_code=404, detail="未知设备")

    city_id = config["devices"][device_id]["weather_city_id"]
    city_name = config["devices"][device_id]["city"]

    if weather_service:
        weather = await weather_service.get_cached_or_fetch(city_id, city_name)
    else:
        cached = await get_weather_cache(city_id)
        weather = cached["data"] if cached else None

    if not weather:
        raise HTTPException(status_code=503, detail="天气数据不可用")

    return {"device_id": device_id, "weather": weather}


@app.post("/message/test")
async def send_test_message(device_id: str, content: str = "测试消息"):
    """测试：向设备发送消息（调试用）"""
    import time
    from models import MessageReceive

    config = load_config()
    if device_id not in config["devices"]:
        raise HTTPException(status_code=404, detail="未知设备")

    msg = MessageReceive(
        msg_id="test_" + str(int(time.time())),
        from_device="server",
        from_name="系统",
        content=content,
        timestamp=int(time.time()),
    )
    topic = f"lovebeacon/{device_id}/message/receive"
    await mqtt_bridge.publish(topic, msg.model_dump_json())

    return {"status": "sent", "device_id": device_id, "content": content}


# ============================================================
# 入口
# ============================================================

if __name__ == "__main__":
    import uvicorn

    config = load_config()
    uvicorn.run(
        "main:app",
        host=config["server"]["host"],
        port=config["server"]["port"],
        reload=False,
        log_level=config["server"]["log_level"].lower(),
    )
