"""
LoveBeacon 天气服务
每30分钟拉取 OpenWeatherMap API，缓存到 SQLite，推送给在线设备
"""

import asyncio
import json
import logging
import httpx
from datetime import datetime, timezone

from database import get_weather_cache, set_weather_cache
from models import WeatherNotify

logger = logging.getLogger("LoveBeacon.Weather")


def load_config():
    """加载配置文件"""
    with open("config.json", "r", encoding="utf-8") as f:
        return json.load(f)


class WeatherService:
    """天气拉取与推送服务"""

    def __init__(self, mqtt_bridge=None):
        self.config = load_config()
        self.api_key = self.config["weather"]["api_key"]
        self.interval = self.config["weather"]["update_interval_minutes"]
        self.units = self.config["weather"]["units"]
        self.lang = self.config["weather"]["lang"]
        self.mqtt_bridge = mqtt_bridge
        self._running = False
        self._task: asyncio.Task | None = None

    async def start(self):
        """启动天气定时任务"""
        self._running = True
        self._task = asyncio.create_task(self._update_loop())
        logger.info(f"天气服务已启动，每 {self.interval} 分钟更新一次")

    async def stop(self):
        """停止天气服务"""
        self._running = False
        if self._task:
            self._task.cancel()
            try:
                await self._task
            except asyncio.CancelledError:
                pass
        logger.info("天气服务已停止")

    async def _update_loop(self):
        """定时更新循环 — 先立即拉取一次，再按间隔循环"""
        # 首次立即更新
        await self._fetch_and_push_all()

        while self._running:
            try:
                await asyncio.sleep(self.interval * 60)
                await self._fetch_and_push_all()
            except asyncio.CancelledError:
                break
            except Exception as e:
                logger.error(f"天气定时任务异常: {e}")

    async def _fetch_and_push_all(self):
        """拉取所有城市天气并推送"""
        devices = self.config["devices"]
        # 收集所有需要拉取的城市（去重）
        cities = {}
        for device_id, info in devices.items():
            city_id = info["weather_city_id"]
            if city_id not in cities:
                cities[city_id] = info["city"]

        for city_id, city_name in cities.items():
            try:
                weather = await self._fetch_weather(city_id, city_name)
                if weather:
                    # 缓存到数据库
                    await set_weather_cache(city_id, weather)
                    # 推送给所有匹配的设备
                    await self._push_weather(city_id, city_name, weather)
            except Exception as e:
                logger.error(f"拉取城市 {city_name} ({city_id}) 天气失败: {e}")

    async def _fetch_weather(self, city_id: str, city_name: str) -> dict | None:
        """从 OpenWeatherMap 拉取天气数据"""
        url = "https://api.openweathermap.org/data/2.5/weather"
        params = {
            "q": city_id,
            "appid": self.api_key,
            "units": self.units,
            "lang": self.lang,
        }

        try:
            async with httpx.AsyncClient(timeout=15.0) as client:
                resp = await client.get(url, params=params)
                resp.raise_for_status()
                data = resp.json()

                now_ts = datetime.now(timezone.utc).timestamp()
                is_day = (data.get("dt", now_ts) > data.get("sys", {}).get("sunrise", 0) and
                          data.get("dt", now_ts) < data.get("sys", {}).get("sunset", 0))

                weather = {
                    "city": city_id,
                    "city_name": city_name,
                    "temperature": data["main"]["temp"],
                    "feels_like": data["main"]["feels_like"],
                    "humidity": data["main"]["humidity"],
                    "description": data["weather"][0]["description"],
                    "icon_code": data["weather"][0]["icon"],
                    "wind_speed": data["wind"]["speed"],
                    "is_day": is_day,
                    "sunrise": data["sys"]["sunrise"],
                    "sunset": data["sys"]["sunset"],
                    "updated_at": datetime.now(timezone.utc).isoformat(),
                }
                logger.info(f"天气已拉取: {city_name} {weather['temperature']}°C {weather['description']}")
                return weather

        except httpx.HTTPError as e:
            logger.error(f"HTTP请求失败 ({city_name}): {e}")
            return None
        except (KeyError, json.JSONDecodeError) as e:
            logger.error(f"天气数据解析失败 ({city_name}): {e}")
            return None

    async def _push_weather(self, city_id: str, city_name: str, weather: dict):
        """将天气数据推送给对应的在线设备"""
        if not self.mqtt_bridge:
            logger.warning("MQTT桥接未就绪，跳过天气推送")
            return

        # 找到匹配该城市的设备
        devices = self.config["devices"]
        for device_id, info in devices.items():
            if info["weather_city_id"] == city_id:
                notify = WeatherNotify(
                    city=city_id,
                    city_name=city_name,
                    temperature=weather["temperature"],
                    description=weather["description"],
                    icon_code=weather["icon_code"],
                    is_day=weather["is_day"],
                    updated_at=weather["updated_at"],
                )
                topic = f"lovebeacon/{device_id}/weather"
                payload = notify.model_dump_json()
                await self.mqtt_bridge.publish(topic, payload)
                logger.info(f"天气已推送: {device_id} <- {city_name} {weather['temperature']}°C")

    async def get_cached_or_fetch(self, city_id: str, city_name: str) -> dict | None:
        """获取缓存天气，如果缓存不存在则实时拉取"""
        cached = await get_weather_cache(city_id)
        if cached:
            return cached["data"]
        # 无缓存，实时拉取
        logger.info(f"无缓存数据，实时拉取: {city_name}")
        return await self._fetch_weather(city_id, city_name)
