"""
LoveBeacon MQTT 桥接模块
gmqtt 异步客户端：订阅设备消息 → 路由转发 → 离线存储
"""

import asyncio
import json
import logging
import uuid
from datetime import datetime, timezone
from typing import Optional, Callable, Awaitable

from gmqtt import Client as MQTTClient
from gmqtt.mqtt.constants import MQTTv311

from models import (
    MessagePayload, MessageReceive, StatusPayload, StatusNotify,
    HeartbeatPayload, HeartbeatNotify, PresenceNotify,
    STATUS_EMOJI_MAP, STATUS_LABEL_MAP,
)
from database import (
    store_offline_message, get_pending_messages, mark_message_delivered,
    set_device_online, is_device_online,
)

logger = logging.getLogger("LoveBeacon.MQTT")


def load_config():
    with open("config.json", "r", encoding="utf-8") as f:
        return json.load(f)


class MQTTBridge:
    """MQTT桥接 — 连接 Mosquitto，订阅设备主题，实现消息路由"""

    def __init__(self):
        self.config = load_config()
        self.mqtt_cfg = self.config["mqtt"]
        self.devices_cfg = self.config["devices"]

        self.client = MQTTClient(self.mqtt_cfg["client_id"])
        self._connected = False
        self._topic_callbacks: dict[str, Callable] = {}

        # 注册回调
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message
        self.client.on_disconnect = self._on_disconnect
        self.client.on_subscribe = self._on_subscribe

    async def start(self):
        """连接MQTT Broker"""
        try:
            await self.client.connect(
                host=self.mqtt_cfg["broker_host"],
                port=self.mqtt_cfg["broker_port"],
                keepalive=self.mqtt_cfg["keepalive"],
                version=MQTTv311,
            )
            logger.info(f"MQTT桥接已连接到 {self.mqtt_cfg['broker_host']}:{self.mqtt_cfg['broker_port']}")
        except Exception as e:
            logger.error(f"MQTT连接失败: {e}")
            raise

    async def stop(self):
        """断开MQTT连接"""
        if self._connected:
            await self.client.disconnect()
            logger.info("MQTT桥接已断开")

    async def publish(self, topic: str, payload: str, qos: int = 1, retain: bool = False):
        """发布MQTT消息"""
        if not self._connected:
            logger.warning(f"MQTT未连接，无法发布: {topic}")
            return
        self.client.publish(topic, payload, qos=qos, retain=retain)
        logger.debug(f"MQTT发布: {topic} <- {payload[:80]}")

    async def _on_connect(self, client, flags, rc, properties):
        """MQTT连接成功回调"""
        self._connected = True
        logger.info("MQTT桥接已连接")

        # 订阅所有设备的上行主题
        topics = [
            # 消息通道
            ("lovebeacon/+/message/send", 1),
            # 状态通道
            ("lovebeacon/+/status", 1),
            # 心跳通道
            ("lovebeacon/+/heartbeat", 1),
            # 在线状态（LWT）
            ("lovebeacon/+/presence", 1),
        ]
        for topic, qos in topics:
            self.client.subscribe(topic, qos)
            logger.info(f"已订阅: {topic} (QoS {qos})")

    async def _on_message(self, client, topic, payload, qos, properties):
        """收到MQTT消息回调"""
        topic_str = str(topic)
        payload_str = payload.decode("utf-8")
        logger.debug(f"MQTT收到: {topic_str} <- {payload_str[:80]}")

        try:
            data = json.loads(payload_str)
        except json.JSONDecodeError:
            logger.error(f"JSON解析失败: {topic_str}")
            return

        # 解析来源设备ID
        parts = topic_str.split("/")
        if len(parts) < 3:
            logger.error(f"无法解析topic: {topic_str}")
            return
        from_device = parts[1]  # lovebeacon/{device_id}/...

        # 查找对方设备ID
        partner_device = self._get_partner(from_device)
        if not partner_device:
            logger.warning(f"未知设备: {from_device}")
            return

        # 根据topic类型分发处理
        if topic_str.endswith("/message/send"):
            await self._handle_message(from_device, partner_device, data)
        elif topic_str.endswith("/status"):
            await self._handle_status(from_device, partner_device, data)
        elif topic_str.endswith("/heartbeat"):
            await self._handle_heartbeat(from_device, partner_device, data)
        elif topic_str.endswith("/presence"):
            await self._handle_presence(from_device, partner_device, data)

    async def _on_disconnect(self, client, packet, exc=None):
        """MQTT断开回调"""
        self._connected = False
        if exc:
            logger.warning(f"MQTT断开: {exc}")
        else:
            logger.info("MQTT正常断开")

    async def _on_subscribe(self, client, mid, qos, properties):
        """订阅确认回调"""
        logger.debug(f"订阅确认: mid={mid} qos={qos}")

    def _get_partner(self, device_id: str) -> Optional[str]:
        """获取对方设备ID"""
        if device_id == "deviceA":
            return "deviceB"
        elif device_id == "deviceB":
            return "deviceA"
        return None

    # ============================================================
    # 消息处理
    # ============================================================

    async def _handle_message(self, from_device: str, to_device: str, data: dict):
        """处理设备上行消息：转发给对方，离线则缓存"""
        try:
            msg = MessagePayload(**data)
        except Exception as e:
            logger.error(f"消息解析失败: {e}")
            return

        from_name = self.devices_cfg.get(from_device, {}).get("name", from_device)

        # 构造下行消息
        receive_msg = MessageReceive(
            msg_id=msg.msg_id,
            from_device=from_device,
            from_name=from_name,
            content=msg.content,
            type=msg.type,
            timestamp=msg.timestamp,
        )
        payload = receive_msg.model_dump_json()
        topic = f"lovebeacon/{to_device}/message/receive"

        # 检查对方是否在线
        online = await is_device_online(to_device)
        if online:
            # 在线：直接推送
            await self.publish(topic, payload)
            logger.info(f"消息已转发: {from_device} -> {to_device} [在线]")
        else:
            # 离线：缓存
            await store_offline_message(
                msg_id=msg.msg_id,
                from_device=from_device,
                from_name=from_name,
                to_device=to_device,
                content=msg.content,
                msg_type=msg.type,
            )
            logger.info(f"消息已缓存(对方离线): {from_device} -> {to_device}")

    async def _handle_status(self, from_device: str, partner_device: str, data: dict):
        """处理状态更新：转发给对方设备"""
        try:
            status = StatusPayload(**data)
        except Exception as e:
            logger.error(f"状态解析失败: {e}")
            return

        emoji = STATUS_EMOJI_MAP.get(status.status, "❓")
        label = STATUS_LABEL_MAP.get(status.status, "未知")

        notify = StatusNotify(
            from_device=from_device,
            status=status.status,
            emoji=emoji,
            label=label,
            timestamp=status.timestamp,
        )
        topic = f"lovebeacon/{partner_device}/status/notify"
        await self.publish(topic, notify.model_dump_json())
        logger.info(f"状态已转发: {from_device} [{emoji} {label}] -> {partner_device}")

    async def _handle_heartbeat(self, from_device: str, partner_device: str, data: dict):
        """处理心跳：转发给对方设备"""
        try:
            hb = HeartbeatPayload(**data)
        except Exception as e:
            logger.error(f"心跳解析失败: {e}")
            return

        from_name = self.devices_cfg.get(from_device, {}).get("name", from_device)

        notify = HeartbeatNotify(
            from_device=from_device,
            from_name=from_name,
            timestamp=hb.timestamp,
        )
        topic = f"lovebeacon/{partner_device}/heartbeat/notify"
        await self.publish(topic, notify.model_dump_json())
        logger.info(f"心跳已转发: {from_device} ❤️ -> {partner_device}")

    async def _handle_presence(self, from_device: str, partner_device: str, data: dict):
        """处理设备上下线"""
        online = data.get("online", False)
        await set_device_online(from_device, online)

        # 通知对方设备
        from_name = self.devices_cfg.get(from_device, {}).get("name", from_device)
        notify = PresenceNotify(
            device_id=from_device,
            device_name=from_name,
            online=online,
            timestamp=datetime.now(timezone.utc).timestamp(),
        )
        topic = f"lovebeacon/{partner_device}/presence/notify"
        await self.publish(topic, notify.model_dump_json())

        logger.info(f"设备 {from_device} ({from_name}) {'上线' if online else '离线'}")

        # 设备上线时：推送积压的离线消息
        if online:
            await self._push_pending_messages(from_device)

    async def _push_pending_messages(self, device_id: str):
        """推送积压的离线消息"""
        pending = await get_pending_messages(device_id)
        if not pending:
            logger.info(f"设备 {device_id} 无积压消息")
            return

        logger.info(f"推送 {len(pending)} 条积压消息给 {device_id}")
        for msg in pending:
            receive_msg = MessageReceive(
                msg_id=msg["msg_id"],
                from_device=msg["from_device"],
                from_name=msg["from_name"],
                content=msg["content"],
                type=msg["type"],
                timestamp=0,  # 离线消息时间戳不重要
            )
            topic = f"lovebeacon/{device_id}/message/receive"
            await self.publish(topic, receive_msg.model_dump_json())
            await mark_message_delivered(msg["msg_id"])
            # 间隔发送，避免设备来不及处理
            await asyncio.sleep(0.3)

        logger.info(f"积压消息已全部推送给 {device_id}")
