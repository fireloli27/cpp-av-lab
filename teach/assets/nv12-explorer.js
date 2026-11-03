(function () {
  function asInteger(input, fallback) {
    var value = Number.parseInt(input.value, 10);
    return Number.isFinite(value) ? value : fallback;
  }

  function format(value) {
    return new Intl.NumberFormat("en-US").format(value);
  }

  function render(root) {
    var widthInput = root.querySelector("[data-nv12-width]");
    var heightInput = root.querySelector("[data-nv12-height]");
    var yStrideInput = root.querySelector("[data-nv12-y-stride]");
    var uvStrideInput = root.querySelector("[data-nv12-uv-stride]");

    var width = Math.max(2, asInteger(widthInput, 1280));
    var height = Math.max(2, asInteger(heightInput, 720));
    if (width % 2 !== 0) width += 1;
    if (height % 2 !== 0) height += 1;

    var uvWidth = Math.ceil(width / 2);
    var uvHeight = Math.ceil(height / 2);
    var yStride = Math.max(width, asInteger(yStrideInput, width));
    var uvStride = Math.max(uvWidth * 2, asInteger(uvStrideInput, uvWidth * 2));

    widthInput.value = String(width);
    heightInput.value = String(height);
    yStrideInput.value = String(yStride);
    uvStrideInput.value = String(uvStride);

    var yBytes = width * height;
    var uvBytes = uvWidth * uvHeight * 2;
    var tightBytes = yBytes + uvBytes;
    var allocatedBytes = yStride * height + uvStride * uvHeight;
    var paddingBytes = allocatedBytes - tightBytes;

    root.querySelector("[data-nv12-y-bytes]").textContent = format(yBytes) + " B";
    root.querySelector("[data-nv12-uv-bytes]").textContent = format(uvBytes) + " B";
    root.querySelector("[data-nv12-frame-bytes]").textContent = format(tightBytes) + " B";
    root.querySelector("[data-nv12-allocated]").textContent = format(allocatedBytes) + " B";
    root.querySelector("[data-nv12-padding]").textContent = format(paddingBytes) + " B";
    root.querySelector("[data-nv12-shape]").textContent = width + " × " + height;
    root.querySelector("[data-nv12-uv-shape]").textContent = uvWidth + " × " + uvHeight + " UV pairs";

    var note = root.querySelector("[data-nv12-note]");
    if (paddingBytes > 0) {
      note.textContent = "分配区比 tight 帧多 " + format(paddingBytes) + " B；复制时按可见宽度逐行处理。";
    } else {
      note.textContent = "当前 stride 正好紧排；真实 CVPixelBuffer 仍要读取 API 返回值，不能假定如此。";
    }
  }

  document.querySelectorAll("[data-nv12-lab]").forEach(function (root) {
    root.querySelectorAll("input").forEach(function (input) {
      input.addEventListener("input", function () { render(root); });
    });
    root.querySelectorAll("[data-nv12-preset]").forEach(function (button) {
      button.addEventListener("click", function () {
        root.querySelector("[data-nv12-width]").value = button.dataset.width;
        root.querySelector("[data-nv12-height]").value = button.dataset.height;
        root.querySelector("[data-nv12-y-stride]").value = button.dataset.yStride;
        root.querySelector("[data-nv12-uv-stride]").value = button.dataset.uvStride;
        render(root);
      });
    });
    render(root);
  });
})();
