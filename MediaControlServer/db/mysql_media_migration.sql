-- Media schema (minimal but interview-ready)
-- Created: 2026-05-11

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

DROP TABLE IF EXISTS media_play_state;
DROP TABLE IF EXISTS media_session_member;
DROP TABLE IF EXISTS media_session;
DROP TABLE IF EXISTS media_stream;

CREATE TABLE media_stream (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '自增主键',
    stream_id VARCHAR(64) NOT NULL COMMENT '业务流ID（给客户端/接口使用）',
    name VARCHAR(128) NOT NULL COMMENT '流显示名称',
    url VARCHAR(1024) NOT NULL COMMENT '实际播放地址（RTSP/HLS/本地文件URL）',
    source_type TINYINT NOT NULL DEFAULT 0 COMMENT '流类型: 1=RTSP,2=HLS,3=FILE,4=HTTP',
    status TINYINT NOT NULL DEFAULT 1 COMMENT '状态: 1=online,2=offline,3=disabled',
    owner_uid BIGINT NOT NULL DEFAULT 0 COMMENT '创建者/维护者uid',
    has_audio TINYINT(1) NOT NULL DEFAULT 1 COMMENT '是否有音频',
    has_video TINYINT(1) NOT NULL DEFAULT 1 COMMENT '是否有视频',
    video_codec VARCHAR(32) NULL DEFAULT NULL COMMENT '视频编码（如h264/h265）',
    audio_codec VARCHAR(32) NULL DEFAULT NULL COMMENT '音频编码（如aac/opus）',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (id),
    UNIQUE KEY uk_stream_id (stream_id),
    KEY idx_owner_uid (owner_uid),
    KEY idx_source_status (source_type, status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='媒体流资源表';

CREATE TABLE media_session (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '自增主键',
    session_id VARCHAR(64) NOT NULL COMMENT '业务会话ID',
    session_name VARCHAR(128) NOT NULL COMMENT '会话名称',
    master_uid BIGINT NOT NULL COMMENT '主控用户uid',
    layout_mode TINYINT NOT NULL DEFAULT 1 COMMENT '布局模式: 1=1x1,2=2x2,3=3x3',
    sync_mode TINYINT NOT NULL DEFAULT 1 COMMENT '同步模式: 1=独立,2=主控同步',
    status TINYINT NOT NULL DEFAULT 1 COMMENT '会话状态: 1=active,2=closed',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (id),
    UNIQUE KEY uk_session_id (session_id),
    KEY idx_master_uid (master_uid),
    KEY idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='媒体播放会话表';

CREATE TABLE media_session_member (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '自增主键',
    session_id VARCHAR(64) NOT NULL COMMENT '所属会话ID',
    uid BIGINT NOT NULL COMMENT '用户uid',
    role TINYINT NOT NULL DEFAULT 2 COMMENT '角色: 1=master,2=member',
    online TINYINT(1) NOT NULL DEFAULT 1 COMMENT '是否在线',
    join_time DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '加入时间',
    last_heartbeat_at DATETIME NULL DEFAULT NULL COMMENT '最近心跳时间',
    PRIMARY KEY (id),
    UNIQUE KEY uk_session_uid (session_id, uid),
    KEY idx_session_online (session_id, online),
    CONSTRAINT fk_member_session FOREIGN KEY (session_id) REFERENCES media_session(session_id)
      ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='会话成员表';

CREATE TABLE media_play_state (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '自增主键',
    session_id VARCHAR(64) NOT NULL COMMENT '所属会话ID',
    slot_index INT NOT NULL COMMENT '布局槽位索引(从0开始)',
    stream_id VARCHAR(64) NULL DEFAULT NULL COMMENT '当前槽位播放流ID',
    play_state TINYINT NOT NULL DEFAULT 0 COMMENT '播放状态: 0=idle,1=playing,2=paused,3=buffering,4=stopped',
    position_ms BIGINT NOT NULL DEFAULT 0 COMMENT '当前播放进度毫秒',
    is_sync_controlled TINYINT(1) NOT NULL DEFAULT 0 COMMENT '该槽位是否受主控同步',
    volume INT NOT NULL DEFAULT 100 COMMENT '音量0-100',
    updated_by_uid BIGINT NOT NULL DEFAULT 0 COMMENT '最后修改者uid',
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '状态更新时间',
    PRIMARY KEY (id),
    UNIQUE KEY uk_session_slot (session_id, slot_index),
    KEY idx_stream_id (stream_id),
    CONSTRAINT fk_state_session FOREIGN KEY (session_id) REFERENCES media_session(session_id)
      ON DELETE CASCADE ON UPDATE CASCADE,
    CONSTRAINT fk_state_stream FOREIGN KEY (stream_id) REFERENCES media_stream(stream_id)
      ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='会话槽位播放状态表';

-- =========================
-- Seed data
-- =========================
INSERT INTO media_stream
(stream_id, name, url, source_type, status, owner_uid, has_audio, has_video, video_codec, audio_codec)
VALUES
('stream_rtsp_bbb', 'RTSP BigBuckBunny', 'rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov', 1, 1, 10001, 1, 1, 'h264', 'aac'),
('stream_hls_mux', 'HLS Mux Demo', 'https://test-streams.mux.dev/x36xhzz/x36xhzz.m3u8', 2, 1, 10001, 1, 1, 'h264', 'aac'),
('stream_hls_apple', 'HLS Apple BipBop', 'https://devstreaming-cdn.apple.com/videos/streaming/examples/img_bipbop_adv_example_ts/master.m3u8', 2, 1, 10001, 1, 1, 'h264', 'aac'),
('stream_hls_tos', 'HLS TearsOfSteel', 'https://demo.unified-streaming.com/k8s/features/stable/video/tears-of-steel/tears-of-steel.ism/.m3u8', 2, 1, 10001, 1, 1, 'h264', 'aac'),
('stream_file_01', 'Local Sample 30s', 'file:///E:/word/AudioandVideo/sample-30s.mp4', 3, 1, 10001, 1, 1, 'h264', 'aac'),
('stream_file_02', 'Local Test 01', 'file:///E:/word/AudioandVideo/test-01.mp4', 3, 1, 10001, 1, 1, 'h264', 'aac'),
('stream_file_03', 'Local Test 02', 'file:///E:/word/AudioandVideo/test-02.mp4', 3, 1, 10001, 1, 1, 'h264', 'aac'),
('stream_file_04', 'Local Test 03', 'file:///E:/word/AudioandVideo/test-03.mp4', 3, 1, 10001, 1, 1, 'h264', 'aac');

INSERT INTO media_session
(session_id, session_name, master_uid, layout_mode, sync_mode, status)
VALUES
('room_default', 'Default Room', 10001, 2, 2, 1),
('room_demo_01', 'Demo Room 01', 10002, 1, 1, 1);

INSERT INTO media_session_member
(session_id, uid, role, online, last_heartbeat_at)
VALUES
('room_default', 10001, 1, 1, NOW()),
('room_default', 10002, 2, 1, NOW()),
('room_default', 10003, 2, 0, DATE_SUB(NOW(), INTERVAL 10 MINUTE)),
('room_demo_01', 10002, 1, 1, NOW());

INSERT INTO media_play_state
(session_id, slot_index, stream_id, play_state, position_ms, is_sync_controlled, volume, updated_by_uid)
VALUES
('room_default', 0, 'stream_hls_mux', 1, 128000, 1, 85, 10001),
('room_default', 1, 'stream_file_02', 2, 45000, 0, 70, 10002),
('room_default', 2, NULL, 0, 0, 0, 100, 10001),
('room_default', 3, NULL, 0, 0, 0, 100, 10001),
('room_demo_01', 0, 'stream_rtsp_bbb', 1, 30000, 0, 90, 10002);

SET FOREIGN_KEY_CHECKS = 1;
