/* =============================================================================
   TOC JS
   1. 工具函数：rAF 节流
   2. TOC 生成：createToc（纯 JS 版）
   3. TOC 双栏布局：splitContentToc2Columns（精简版平衡算法）
   4. 侧边栏交互：显示/隐藏 + 高亮当前项 + 自动滚动
   5. 初始化入口：initToc
   ============================================================================= */

// ============================================================
// Section 1: 工具函数
// ============================================================

// 使用 requestAnimationFrame 进行高效的事件节流，适用于 scroll 和 resize
const throttleRAF = (fn) => {
    let ticking = false;
    return (...args) => {
        if (!ticking) {
            requestAnimationFrame(() => {
                fn(...args);
                ticking = false;
            });
            ticking = true;
        }
    };
};


// ============================================================
// Section 2: TOC 生成
// ============================================================

function createToc(container, options) {
    const defaults = {
        minimumHeaders: 3,
        headers: 'h1, h2, h3, h4, h5, h6',
        listType: 'ul',
        classes: { list: '', item: '' }
    };
    const settings = Object.assign({}, defaults, options);

    if (!container) return false;

    const createLink = header => {
        const text = header.textContent || header.innerText || '';
        return `<a href="#${header.id}">${text}</a>`;
    };

    const allHeaders = Array.from(document.querySelectorAll(settings.headers)).filter(header => {
        const prevSibling = header.previousElementSibling;
        const prevName = prevSibling?.getAttribute?.("name");
        if (!header.id && prevName) {
            header.id = prevName.replace(/\./g, "-");
        }
        return header.id;
    });

    if (allHeaders.length < settings.minimumHeaders) {
        container.style.display = 'none';
        return false;
    }

    const getLevel = ele => parseInt(ele.nodeName.replace("H", ""), 10);
    const highestLevel = allHeaders.map(getLevel).sort((a, b) => a - b)[0];

    let level = getLevel(allHeaders[0]);
    let html = `<${settings.listType} class="${settings.classes.list}">`;

    allHeaders.forEach(header => {
        const thisLevel = getLevel(header);
        if (thisLevel === level) {
            html += `<li class="${settings.classes.item}">${createLink(header)}`;
        } else if (thisLevel < level) {
            for (let i = thisLevel; i < level; i++) {
                html += `</li></${settings.listType}>`;
            }
            html += `<li class="${settings.classes.item}">${createLink(header)}`;
        } else if (thisLevel > level) {
            for (let j = thisLevel; j > level; j--) {
                html += `<${settings.listType} class="${settings.classes.list}"><li class="${settings.classes.item}">`;
            }
            html += createLink(header);
        }
        level = thisLevel;
    });

    for (let k = 0; k < level - highestLevel; k++) {
        html += `</li></${settings.listType}>`;
    }

    container.innerHTML = html;
    return true;
}


// ============================================================
// Section 3: TOC 双栏布局算法（优化版）
// ============================================================
function splitContentToc2Columns() {
    const tocContent = document.querySelector("#toc-content");
    if (!tocContent) return;
    const tocTopUl = tocContent.querySelector("ul");
    if (!tocTopUl) return;

    const topLis = Array.from(tocTopUl.querySelectorAll(":scope > li"));
    if (topLis.length <= 1) return;

    // 1. 统计每个顶级项的叶子节点数 (自身 1 + 所有子 <li> 数)
    const leafCounts = topLis.map(li => 1 + li.querySelectorAll(":scope > ul li").length);
    const totalLeaves = leafCounts.reduce((sum, count) => sum + count, 0);

    let accum = 0;
    const cumSums = leafCounts.map(count => accum += count);

    // 2. 寻找最佳的顶级边界划分点 (优先选择能让左栏数量 >= 右栏数量，且差值最小的边界)
    let bestBoundaryIdx = 0;
    let minBoundaryDiff = Infinity;
    let boundaryIsLeftHeavy = false;

    for (let i = 0; i < topLis.length - 1; i++) {
        const left = cumSums[i];
        const right = totalLeaves - left;
        const diff = Math.abs(left - right);
        const isLeftHeavy = left >= right;

        if (diff < minBoundaryDiff || (diff === minBoundaryDiff && isLeftHeavy && !boundaryIsLeftHeavy)) {
            minBoundaryDiff = diff;
            bestBoundaryIdx = i;
            boundaryIsLeftHeavy = isLeftHeavy;
        }
    }

    // 3. 计算潜在的内部拆分方案（支持子项权重感知）
    const adjustedTotal = totalLeaves + 1; // 内部拆分会新增一个“（续）”标题，总数+1
    const targetLeft = Math.ceil(adjustedTotal / 2); // 向上取整，确保不平衡时左边多
    const K = cumSums.findIndex(sum => sum >= targetLeft);

    let splitWithin = false;
    let P = 0; // 最终划分给左栏的子节点数
    let finalLeftWeight = cumSums[bestBoundaryIdx];
    let finalRightWeight = totalLeaves - finalLeftWeight;

    if (K >= 0) {
        const splitLi = topLis[K];
        const subUl = splitLi.querySelector(":scope > ul");
        const children = subUl ? Array.from(subUl.children) : [];

        if (children.length > 0) {
            // 计算每一个子项的真实权重 (自身 1 + 内部所有嵌套 li 数量)
            const childWeights = children.map(c => 1 + c.querySelectorAll("li").length);
            
            // 计算子项的累加权重
            let childAccum = 0;
            const childCumWeights = childWeights.map(w => childAccum += w);

            const cumBefore = K > 0 ? cumSums[K - 1] : 0;
            
            let bestP = 0;
            let minWithinDiff = Infinity;
            let withinIsLeftHeavy = false;

            // 遍历所有可能的切分点（必须给右侧留至少 1 个子项，所以上限是 children.length - 1）
            for (let p = 1; p < children.length; p++) {
                const leftW = cumBefore + 1 + childCumWeights[p - 1]; // 左侧真实总重
                const rightW = adjustedTotal - leftW;               // 右侧真实总重
                const diff = Math.abs(leftW - rightW);
                const isLeftHeavy = leftW >= rightW;

                // 筛选策略：在保证左倾（Left >= Right）的前提下寻找最平衡的切分点
                if (diff < minWithinDiff || (diff === minWithinDiff && isLeftHeavy && !withinIsLeftHeavy)) {
                    minWithinDiff = diff;
                    bestP = p;
                    withinIsLeftHeavy = isLeftHeavy;
                }
            }

            if (bestP > 0) {
                const optLeftW = cumBefore + 1 + childCumWeights[bestP - 1];
                const optRightW = adjustedTotal - optLeftW;

                // 决策：只有当内部拆分确实能保证“左 >= 右”，
                // 且其平衡效果优于边界拆分（或边界差值大于 4）时，才采用内部拆分
                if (optLeftW >= optRightW) {
                    if (minBoundaryDiff > 4 || minWithinDiff < minBoundaryDiff) {
                        splitWithin = true;
                        P = bestP;
                        finalLeftWeight = optLeftW;
                        finalRightWeight = optRightW;
                    }
                }
            }
        }
    }

    // 4. 重建双栏 DOM 结构
    const col1Ul = document.createElement("ul");
    const col2Ul = document.createElement("ul");
    col1Ul.className = tocTopUl.className;
    col2Ul.className = tocTopUl.className;

    if (splitWithin) {
        const splitLi = topLis[K];
        const subUl = splitLi.querySelector(":scope > ul");
        const children = Array.from(subUl.children);
        const leftChildren = children.slice(0, P);
        const rightChildren = children.slice(P);

        // 左栏填充：[0 ... K-1] 以及项 K 的前 P 个子树
        for (let i = 0; i < K; i++) {
            col1Ul.appendChild(topLis[i]);
        }
        subUl.innerHTML = '';
        leftChildren.forEach(child => subUl.appendChild(child));
        col1Ul.appendChild(splitLi);

        // 右栏填充：项 K 的续标题及剩余子树，以及 [K+1 ... 末尾]
        const splitLink = splitLi.querySelector(":scope > a");
        if (splitLink) {
            const contLi = document.createElement("li");
            const contLink = document.createElement("a");
            contLink.href = splitLink.getAttribute("href");
            contLink.textContent = `（续）${splitLink.textContent}`;
            contLi.appendChild(contLink);

            if (rightChildren.length > 0) {
                const contUl = document.createElement("ul");
                rightChildren.forEach(child => contUl.appendChild(child));
                contLi.appendChild(contUl);
            }
            col2Ul.appendChild(contLi);
        }

        for (let i = K + 1; i < topLis.length; i++) {
            col2Ul.appendChild(topLis[i]);
        }
    } else {
        // 采用最合理的顶级边界拆分
        for (let i = 0; i <= bestBoundaryIdx; i++) {
            col1Ul.appendChild(topLis[i]);
        }
        for (let i = bestBoundaryIdx + 1; i < topLis.length; i++) {
            col2Ul.appendChild(topLis[i]);
        }
    }

    // 将生成的两栏放入页面
    const wrapper = document.createElement("div");
    wrapper.className = "toc-columns";

    const col1Div = document.createElement("div");
    col1Div.className = "toc-column toc-column--left";
    col1Div.appendChild(col1Ul);

    const col2Div = document.createElement("div");
    col2Div.className = "toc-column toc-column--right";
    col2Div.appendChild(col2Ul);

    wrapper.appendChild(col1Div);
    wrapper.appendChild(col2Div);

    tocContent.appendChild(wrapper);
    tocTopUl.remove();
}


// ============================================================
// Section 4: 侧边栏交互
// ============================================================

function initSideNavInteraction() {
    const pageSideRail = document.querySelector(".page-side-rail");
    const contentTocContainer = document.querySelector("#content-toc-container");
    const tocList = document.querySelector("#toc-nav");
    const navListContainer = document.querySelector(".nav-list");

    if (!pageSideRail) return;

    const ACTIVE_OFFSET = 120;
    const SIDE_RAIL_GAP = 24;
    const SIDE_RAIL_WIDTH_RATIO = 0.15;
    const SIDE_RAIL_MIN_WIDTH = 10;
    const SIDE_RAIL_MAX_WIDTH = 240;
    // 用两个阈值做滞回，避免接近临界宽度时反复显示/隐藏。
    const SIDE_RAIL_SHOW_BUFFER = 8;
    const SIDE_RAIL_HIDE_BUFFER = 4;
    let isSideRailVisible = false;

    const getShowOffset = () => {
        if (!contentTocContainer) return 0;
        return contentTocContainer.getBoundingClientRect().top + window.scrollY + contentTocContainer.offsetHeight;
    };

    const getSideRailWidth = () =>
        Math.min(Math.max(window.innerWidth * SIDE_RAIL_WIDTH_RATIO, SIDE_RAIL_MIN_WIDTH), SIDE_RAIL_MAX_WIDTH);

    const getAvailableSideRailSpace = () => {
        const postPage = document.querySelector(".post-page");
        if (!postPage) return 0;

        const rightEdge = postPage.getBoundingClientRect().right;
        return window.innerWidth - rightEdge;
    };

    const getSideRailSpaceThreshold = (isVisible) => {
        const baseWidth = getSideRailWidth() + SIDE_RAIL_GAP;
        return isVisible
            ? baseWidth + SIDE_RAIL_HIDE_BUFFER
            : baseWidth + SIDE_RAIL_SHOW_BUFFER;
    };

    const shouldShowSideNav = () => {
        const availableSpace = getAvailableSideRailSpace();
        const showOffsetReached = window.scrollY >= getShowOffset();
        const spaceThreshold = getSideRailSpaceThreshold(isSideRailVisible);

        return availableSpace >= spaceThreshold && showOffsetReached;
    };

    const setSideRailVisibility = (visible) => {
        if (isSideRailVisible === visible) return;

        pageSideRail.style.display = visible ? "flex" : "none";
        isSideRailVisible = visible;
    };

    const updateTocVisibility = () => {
        const links = tocList ? tocList.querySelectorAll("a[href^='#']") : [];
        if (links.length === 0) {
            setSideRailVisibility(false);
            return;
        }

        if (!shouldShowSideNav()) {
            setSideRailVisibility(false);
            return;
        }

        setSideRailVisibility(true);

        const scrollPosition = window.scrollY;
        let currentLink = null;

        links.forEach(link => {
            const href = link.getAttribute("href");
            if (href?.startsWith("#")) {
                const target = document.getElementById(href.substring(1));
                if (target) {
                    const targetTop = target.getBoundingClientRect().top + scrollPosition;
                    if (targetTop <= scrollPosition + ACTIVE_OFFSET) {
                        currentLink = link;
                    }
                }
            }
        });

        if (tocList) {
            tocList.querySelectorAll(".active").forEach(el => el.classList.remove("active"));
        }

        if (currentLink) {
            let parent = currentLink.parentElement;
            while (parent && parent !== tocList) {
                if (parent.tagName === 'LI') parent.classList.add("active");
                parent = parent.parentElement;
            }

            if (navListContainer) {
                const linkRect = currentLink.getBoundingClientRect();
                const containerRect = navListContainer.getBoundingClientRect();
                if (linkRect.top < containerRect.top || linkRect.bottom > containerRect.bottom) {
                    currentLink.scrollIntoView({ behavior: "smooth", block: "nearest" });
                }
            }
        }
    };

    const handleScrollOrResize = throttleRAF(updateTocVisibility);

    window.addEventListener("scroll", handleScrollOrResize);
    window.addEventListener("resize", handleScrollOrResize);

    updateTocVisibility();
}


// ============================================================
// Section 5: TOC 初始化入口
// ============================================================

function initToc(buildTocCmd) {
    let buildSuccess = false;

    const makeTocDom = (minHeaderNum) => {
        const navTocEl = document.querySelector('#toc-nav');
        const contentTocEl = document.querySelector('#toc-content');
        let navSuccess = false;
        let contentSuccess = false;

        if (navTocEl) {
            navSuccess = createToc(navTocEl, {
                listType: "ul",
                classes: { list: "nav", item: "" },
                headers: 'h1, h2, h3, h4',
                minimumHeaders: minHeaderNum
            });
        }

        if (contentTocEl) {
            contentSuccess = createToc(contentTocEl, {
                listType: "ul",
                classes: { list: "", item: "" },
                headers: 'h1, h2, h3',
                minimumHeaders: minHeaderNum
            });
        }

        return navSuccess && contentSuccess;
    };

    if (buildTocCmd === "force_enable") {
        buildSuccess = makeTocDom(0);
    } else if (buildTocCmd === "disable") {
        console.log("disable toc gen");
    } else {
        buildSuccess = makeTocDom(4);
    }

    if (buildSuccess) {
        splitContentToc2Columns();
        const contentTocContainer = document.querySelector("#content-toc-container");
        if (contentTocContainer) {
            contentTocContainer.style.display = "";
        }
    }

    initSideNavInteraction();
}
