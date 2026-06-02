/* MathJax loader
 * - Detect whether the page actually contains math
 * - Load MathJax from a CDN with fallback
 * - Keep the HTML include as small as possible
 */

async function loadScript(src) {
    return new Promise((resolve, reject) => {
        const script = document.createElement("script");
        script.src = src;
        script.async = true;
        script.onload = () => resolve(script);
        script.onerror = () => reject(new Error(`Failed to load ${src}`));
        document.head.appendChild(script);
    });
}

function hasMathInText(text) {
    // Common TeX/MathJax syntaxes we use on the blog.
    const patterns = [
        /\$\s*(?:[^$\n\\]|\\.)+?\s*\$/,
        /\\\(/,
        /\\\[/,
        /\$\$/,
        /\\begin\{[a-zA-Z*]+\}/
    ];

    return patterns.some((pattern) => pattern.test(text));
}

function hasMathInDocument() {
    const root = document.body || document.documentElement;
    if (!root) {
        return false;
    }

    const walker = document.createTreeWalker(
        root,
        NodeFilter.SHOW_TEXT,
        {
            acceptNode(node) {
                const parent = node.parentElement;
                if (!parent) {
                    return NodeFilter.FILTER_SKIP;
                }

                if (["SCRIPT", "STYLE", "NOSCRIPT"].includes(parent.tagName)) {
                    return NodeFilter.FILTER_SKIP;
                }

                return NodeFilter.FILTER_ACCEPT;
            }
        }
    );

    let currentNode = walker.nextNode();
    while (currentNode) {
        if (hasMathInText(currentNode.textContent || "")) {
            return true;
        }
        currentNode = walker.nextNode();
    }

    return false;
}

async function bootstrapMathJax() {
    const loadMode = window.__MATHJAX_LOAD_MODE__ || "auto";
    const shouldLoad = loadMode === "force_enable" || hasMathInDocument();

    if (!shouldLoad) {
        console.info("This page does not need MathJax.");
        return;
    }

    console.info("> Loading MathJax");
    window.MathJax = window.MathJax || {
        tex: {
            inlineMath: [["$", "$"]]
        }
    };

    const mathjaxSources = [
        "https://cdn.jsdelivr.net/npm/mathjax@4/tex-chtml.js",
        "https://cdnjs.cloudflare.com/ajax/libs/mathjax/4.0.0/tex-chtml.min.js"
    ];

    for (let index = 0; index < mathjaxSources.length; index += 1) {
        try {
            await loadScript(mathjaxSources[index]);
            return;
        } catch (error) {
            if (index + 1 < mathjaxSources.length) {
                console.warn("Primary MathJax CDN failed, falling back to cdnjs.");
                continue;
            }
            console.error("Failed to load MathJax from all CDNs.", error);
        }
    }
}

bootstrapMathJax();
