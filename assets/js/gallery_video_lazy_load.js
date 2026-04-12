document.addEventListener("DOMContentLoaded", function() {
  // 创建观察器
  const videoObserver = new IntersectionObserver((entries, observer) => {
    entries.forEach(entry => {
      if (entry.isIntersecting) {
        const video = entry.target;
        // 将 data-src 的值赋给 src，触发真正的加载
        video.src = video.dataset.src;
        video.load(); // 强制加载
        video.play(); // 播放
        // 加载后就停止观察该元素，节省性能
        observer.unobserve(video);
      }
    });
  }, {
    rootMargin: "200px", // 提前 200 像素开始加载，用户感觉不到延迟
    threshold: 0.1
  });

  // 监听所有带 .lazy-video 类的视频
  const lazyVideos = document.querySelectorAll(".lazy-video");
  lazyVideos.forEach(video => videoObserver.observe(video));
});