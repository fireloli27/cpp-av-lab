/** Minimal quiz helper for file:// lessons (no ES modules). */
(function () {
  function wireQuiz(root) {
    var form = root.querySelector("form");
    var feedback = root.querySelector(".feedback");
    if (!form || !feedback) return;

    form.addEventListener("submit", function (event) {
      event.preventDefault();
      var selected = form.querySelector('input[type="radio"]:checked');
      if (!selected) {
        feedback.textContent = "先选一个再提交。";
        feedback.className = "feedback bad";
        return;
      }
      var correct = form.getAttribute("data-correct");
      if (selected.value === correct) {
        feedback.textContent = form.getAttribute("data-ok") || "对。";
        feedback.className = "feedback ok";
      } else {
        feedback.textContent = form.getAttribute("data-bad") || "再想一想。";
        feedback.className = "feedback bad";
      }
    });
  }

  document.querySelectorAll("[data-quiz]").forEach(wireQuiz);
})();
