(function () {
  function toMermaidBlocks() {
    const codeBlocks = document.querySelectorAll('pre code.language-mermaid');
    codeBlocks.forEach((code) => {
      const pre = code.parentElement;
      if (!pre) return;

      const mermaidBlock = document.createElement('pre');
      mermaidBlock.className = 'mermaid';
      mermaidBlock.textContent = code.textContent || '';
      pre.replaceWith(mermaidBlock);
    });
  }

  function detectTheme() {
    const htmlClasses = document.documentElement.classList;
    const darkThemes = ['ayu', 'navy', 'coal'];
    for (const darkTheme of darkThemes) {
      if (htmlClasses.contains(darkTheme)) return 'dark';
    }
    return 'default';
  }

  function initMermaid() {
    toMermaidBlocks();

    if (!window.mermaid) return;
    window.mermaid.initialize({
      startOnLoad: true,
      theme: detectTheme(),
      securityLevel: 'loose'
    });

    // Re-render after converting code blocks.
    window.mermaid.run({ querySelector: '.mermaid' });
  }

  function loadMermaidAndInit() {
    const script = document.createElement('script');
    script.src = 'https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.min.js';
    script.onload = initMermaid;
    script.defer = true;
    document.head.appendChild(script);
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', loadMermaidAndInit);
  } else {
    loadMermaidAndInit();
  }
})();
