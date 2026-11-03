(function () {
  var scenarios = {
    steady: {
      label: "稳定 30 fps",
      pts: [0, 33333333, 66666667, 100000000, 133333334]
    },
    dropped: {
      label: "第 3 帧前丢了一帧",
      pts: [0, 33333333, 66666667, 133333334, 166666667]
    },
    jitter: {
      label: "到达节奏抖动",
      pts: [0, 28000000, 71000000, 104000000, 151000000]
    }
  };

  function ms(ns) {
    return (ns / 1000000).toFixed(1) + " ms";
  }

  function render(root, key) {
    var scenario = scenarios[key];
    var first = scenario.pts[0];
    var max = scenario.pts[scenario.pts.length - 1] - first;
    var body = root.querySelector("[data-timeline-rows]");
    var track = root.querySelector("[data-timeline-track]");
    var summary = root.querySelector("[data-timeline-summary]");

    body.replaceChildren();
    track.replaceChildren();

    scenario.pts.forEach(function (pts, index) {
      var relative = pts - first;
      var delta = index === 0 ? null : pts - scenario.pts[index - 1];
      var isGap = delta !== null && delta > 50000000;
      var row = document.createElement("tr");
      if (isGap) row.className = "is-gap";
      [String(index), String(pts), String(relative), delta === null ? "—" : ms(delta)].forEach(function (value) {
        var cell = document.createElement("td");
        cell.textContent = value;
        row.appendChild(cell);
      });
      body.appendChild(row);

      var dot = document.createElement("span");
      dot.className = "timeline-dot";
      dot.style.left = String(max === 0 ? 0 : (relative / max) * 100) + "%";
      dot.textContent = "f" + index;
      track.appendChild(dot);
    });

    root.querySelectorAll("[data-timeline-scenario]").forEach(function (button) {
      button.setAttribute("aria-pressed", button.dataset.timelineScenario === key ? "true" : "false");
    });

    var gaps = [];
    for (var i = 1; i < scenario.pts.length; i += 1) {
      var deltaNs = scenario.pts[i] - scenario.pts[i - 1];
      if (deltaNs > 50000000) gaps.push("frame_index=" + i + " 前出现 " + ms(deltaNs) + " 时间差");
    }
    summary.textContent = scenario.label + "：" + (gaps.length ? gaps.join("；") : "PTS 间隔接近 30 fps 的 33.3 ms") + "。";
  }

  document.querySelectorAll("[data-pts-timeline]").forEach(function (root) {
    root.querySelectorAll("[data-timeline-scenario]").forEach(function (button) {
      button.addEventListener("click", function () {
        render(root, button.dataset.timelineScenario);
      });
    });
    render(root, "steady");
  });
})();
