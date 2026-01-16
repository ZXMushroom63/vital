function detectWebGLContext() {
    const canvas = document.createElement("canvas");
    const gl = canvas.getContext("webgl");
    return (gl instanceof WebGLRenderingContext)
        ? "✓ Browser supports WebGL."
        : `<span style="color: red">⚠︎ Error: Browser / device does not support WebGL.</span>`;
}

globalThis.VIAL_CHANNEL_COUNT = 2;
globalThis.VIAL_TARGET_SAMPLERATE = 24000;
globalThis.VIAL_AUDIO_STACK_SIZE_SAMPLES = 2048; //How much buffer to smooth over timing inconsistencies
globalThis.VIAL_CLOCKSPEED_MULTIPLIER = 1.25; //Speed multiplier for how often the audio render loop checks if rendering is necessary
globalThis.VIAL_BSIZE = 0; //is initialised automatically
globalThis.VIAL_TARGET_FPS = 24;

const pages = 10400; //64kb pages
const memSize = pages * 64 * 1024;

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

createVial({
    canvas: document.querySelector("#canvas"),
    wasmMemory: SMEM
}).then(Vial => {
    globalThis.Vial = Vial;
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

function ratelimit(func, mininterval) {
    let lastSend = -1;
    let timeout;
    return function (...args) {
        clearTimeout(timeout);
        if ((Date.now() - lastSend) > mininterval) {
            func.apply(this, args);
            lastSend = Date.now();
        } else {
            timeout = setTimeout(() => {
                func.apply(this, args);
            }, mininterval * 2);
        }
    };
}

addEventListener("load", () => {
    document.querySelector("#loading_blocker").remove();
    let inited = false;
    let launchingDisabled = false;
    document.querySelector("#webgl").innerHTML = detectWebGLContext() + "<br>" + (crossOriginIsolated ? "✓ SharedArrayBuffer supported." : `<span style="color:red">⚠︎ Error: SharedArrayBuffer not supported. (try reloading?)</span>`);
    if (document.querySelector("#webgl").innerText.toLowerCase().includes("error")) {
        launchingDisabled = true;
    }
    document.querySelector("#init").addEventListener("click", () => {
        if (!crossOriginIsolated && (location.protocol === "https:" || location.hostname === "localhost")) {
            return location.reload();
        }
        if (inited || !Vial || launchingDisabled) {
            return;
        }
        inited = true;
        Vial._startApplication_Classic();
        Vial._vialSetWindowSize(innerWidth * devicePixelRatio, innerHeight * devicePixelRatio);
        let prevFrame = performance.now();
        let audioTimeRef = -1;
        document.querySelector("#bgvideo").remove();
        document.querySelector("#init_panel").remove();
        function renderLoop() {
            const now = performance.now();
            Vial._vialRedraw();
            const post = performance.now();

            setTimeout(renderLoop, Math.max(1000 / (VIAL_TARGET_FPS) - (post - now), 1));
            const audioResponseTime = (VIAL_BSIZE / VIAL_TARGET_SAMPLERATE * 1000).toFixed(1);
            document.querySelector("#fps_counter").innerText = `${(1000 / (now - prevFrame)).toFixed(2)} FPS [💻${(1000 / (post - now)).toFixed(2)}] [🔊${(Vial.HEAPF64[audioTimeRef] || 0).toFixed(1)}ms / ${audioResponseTime}ms] [📦${VIAL_BSIZE}/${VIAL_TARGET_SAMPLERATE} : ${["MONO", "STEREO"][VIAL_CHANNEL_COUNT - 1] || "OTHER"}]`;

            prevFrame = now;
        }
        renderLoop();
        document.querySelector("#canvas").style.zIndex = 100;

        setTimeout(async () => {
            const audioContext = new AudioContext({
                sampleRate: VIAL_TARGET_SAMPLERATE,
                latencyHint: "playback"
            });
            await audioContext.audioWorklet.addModule("worklet.js");
            const vialNode = new AudioWorkletNode(
                audioContext,
                "vial",
                {
                    numberOfOutputs: 1,
                    outputChannelCount: [VIAL_CHANNEL_COUNT]
                }
            );
            vialNode.port.onmessage = (ev) => {
                if (ev.data.bufferSize) {
                    globalThis.VIAL_BSIZE = ev.data.bufferSize;
                    const audioStackSize = Math.ceil(VIAL_AUDIO_STACK_SIZE_SAMPLES / VIAL_BSIZE);
                    console.log("Buffer size: ", VIAL_BSIZE);
                    console.log(((audioStackSize * VIAL_BSIZE) / VIAL_TARGET_SAMPLERATE * 1000).toFixed(1) + "ms of audio latency.");

                    audioTimeRef = Vial._setupAudioThread(VIAL_CHANNEL_COUNT, audioStackSize, VIAL_BSIZE, VIAL_CLOCKSPEED_MULTIPLIER) / 8;

                    console.log("Attempting AudioWorklet init!");

                    globalThis._dbgVialNode = vialNode;
                    vialNode.port.postMessage({
                        sharedArrBuf: rootBuffer,
                        consumption: _V_AUDIO_CONSUMPTION_PTR,
                        lockPtr: _V_AUDIO_LOCK_PTR,
                        bufferStack: _V_AUDIO_PTRSTACK
                    });

                    console.log("Message posted.");
                }
            }


            vialNode.connect(audioContext.destination);
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
        Vial._processMouseEvent(ev.lmb, ev.mmb, ev.rmb, ev.ctrlKey, ev.shiftKey, ev.altKey, Math.floor(ev.x * devicePixelRatio), Math.floor(ev.y * devicePixelRatio), ev.dbl, ev.wheel);
        ev.wheel = 0;
        ev.dbl = false;
    }
    const queueMouseEvent = ratelimit(sendMouseEvent, 1000 / 120);

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
    canvas.addEventListener("wheel", (e) => {
        ev.wheel += Math.min(Math.max(-1, e.deltaY), 1);
        queueMouseEvent();
    });
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
    const resizeHandler = () => {
        console.log("Triggering resize!");
        if (!inited) {
            return;
        }
        Vial._vialSetWindowSize(innerWidth * devicePixelRatio, innerHeight * devicePixelRatio);
    };

    const debouncedHandler = debounce(resizeHandler, 850);
    addEventListener("resize", () => {
        debouncedHandler();
    });
});
function getProgramComponents(programID, kNumShaders) {
    kNumShaders ||= 21;
    return {
        "VertexShaderID": Math.floor(programID / kNumShaders),
        "FragmentShaderID": programID % kNumShaders
    };
}