"""
LoveBeacon 数据模型
定义消息、状态、心跳、天气等数据结构
"""

from pydantic import BaseModel, Field
from typing import Optional, Literal
from datetime import datetime
import uuid


# ============================================================
# 消息相关模型
# ============================================================

class MessagePayload(BaseModel):
    """设备上行消息（发给服务器）"""
    msg_id: str = Field(default_factory=lambda: str(uuid.uuid4())[:8])
    to: str = Field(..., description="目标设备ID: deviceA / deviceB")
    content: str = Field(..., description="消息内容（预设key或自定义文本）")
    type: Literal["msg", "preset"] = Field("msg", description="消息类型")
    timestamp: int = Field(..., description="Unix时间戳（秒）")


class MessageReceive(BaseModel):
    """服务端下行消息（推送给目标设备）"""
    msg_id: str
    from_device: str = Field(..., description="来源设备ID")
    from_name: str = Field(..., description="来源设备昵称")
    content: str
    type: str = "msg"
    timestamp: int
    server_ts: float = Field(default_factory=lambda: datetime.utcnow().timestamp())


# ============================================================
# 状态相关模型
# ============================================================

STATUS_OPTIONS = [
    {"key": "play",    "emoji": "🎮", "label": "出去玩"},
    {"key": "study",   "emoji": "📚", "label": "学习中"},
    {"key": "relax",   "emoji": "🛋️", "label": "休闲"},
    {"key": "sleep",   "emoji": "😴", "label": "睡觉"},
    {"key": "love",    "emoji": "💕", "label": "想你"},
    {"key": "eat",     "emoji": "🍽️", "label": "吃饭"},
]

STATUS_EMOJI_MAP = {s["key"]: s["emoji"] for s in STATUS_OPTIONS}
STATUS_LABEL_MAP = {s["key"]: s["label"] for s in STATUS_OPTIONS}


class StatusPayload(BaseModel):
    """设备上行状态更新"""
    status: str = Field(..., description="状态key: play/study/relax/sleep/love/eat")
    timestamp: int = Field(..., description="Unix时间戳（秒）")


class StatusNotify(BaseModel):
    """服务端下行状态通知（推送给对方设备）"""
    from_device: str
    status: str
    emoji: str
    label: str
    timestamp: int


# ============================================================
# 心跳相关模型
# ============================================================

class HeartbeatPayload(BaseModel):
    """设备上行心跳"""
    timestamp: int = Field(..., description="Unix时间戳（秒）")


class HeartbeatNotify(BaseModel):
    """服务端下行心跳通知（推送给对方设备）"""
    from_device: str
    from_name: str
    timestamp: int


# ============================================================
# 天气相关模型
# ============================================================

class WeatherData(BaseModel):
    """天气数据结构"""
    city: str
    temperature: float          # 温度（摄氏度）
    feels_like: float           # 体感温度
    humidity: int               # 湿度 %
    description: str            # 天气描述（中文）
    icon_code: str              # OpenWeatherMap图标代码 (如 "01d")
    wind_speed: float           # 风速 m/s
    updated_at: str             # 更新时间
    sunrise: int                # 日出时间戳
    sunset: int                 # 日落时间戳


class WeatherNotify(BaseModel):
    """服务端下行天气通知（推送给设备）"""
    city: str
    city_name: str              # 城市中文名
    temperature: float
    description: str
    icon_code: str
    is_day: bool                # 是否白天
    updated_at: str


# ============================================================
# 在线状态
# ============================================================

class PresenceNotify(BaseModel):
    """上下线通知"""
    device_id: str
    device_name: str
    online: bool
    timestamp: float


# ============================================================
# 离线消息存储模型
# ============================================================

class StoredMessage(BaseModel):
    """SQLite中存储的离线消息"""
    id: Optional[int] = None
    msg_id: str
    from_device: str
    from_name: str
    to_device: str
    content: str
    type: str = "msg"
    status: Literal["queued", "delivered"] = "queued"
    created_at: str
    delivered_at: Optional[str] = None
