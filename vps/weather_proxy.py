#!/usr/bin/env python3
"""
SDD Weather Proxy — MQTT-to-OpenWeatherMap bridge
监听 sdd/+/weather/req → 调 API → 发布 sdd/weather/resp
"""

import json, time, requests, paho.mqtt.client as mqtt
from threading import Lock

# ==== 配置 ====
MQTT_BROKER = "127.0.0.1"
MQTT_PORT = 1883
OWM_KEY = "271834fb90974033d0a897a008687d88"
POLL_MIN = 10  # 缓存 10 分钟, 不管几个设备请求

# OpenWeatherMap 天气代码 → 内部图标编号映射
CODE_MAP = {
    800: 0,   # 晴
    801: 1,   # 少云
    802: 2,   # 多云
    803: 3,   # 阴(多云)
    804: 4,   # 阴
    300: 5, 301: 5, 302: 5, 310: 5, 311: 5, 312: 5, 313: 5, 314: 5, 321: 5,  # 小雨
    500: 6, 501: 6,  # 中雨
    502: 7, 503: 7, 504: 7, 511: 7, 520: 7, 521: 7, 522: 7, 531: 7,  # 大雨
    200: 9, 201: 9, 202: 9, 210: 9, 211: 9, 212: 9, 221: 9, 230: 9, 231: 9, 232: 9,  # 雷暴
    600: 13, 601: 13, 602: 13, 611: 13, 612: 13, 613: 13, 615: 13, 616: 13, 620: 13, 621: 13, 622: 13,  # 雪
    701: 18, 711: 18, 721: 18, 731: 18, 741: 18, 751: 18, 761: 18, 762: 18, 771: 18, 781: 18,  # 雾
}

def map_code(owm_id):
    return CODE_MAP.get(owm_id, 99)

def fetch_city(city_en, city_cn):
    try:
        r = requests.get("https://api.openweathermap.org/data/2.5/weather", params={
            "q": city_en, "appid": OWM_KEY, "units": "metric", "lang": "en"
        }, timeout=10)
        r.raise_for_status()
        d = r.json()
        return {
            "temp": round(d["main"]["temp"]),
            "humidity": d["main"]["humidity"],
            "weather": d["weather"][0]["description"],
            "weather_code": map_code(d["weather"][0]["id"]),
            "city": city_cn
        }
    except Exception as e:
        print(f"[Weather] Error fetching {city_cn}: {e}")
        return None

# ==== 缓存 ====
cache = None
cache_time = 0
lock = Lock()

def refresh():
    global cache, cache_time
    with lock:
        now = time.time()
        if cache and (now - cache_time) < POLL_MIN * 60:
            return
        bj = fetch_city("Beijing,CN", "Beijing")
        sg = fetch_city("Singapore,SG", "Singapore")
        if bj and sg:
            cache = {"beijing": bj, "singapore": sg}
            cache_time = now
            print(f"[Weather] Cache refreshed: BJ={bj['temp']}C SG={sg['temp']}C")

# ==== MQTT ====
def on_connect(client, userdata, flags, rc):
    print(f"[MQTT] Proxy connected (rc={rc})")
    client.subscribe("sdd/+/weather/req")

def on_message(client, userdata, msg):
    print(f"[MQTT] Weather request from {msg.topic}")
    refresh()
    if cache:
        resp = dict(cache)
        resp["fetched_at"] = int(time.time())
        client.publish("sdd/weather/resp", json.dumps(resp, ensure_ascii=False), qos=1)
        print("[MQTT] Published weather response")

def main():
    client = mqtt.Client(client_id="weather_proxy")
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    print(f"[Weather] Proxy started, broker={MQTT_BROKER}:{MQTT_PORT}")
    client.loop_forever()

if __name__ == "__main__":
    main()
