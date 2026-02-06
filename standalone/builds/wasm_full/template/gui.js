addEventListener("load", ()=>{
    const params = new URLSearchParams(location.search);

    document.querySelectorAll("[data-target][data-value]").forEach(node => {
        const value = parseFloat(node.getAttribute("data-value"));
        const targetProp = node.getAttribute("data-target");
        const searchKey = targetProp.substring(5).toLowerCase();
        
        const storageKey = `vialLauncherProperty->${targetProp}`;
        const stored = localStorage.getItem(storageKey);
        if (params.has(searchKey)) {
            const v = parseFloat(params.get(searchKey));
            globalThis[targetProp] = parseFloat(isFinite(v) ? v: value);
        } else if (stored) {
            globalThis[targetProp] = parseFloat(stored);
        }
        if (!node.innerText.trim()) {
            node.innerText = value + (node.getAttribute("data-suffix") || "");
        }
        if (value === globalThis[targetProp]) {
            node.classList.add("selected");
        }
        node.addEventListener("mousedown", ()=>{
            document.querySelectorAll(`[data-target=${targetProp}]`).forEach(x => x.classList.remove("selected"));
            node.classList.add("selected");
            globalThis[targetProp] = value;
            localStorage.setItem(storageKey, value);
        });
    });
});