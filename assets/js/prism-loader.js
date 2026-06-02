/* Prism loader
 * - Detect whether code blocks exist
 * - Preprocess code blocks before Prism highlights them
 * - Load Prism core with a local autoloader path
 */

function hasCodeBlocks() {
    return document.querySelector("pre code") !== null;
}

function preprocessCodeBlocks() {
    const preElements = document.getElementsByTagName("pre");
    Array.from(preElements).forEach((preElement, idx) => {
        const hasLang = (preElement.getAttribute("class") || "").includes("lang");
        if (!hasLang) {
            // Enable Prism and keep plain text blocks from being skipped.
            preElement.classList.add("language-plain");
        }

        const lineNum = (preElement.textContent || "").split("\n").length;
        if (lineNum > 5) {
            preElement.classList.add("line-numbers", "linkable-line-numbers");
        } else {
            preElement.classList.add("no-line-numbers");
        }

        const wrapStyle = "white-space:pre-wrap;";
        preElement.setAttribute("style", wrapStyle);
        Array.from(preElement.getElementsByTagName("code")).forEach((ele) => {
            ele.setAttribute("style", wrapStyle);
        });

        preElement.setAttribute("id", `pre-code-${idx}`);
    });
}

function addStylesheet(href) {
    const link = document.createElement("link");
    link.rel = "stylesheet";
    link.type = "text/css";
    link.href = href;
    document.head.appendChild(link);
}

function loadScript(src, attrs) {
    return new Promise((resolve, reject) => {
        const script = document.createElement("script");
        script.src = src;
        script.async = true;
        if (attrs) {
            for (const [key, value] of Object.entries(attrs)) {
                script.setAttribute(key, value);
            }
        }
        script.onload = () => resolve(script);
        script.onerror = () => reject(new Error(`Failed to load ${src}`));
        document.head.appendChild(script);
    });
}

(function bootstrapPrism() {
    if (!hasCodeBlocks()) {
        console.info("This page does not need to load Prism");
        return;
    }

    preprocessCodeBlocks();
    console.info("> Loading Prism");

    const config = window.__PRISM_LOADER_CONFIG__ || {};
    const prismJsPath = config.prismJsPath;
    const prismCssPath = config.prismCssPath;
    const componentsPath = config.prismComponentsPath;

    if (prismCssPath) {
        addStylesheet(prismCssPath);
    }

    if (!prismJsPath) {
        console.error("Prism loader is missing prism.js path.");
        return;
    }

    window.Prism = window.Prism || {};
    window.Prism.manual = true;

    loadScript(prismJsPath, componentsPath ? {
        "data-autoloader-path": componentsPath
    } : undefined).then(() => {
        if (typeof Prism !== "undefined" && typeof Prism.highlightAll === "function") {
            Prism.highlightAll();
        }
    }).catch((error) => {
        console.error("Failed to load Prism core.", error);
    });
})();
