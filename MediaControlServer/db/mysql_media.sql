SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

DROP TABLE IF EXISTS media_client_playing;
DROP TABLE IF EXISTS media_session_stream;
DROP TABLE IF EXISTS media_stream;

-- 1) 所有可播放流
CREATE TABLE media_stream (
    stream_id VARCHAR(64) NOT NULL COMMENT '流ID',
    url VARCHAR(1024) NOT NULL COMMENT '流播放URL',
    owner_id INT NOT NULL COMMENT '流所属客户端uid',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (stream_id),
    KEY idx_owner_id (owner_id),
    CONSTRAINT fk_media_stream_owner
        FOREIGN KEY (owner_id) REFERENCES `user`(uid)
        ON DELETE RESTRICT ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='可播放流信息表';

-- 2) 房间(session_id)管理的流
CREATE TABLE media_session_stream (
    session_id VARCHAR(64) NOT NULL COMMENT '房间号',
    stream_id VARCHAR(64) NOT NULL COMMENT '流ID',
    owner_id INT NOT NULL COMMENT '流所属客户端uid',
    state TINYINT NOT NULL DEFAULT 0 COMMENT '状态: 0=stopped,1=playing,2=paused',
    `count` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '当前房间内该流在线人数',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (session_id, stream_id),
    KEY idx_stream_id (stream_id),
    KEY idx_owner_id (owner_id),
    CONSTRAINT fk_media_session_stream_stream
        FOREIGN KEY (stream_id) REFERENCES media_stream(stream_id)
        ON DELETE CASCADE ON UPDATE CASCADE,
    CONSTRAINT fk_media_session_stream_owner
        FOREIGN KEY (owner_id) REFERENCES `user`(uid)
        ON DELETE RESTRICT ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='房间流管理表';

-- 3) 客户端正在播放的视频
-- 说明：为了解决“结束播放时无法定位到哪个session_id”的问题，这里补充了 session_id 和 uid。
-- 每个用户在同一房间仅保留一条当前播放记录，换流时更新 stream_id/state。
CREATE TABLE media_client_playing (
    session_id VARCHAR(64) NOT NULL COMMENT '房间号',
    uid INT NOT NULL COMMENT '客户端uid',
    stream_id VARCHAR(64) NOT NULL COMMENT '正在播放的流ID',
    state TINYINT NOT NULL DEFAULT 1 COMMENT '状态: 1=playing,2=paused,3=stopped',
    started_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (session_id, uid),
    KEY idx_stream_id (stream_id),
    CONSTRAINT fk_media_client_playing_session_stream
        FOREIGN KEY (session_id, stream_id) REFERENCES media_session_stream(session_id, stream_id)
        ON DELETE CASCADE ON UPDATE CASCADE,
    CONSTRAINT fk_media_client_playing_uid
        FOREIGN KEY (uid) REFERENCES `user`(uid)
        ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='客户端当前播放表';

-- media_stream 测试数据（仅此表）
INSERT INTO media_stream (stream_id, url, owner_id) VALUES
('stream_local_01', 'file:///E:/word/AudioandVideo/sample-30s.mp4', 10001),
('stream_local_02', 'file:///E:/word/AudioandVideo/test-01.mp4', 10001),
('stream_local_03', 'file:///E:/word/AudioandVideo/test-02.mp4', 10001),
('stream_local_04', 'file:///E:/word/AudioandVideo/test-03.mp4', 10001),
('stream_hls_01', 'https://test-streams.mux.dev/x36xhzz/x36xhzz.m3u8', 10001),
('stream_hls_02', 'https://devstreaming-cdn.apple.com/videos/streaming/examples/img_bipbop_adv_example_ts/master.m3u8', 10001),
('stream_rtsp_01', 'rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov', 10001);

SET FOREIGN_KEY_CHECKS = 1;
