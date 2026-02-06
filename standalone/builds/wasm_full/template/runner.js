//?screen_percentage=100&target_fps=12&channel_count=2&audio_stack_size_samples=512&clockspeed_multiplier=1&autostart

const AUTOSTART = (new URLSearchParams(location.search)).has("autostart");
function detectWebGLContext() {
    const canvas = document.createElement("canvas");
    const gl = canvas.getContext("webgl");
    return (gl instanceof WebGLRenderingContext)
        ? "✓ Browser supports WebGL."
        : `<span style="color: red">⚠︎ Error: Browser / device does not support WebGL.</span>`;
}

globalThis.VIAL_CHANNEL_COUNT = 2;
globalThis.VIAL_SAMPLERATE = 24000;
globalThis.VIAL_AUDIO_STACK_SIZE_SAMPLES = 2048; //How much buffer to smooth over timing inconsistencies
globalThis.VIAL_CLOCKSPEED_MULTIPLIER = 1.25; //Speed multiplier for how often the audio render loop checks if rendering is necessary
globalThis.VIAL_BSIZE = 0; //is initialised automatically
globalThis.VIAL_SCREEN_PERCENTAGE = 100;
globalThis.VIAL_TARGET_FPS = 24;

const pages = 9800; //64kb pages
const memSize = pages * 64 * 1024;
console.log("Memory Budget: " + memSize + "b");
console.log(" - Megabytes: " + memSize / 1024 / 1024 + "MiB");
console.log(" - Gigabytes: " + memSize / 1024 / 1024 / 1024 + "GiB");

// float = 4b
// if stereo, each packet is 1024 instead (obvi)
// structure
// DATA STORAGE = VIAL_CHANNEL_COUNT*audioQueueSize*??*4
// POINTER STACK = (4byte pointer)*audioQueueSize*VIAL_CHANNEL_COUNT
// CONSUMPTION = 1 byte
// LOCK = 1 byte

const SMEM = new WebAssembly.Memory({
    initial: pages,
    maximum: pages,
    shared: true
});

const rootBuffer = SMEM.buffer;
function lookupFSPath(path) {
    let currNode = Vial.FS.root;
    path.split("/").flat().filter(x => !!x).forEach(e => {
        if (currNode?.mounted?.root) {
            currNode = currNode?.mounted?.root;
        }
        currNode = currNode?.contents?.[e];
    });
    return currNode;
}
function buf2str(buf) {
    let str = "";
    for (let i = 0; i < buf.length; i++) {
        str += String.fromCharCode(buf[i]);
    }
    return str;
}
function getVialConfig() {
    return buf2str(lookupFSPath("/home/web_user/.vial/Vial.config").contents);
}
function exportTrigger() {
    let exports = Vial.FS.root.contents.home.contents?.web_user?.contents?.[".export"]?.contents || [];
    if (!Array.isArray(exports)) {
        exports = Object.values(exports);
    }
    exports.forEach(exp => {
        if (!(exp.contents instanceof Object.getPrototypeOf(Uint8Array))) {
            return;
        }
        const blob = new File([exp.contents], exp.name, { type: "application/octet-steam" });
        const dlTarget = document.createElement("a");
        const url = URL.createObjectURL(blob);
        dlTarget.href = url;
        dlTarget.setAttribute("download", exp.name);
        dlTarget.click();
        setTimeout(() => {
            URL.revokeObjectURL(url);
            Vial.FS.unlink("/home/web_user/.export/" + exp.name);
        }, 1);
    });
}
function getFSChecksum() {
    let checksum = 0;
    const targets = Vial.FS.root.contents.home.contents?.web_user?.contents?.[".local"]?.contents?.share?.contents?.vital?.contents?.User?.contents?.Presets?.mounted?.root?.contents;
    if (!targets) {
        return -1;
    }
    Object.values(targets).map(file => {
        for (let i = 0; i < file.name.length; i++) {
            checksum += file.name.charCodeAt(i);
        }
        checksum %= 2 ** 31;
        checksum += file.mtime.getTime ? file.mtime.getTime() : file.mtime
        checksum %= 2 ** 31;
    });
    return checksum;
}
globalThis._V_KMAP_PTR = null;
globalThis.fileReadWorker = new Worker("fileworker.js");
const FILE_DEALLOC_TIMEOUT = 30 * 1000;
globalThis.FILE_DEALLOC_QUEUE = new Set();
globalThis.BLANK_BUF = new Uint8Array(0);

globalThis.ASSET_MOUNTPOINTS = {
    "./Wavetables": "/wavetable_mount",
    "./Samples": "/sample_mount",
};
function deallocateUnusedFiles() {
    if (Math.random() > 0.1) {
        return;
    }
    const now = Date.now();
    const remQueue = [];
    for (const allocatedFile of FILE_DEALLOC_QUEUE) { //remove unused file buffers from memory
        if ((now - allocatedFile.readTime) > FILE_DEALLOC_TIMEOUT) {
            console.log("[FILEALLOC] GC:", allocatedFile.entName);
            FILE_OVERRIDES[allocatedFile.entName] = allocatedFile.entry; //replace buffer with handle
            Vial.FS.writeFile(allocatedFile.targetWritePath, BLANK_BUF); //remove the buffer
            remQueue.push(allocatedFile);
        }
    }
    remQueue.forEach(x => FILE_DEALLOC_QUEUE.delete(x));
}
createVial({
    canvas: document.querySelector("#canvas"),
    wasmMemory: SMEM
}).then(async Vial => {
    // hack in blob/file support, so you can access a lot of files without needing obtuse amounts of memory.
    // using the sample blob in multiple areas will unfortunately not use the same arraybuffer for all blobs, so consider each blob/file in this dict unique
    globalThis.FILE_OVERRIDES = {
        "./Wavetables/test_wavetable.wav": await (await fetch("test_wavetable.wav")).blob(),
        "./Samples/clock.wav": await (await fetch("clock_final.mp3")).blob(),
        "./Presets/demopad.vital": await (await fetch("demopad.vital")).blob(),
    };
    globalThis.Vial = Vial;
    globalThis._V_KMAP_PTR = Vial._preinit();
    //Vial.FS.mkdir('/home/web_user');
    //vIDBFS.mkdir("/home/web_user");
    Vial.FS.mkdirTree('/home/web_user/.local/share/vital/User/Presets');
    Vial.FS.mkdirTree('/home/web_user/.export');
    Vial.FS.mount(vIDBFS, {}, '/home/web_user/.local/share/vital/User/Presets');
    const targetDir = "/home/web_user/.local/share/vital/User/";
    Vial.FS.syncfs(true, (err) => {
        if (err) {
            console.error("Failed to restore filesystem state! ", err);
        } else {
            console.log("Restored filesystem state.");
        }

        Object.entries(globalThis.ASSET_MOUNTPOINTS).forEach(mnt => {
            Vial.FS.mkdir(mnt[1]);
            const folderName = mnt[0].replace("./", "");
            try {
                Vial.FS.rmdir(targetDir + folderName);
            } catch (e) { }
            try {
                Vial.FS.symlink(mnt[1], targetDir + folderName);
            } catch (e) {
                console.error(e);
            }
        });

        Object.keys(FILE_OVERRIDES).forEach(assetPath => {
            let targetAssetPath = assetPath;
            let foundMountpoint = false;
            Object.entries(globalThis.ASSET_MOUNTPOINTS).forEach(ent => {
                if (targetAssetPath.includes(ent[0])) {
                    foundMountpoint = true;
                    targetAssetPath = targetAssetPath.replace(ent[0], ent[1]);
                }
            });
            if (foundMountpoint) {
                console.log("1. Writing to ", targetAssetPath);
                Vial.FS.writeFile(targetAssetPath, BLANK_BUF);
            } else {
                console.log("2. Writing to ", targetAssetPath);
                Vial.FS.writeFile(targetDir + targetAssetPath.slice(2), BLANK_BUF);
            }
        });

        let lastSum = getFSChecksum();
        const SAVE_INTERVAL = 10 * 1000;
        function trySave() {
            let newSum = getFSChecksum();
            if (newSum !== lastSum) {
                lastSum = newSum;
                Vial.FS.syncfs(false, (err) => {
                    if (err) {
                        console.error("Failed to record filesystem state! ", err);
                    } else {
                        console.log("Saved filesystem state.");
                    }
                    setTimeout(trySave, SAVE_INTERVAL);
                });
            } else {
                setTimeout(trySave, SAVE_INTERVAL);
            }
        }
        setTimeout(trySave, SAVE_INTERVAL);
        if (AUTOSTART) {
            setTimeout( () => document.querySelector("#init").click(), 250 );
        }
    });

}).catch(function (error) {
    console.error("Module creation failed:", error);
    alert("Failed to initialise WebAssembly module!\nYour CPU chipset likely does not support SIMD, or\nyour browser is missing WASM SIMD/thread support.");
    alert("See console for full error.");
});

addEventListener("contextmenu", (e) => { e.preventDefault(); });
function debounce(func, delay) {
    let timeout;
    return function (...args) {
        clearTimeout(timeout);
        timeout = setTimeout(() => {
            func.apply(this, args);
        }, delay);
    };
}

function ratelimit(func, mininterval, deb) {
    let lastSend = -1;
    let timeout;
    return function (...args) {
        if (deb) {
            clearTimeout(timeout);
        }
        if ((Date.now() - lastSend) > mininterval) {
            func.apply(this, args);
            lastSend = Date.now();
        } else {
            if (deb) {
                timeout = setTimeout(() => {
                    func.apply(this, args);
                }, mininterval * 2);
            }
        }
    };
}
function isPageHidden() {
    return document.hidden || document.msHidden || document.webkitHidden || document.mozHidden;
}
const getRatio = ()=>globalThis.VIAL_SCREEN_PERCENTAGE / 100 * devicePixelRatio;
addEventListener("load", () => {
    if (!AUTOSTART) {document.querySelector("#loading_blocker").remove();}
    let inited = false;
    let launchingDisabled = false;
    document.querySelector("#webgl").innerHTML = detectWebGLContext() + "<br>" + (crossOriginIsolated ? "✓ SharedArrayBuffer supported." : `<span style="color:red">⚠︎ Error: SharedArrayBuffer not supported. (try reloading?)</span>`);
    if (document.querySelector("#webgl").innerText.toLowerCase().includes("error")) {
        launchingDisabled = true;
    }
    const resizeHandler = () => {
        console.log("Triggering resize!");
        canvas.style.imageRendering = (VIAL_SCREEN_PERCENTAGE === 100) ? "pixelated" : "auto";
        if (!inited) {
            return;
        }
        Vial._vialSetWindowSize(Math.round(innerWidth * getRatio()), Math.round(innerHeight * getRatio()));
    };
    document.querySelector("#init").addEventListener("click", () => {
        if (!crossOriginIsolated && (location.protocol === "https:" || location.hostname === "localhost")) {
            return location.reload();
        }
        if (inited || !Vial || launchingDisabled) {
            return;
        }
        inited = true;
        Vial._startApplication_Classic();
        resizeHandler();
        let prevFrame = performance.now();
        let audioTimeRef = -1;
        document.querySelector("#bgvideo").remove();
        document.querySelector("#init_panel").remove();
        function renderLoop() {
            const now = performance.now();
            const visible = !isPageHidden();
            if (visible) {
                Vial._vialRedraw();
            }
            const post = performance.now();

            setTimeout(renderLoop, Math.max(1000 / (VIAL_TARGET_FPS) - (post - now), 1));
            const audioResponseTime = (VIAL_BSIZE / VIAL_SAMPLERATE * 1000).toFixed(1);
            if (visible) {
                document.querySelector("#fps_counter").innerText = `${(1000 / (now - prevFrame)).toFixed(2)} FPS [💻${(1000 / (post - now)).toFixed(2)}] [🔊${(Vial.HEAPF64[audioTimeRef] || 0).toFixed(1)}ms / ${audioResponseTime}ms] [📦${VIAL_BSIZE}/${VIAL_SAMPLERATE} : ${["MONO", "STEREO"][VIAL_CHANNEL_COUNT - 1] || "OTHER"}]`;
            } else {
                document.querySelector("#fps_counter").innerText = `webvial is not focused/visible`;
            }

            prevFrame = now;
            deallocateUnusedFiles();
            exportTrigger();
        }
        renderLoop();
        document.querySelector("#canvas").style.zIndex = 100;
        if (AUTOSTART) {document.querySelector("#loading_blocker").remove();}

        setTimeout(async () => {
            console.log("Attempting Audio Init!");
            const audioContext = new AudioContext({
                sampleRate: VIAL_SAMPLERATE,
                latencyHint: "playback"
            });
            await audioContext.audioWorklet.addModule("worklet.js");
            console.log("Audio Worklet loaded.");
            const vialNode = new AudioWorkletNode(
                audioContext,
                "vial",
                {
                    numberOfOutputs: 1,
                    outputChannelCount: [VIAL_CHANNEL_COUNT]
                }
            );
            vialNode.port.onmessage = (ev) => {
                console.log("Port event: ", ev);
                if (ev.data.bufferSize) {
                    globalThis.VIAL_BSIZE = ev.data.bufferSize;
                    const audioStackSize = Math.ceil(VIAL_AUDIO_STACK_SIZE_SAMPLES / VIAL_BSIZE);
                    console.log("Buffer size: ", VIAL_BSIZE);
                    console.log(((audioStackSize * VIAL_BSIZE) / VIAL_SAMPLERATE * 1000).toFixed(1) + "ms of audio latency.");

                    audioTimeRef = Vial._setupAudioThread(VIAL_CHANNEL_COUNT, audioStackSize, VIAL_BSIZE, VIAL_CLOCKSPEED_MULTIPLIER) / 8;

                    console.log("Attempting AudioWorklet init!");

                    globalThis._dbgVialNode = vialNode;
                    vialNode.port.postMessage({
                        sharedArrBuf: rootBuffer,
                        consumption: _V_AUDIO_CONSUMPTION_PTR,
                        lockPtr: _V_AUDIO_LOCK_PTR,
                        bufferStack: _V_AUDIO_PTRSTACK,
                        maxConsumption: audioStackSize * 2 - 1
                    });

                    console.log("Message posted.");
                }
            }

            console.log("Connecting to audio dest");
            vialNode.connect(audioContext.destination);
            addEventListener("mousedown", (e)=>{
                if (audioContext.state === "suspended") {
                    audioContext.resume();
                }
            });
        }, 250);
    });
    const ev = {
        lmb: false,
        mmb: false,
        rmb: false,
        ctrlKey: false,
        shiftKey: false,
        altKey: false,
        x: 0,
        y: 0,
        dbl: false,
        wheel: 0,
    }

    function sendMouseEvent() {
        if (!inited) {
            return;
        }
        //console.log("sending mouse data.");
        Vial._processMouseEvent(ev.lmb, ev.mmb, ev.rmb, ev.ctrlKey, ev.shiftKey, ev.altKey, Math.floor(ev.x * getRatio()), Math.floor(ev.y * getRatio()), ev.dbl, ev.wheel);
        ev.wheel = 0;
        ev.dbl = false;
    }
    const queueMouseEvent = ratelimit(sendMouseEvent, 1000 / Math.max(VIAL_TARGET_FPS * 2, 120), true);

    console.log("Resistering input handlers.");
    const canvas = document.querySelector("#canvas");
    canvas.addEventListener("mousedown", (event) => {
        if (event instanceof MouseEvent) {
            event.preventDefault();
            ev.ctrlKey = event.ctrlKey;
            ev.altKey = event.altKey;
            ev.shiftKey = event.shiftKey;
            ev.x = event.layerX;
            ev.y = event.layerY;
            switch (event.button) {
                case 0:
                    ev.lmb = true;
                    break;
                case 1:
                    ev.mmb = true;
                    break;
                case 2:
                    //ev.rmb = true;
                    // emulate macos control scheme
                    ev.lmb = true;
                    ev.ctrlKey = true;
                    break;

                default:
                    break;
            }
            queueMouseEvent();
        }
    });
    addEventListener("keydown", (e) => {
        if (!e.altKey && !e.ctrlKey && !e.metaKey && inited && !e.repeat) {
            const charCode = e.key.length > 1 ? 0 : e.key.charCodeAt(0);
            Vial.HEAPU8[globalThis._V_KMAP_PTR + charCode] = 1;
            Vial._processKeyboardKey(e.keyCode, charCode, true);
        }
    });
    addEventListener("keyup", (e) => {
        if (e.repeat || !inited) {
            return;
        }
        const charCode = e.key.length > 1 ? 0 : e.key.charCodeAt(0);
        Vial.HEAPU8[globalThis._V_KMAP_PTR + charCode] = 0;
        Vial._processKeyboardKey(e.keyCode, charCode, false);
    });
    canvas.addEventListener("mouseout", () => {
        event.preventDefault();
        ev.lmb = false;
        ev.mmb = false;
        ev.rmb = false;
        ev.ctrlKey = false;
        ev.altKey = false;
        ev.shiftKey = false;
        queueMouseEvent();
    });
    canvas.addEventListener("dblclick", () => {
        ev.dbl = true;
        queueMouseEvent();
    });
    function wheelHandler(e) {
        ev.wheel += Math.min(Math.max(-1, e.deltaY * (e.deltaMode ? 1 : 0.05)), 1);
        queueMouseEvent();
    }
    const ratelimitedWheel = wheelHandler; //ratelimit(wheelHandler, 1000 / 14, false);
    canvas.addEventListener("wheel", (e) => {
        if (e.deltaType) {
            wheelHandler(e);
        } else {
            // trackpad event flooding
            ratelimitedWheel(e);
        }
        e.preventDefault();
    }, { passive: false });
    canvas.addEventListener("mouseup", (event) => {
        if (event instanceof MouseEvent) {
            switch (event.button) {
                case 0:
                    ev.lmb = false;
                    break;
                case 1:
                    ev.mmb = false;
                    break;
                case 2:
                    ev.rmb = false;
                    // emulate macos control scheme
                    ev.lmb = false;
                    break;

                default:
                    break;
            }
            ev.ctrlKey = event.ctrlKey;
            ev.altKey = event.altKey;
            ev.shiftKey = event.shiftKey;
            ev.x = event.layerX;
            ev.y = event.layerY;
            queueMouseEvent();
        }
    });
    canvas.addEventListener("mousemove", (event) => {
        if (event instanceof MouseEvent) {
            ev.ctrlKey = event.ctrlKey;
            ev.altKey = event.altKey;
            ev.shiftKey = event.shiftKey;
            ev.x = event.layerX;
            ev.y = event.layerY;
            queueMouseEvent();
        }
    });
    

    console.log("drop listener added");
    addEventListener("dragover", (e) => {
        e.preventDefault();
        e.stopPropagation();
        e.dataTransfer.dropEffect = "copy";
    });
    addEventListener("drop", (ev) => {
        ev.preventDefault();
        ev.stopPropagation();
        console.log("done");
        /** @type {DataTransfer} */
        const dT = ev.dataTransfer;
        ev.dataTransfer.dropEffect = "copy";
        const data = [...dT.files];
        if (data.length !== 1) {
            return;
        }
        let fileExt = data[0].name.split(".");
        fileExt = fileExt[fileExt.length - 1];
        const accept = ["vital", "vial", "json"];
        if (!accept.includes(fileExt)) {
            return console.error("Invalid file type!");
        }
        const fileName = data[0].name.split(".")[0];
        const fr = new FileReader();
        fr.onload = ()=>{
            const buffer = new Uint8Array(fr.result);
            const storeName = "/" + fileName.substring(0, 25) + "." + fileExt;
            const strPtr = Vial._malloc(32);
            for (let i = 0; i < storeName.length; i++) {
                Vial.HEAPU8[strPtr + i] = storeName.charCodeAt(i) & 255;
            }
            Vial.HEAPU8[strPtr + storeName.length] = 0; //null byte terminator
            Vial.FS.writeFile(storeName, buffer);
            Vial._processDnD(strPtr);
            console.log("DnD sent!");
            Vial._free(strPtr);
            Vial.FS.unlink(storeName);
        };
        fr.readAsArrayBuffer(data[0]);
    });

    const debouncedHandler = debounce(resizeHandler, 850);
    addEventListener("resize", () => {
        debouncedHandler();
    });


    /** @type Map<string, MIDIInput>  */
    const midiListenerDB = new Map();

    function isNoteOn(statusByte, vel) {
        return (statusByte & 0xF0) === 0x90 && vel !== 0;
    }
    navigator.permissions.query({ name: "midi" }).then(res => {
        /** @param {MIDIMessageEvent} message  */
        function handleMidiData(message) {
            if (!message.data || !inited) {
                return;
            }
            const [stat, note, vel] = message.data;
            const isDownEvent = isNoteOn(stat, vel);

            Vial._processMidiEvent(isDownEvent, note, vel);
        }

        /** @param {MIDIInput[]} inputList  */
        function updateMidiState(inputList) {
            inputList.forEach(inp => {
                if (midiListenerDB.has(inp.id)) {
                    midiListenerDB.get(inp.id).onmidimessage = null;
                }
                inp.onmidimessage = handleMidiData;
                midiListenerDB.set(inp.id, inp);
            });
        }

        /** @param {MIDIAccess} midiIF  */
        function initMidi(midiIF) {
            updateMidiState([...midiIF.inputs.values()]);
            midiIF.addEventListener("statechange", () => {
                updateMidiState([...midiIF.inputs.values()]);
            });
        }
        if (res.state !== "denied") {
            navigator.requestMIDIAccess().then((midiIF) => {
                initMidi(midiIF);
            });
        }
    })
});
function getProgramComponents(programID, kNumShaders) {
    kNumShaders ||= 21;
    return {
        "VertexShaderID": Math.floor(programID / kNumShaders),
        "FragmentShaderID": programID % kNumShaders
    };
}