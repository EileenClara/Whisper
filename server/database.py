"""
LoveBeacon 数据库模块
SQLite操作：离线消息 + 天气缓存
"""

import aiosqlite
import json
import logging
from typing import Optional, List
from datetime import datetime, timezone

logger = logging.getLogger("LoveBeacon.DB")

DB_PATH = "lovebeacon.db"


async def init_db():
    """初始化数据库：创建表"""
    async with aiosqlite.connect(DB_PATH) as db:
        # 离线消息表
        await db.execute("""
            CREATE TABLE IF NOT EXISTS offline_messages (
                id           INTEGER PRIMARY KEY AUTOINCREMENT,
                msg_id       TEXT UNIQUE NOT NULL,
                from_device  TEXT NOT NULL,
                from_name    TEXT NOT NULL DEFAULT '',
                to_device    TEXT NOT NULL,
                content      TEXT NOT NULL,
                type         TEXT NOT NULL DEFAULT 'msg',
                status       TEXT NOT NULL DEFAULT 'queued',
                created_at   TEXT NOT NULL,
                delivered_at TEXT
            )
        """)

        # 天气缓存表
        await db.execute("""
            CREATE TABLE IF NOT EXISTS weather_cache (
                city        TEXT PRIMARY KEY,
                data_json   TEXT NOT NULL,
                updated_at  TEXT NOT NULL
            )
        """)

        # 设备在线状态表（用于判断对方是否在线，决定消息是下发还是缓存）
        await db.execute("""
            CREATE TABLE IF NOT EXISTS device_presence (
                device_id   TEXT PRIMARY KEY,
                online      INTEGER NOT NULL DEFAULT 0,
                last_seen   TEXT NOT NULL
            )
        """)

        # 索引
        await db.execute("""
            CREATE INDEX IF NOT EXISTS idx_offline_msgs_to_status
            ON offline_messages(to_device, status)
        """)

        await db.commit()
    logger.info("数据库初始化完成")


# ============================================================
# 离线消息操作
# ============================================================

async def store_offline_message(
    msg_id: str,
    from_device: str,
    from_name: str,
    to_device: str,
    content: str,
    msg_type: str = "msg"
):
    """存储一条离线消息（对方不在线时）"""
    now = datetime.now(timezone.utc).isoformat()
    async with aiosqlite.connect(DB_PATH) as db:
        await db.execute(
            """INSERT OR IGNORE INTO offline_messages
               (msg_id, from_device, from_name, to_device, content, type, status, created_at)
               VALUES (?, ?, ?, ?, ?, ?, 'queued', ?)""",
            (msg_id, from_device, from_name, to_device, content, msg_type, now)
        )
        await db.commit()
    logger.info(f"离线消息已存储: {msg_id} {from_device} -> {to_device}")


async def get_pending_messages(device_id: str) -> List[dict]:
    """获取指定设备的积压离线消息"""
    async with aiosqlite.connect(DB_PATH) as db:
        db.row_factory = aiosqlite.Row
        cursor = await db.execute(
            """SELECT msg_id, from_device, from_name, content, type, status, created_at
               FROM offline_messages
               WHERE to_device = ? AND status = 'queued'
               ORDER BY id ASC""",
            (device_id,)
        )
        rows = await cursor.fetchall()
        return [dict(row) for row in rows]


async def mark_message_delivered(msg_id: str):
    """标记消息为已投递"""
    now = datetime.now(timezone.utc).isoformat()
    async with aiosqlite.connect(DB_PATH) as db:
        await db.execute(
            """UPDATE offline_messages
               SET status = 'delivered', delivered_at = ?
               WHERE msg_id = ?""",
            (now, msg_id)
        )
        await db.commit()
    logger.info(f"消息已投递: {msg_id}")


# ============================================================
# 天气缓存操作
# ============================================================

async def get_weather_cache(city: str) -> Optional[dict]:
    """获取缓存的天气数据"""
    async with aiosqlite.connect(DB_PATH) as db:
        db.row_factory = aiosqlite.Row
        cursor = await db.execute(
            "SELECT data_json, updated_at FROM weather_cache WHERE city = ?",
            (city,)
        )
        row = await cursor.fetchone()
        if row:
            return {
                "data": json.loads(row["data_json"]),
                "updated_at": row["updated_at"]
            }
        return None


async def set_weather_cache(city: str, data: dict):
    """更新天气缓存"""
    now = datetime.now(timezone.utc).isoformat()
    async with aiosqlite.connect(DB_PATH) as db:
        await db.execute(
            """INSERT OR REPLACE INTO weather_cache (city, data_json, updated_at)
               VALUES (?, ?, ?)""",
            (city, json.dumps(data, ensure_ascii=False), now)
        )
        await db.commit()
    logger.info(f"天气缓存已更新: {city}")


# ============================================================
# 设备在线状态
# ============================================================

async def set_device_online(device_id: str, online: bool):
    """设置设备在线状态"""
    now = datetime.now(timezone.utc).isoformat()
    async with aiosqlite.connect(DB_PATH) as db:
        await db.execute(
            """INSERT OR REPLACE INTO device_presence (device_id, online, last_seen)
               VALUES (?, ?, ?)""",
            (device_id, 1 if online else 0, now)
        )
        await db.commit()
    logger.info(f"设备 {device_id} {'上线' if online else '离线'}")


async def is_device_online(device_id: str) -> bool:
    """检查设备是否在线"""
    async with aiosqlite.connect(DB_PATH) as db:
        cursor = await db.execute(
            "SELECT online FROM device_presence WHERE device_id = ?",
            (device_id,)
        )
        row = await cursor.fetchone()
        return bool(row and row[0])
