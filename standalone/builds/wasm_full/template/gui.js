addEventListener("load", ()=>{
    document.querySelectorAll("[data-target][data-value]").forEach(node => {
        const value = parseFloat(node.getAttribute("data-value"));
        const targetProp = node.getAttribute("data-target");
        if (!node.innerText.trim()) {
            node.innerText = value + (node.getAttribute("data-suffix") || "");
        }
        if (value === globalThis[targetProp]) {
            node.classList.add("selected");
        }
        node.addEventListener("click", ()=>{
            document.querySelectorAll(`[data-target=${targetProp}]`).forEach(x => x.classList.remove("selected"));
            node.classList.add("selected");
            globalThis[targetProp] = value;
        });
    });
});