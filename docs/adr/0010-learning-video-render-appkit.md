# Learning video render uses an AppKit window

Learning Project 视频「渲染」阶段在 macOS 上用 **AppKit** 的 `NSWindow` + `NSImageView`：把解码目录里的 tight NV12 `420v` 经 Core Image 变成 `NSImage`，按 manifest 的 `relative_pts_ns` 逐帧交给窗口。不用 `AVSampleBufferDisplayLayer`，不调用外部查看器去打开 `preview.png`，不引入第三方媒体库。理由与 ADR-0005 对称：要看见「帧字节被交给显示对象」这一步，而不是把显示藏进播放器或 sample-buffer 层。

窗口内容区是 960×540 point，图像按比例缩放；源像素仍是 1280×720，不在渲染时改分辨率。成功判据是 `frames_loaded == frames_presented`（交给 `NSImageView` 的帧数）加上人眼能看见窗口。Catch2 不创建窗口。间隔超过 200 ms 的 PTS 会被夹住，避免一次时间戳跳跃把窗口卡住；这不是 jitter buffer。A/V 同步、jitter buffer、AEC 只叙述，不在本阶段实现。本实验只放视频，没有音频主钟。
