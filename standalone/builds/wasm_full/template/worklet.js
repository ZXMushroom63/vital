class VialProcessor extends AudioWorkletProcessor {
    constructor() {
        super();
        this.Lock = {
            acquireLock: () => { },
            releaseLock: () => { },
        };
        this.sharedMemory = null;
        this.HEAPF32 = null;
        this.HEAPU32 = null;
        this.HEAPU8 = null;
        this.lockPtr = -1;
        this.consumptionPtr = -1;
        this.bufferPtr = -1;

        // i sincerely hate W3C for replacing the beautiful, clean, scriptprocessor with this piece of junk
        // wdym i cant specify my own buffer size
        // wdym there is *no* official way to check the buffer size
        // who designed ts?? are they dumb???
        this.sentBufferSize = false;

        // AND, to make it EVEN WORSE, the buffer size can just *change*
        // excuse me? I'm working with efficient webassembly here!
        // I allocate memory *once*, not every time your message is called.
        // and you want me to resize my buffers every frame? this is why web apps have a reputation of being slow!
        this.confirmedBufferSize = -1;

        this.port.onmessage = this.handleInitMessage.bind(this);
        console.log("Ready for message.");
    }

    handleInitMessage(event) {
        console.log("Received event!!!", event);
        const { sharedArrBuf, consumption, lockPtr, bufferStack } = event.data;
        console.log("Worklet Initialised! Data: ", event.data);
        this.sharedMemory = sharedArrBuf;
        this.HEAPF32 = new Float32Array(sharedArrBuf);
        this.HEAPU32 = new Uint32Array(sharedArrBuf);
        this.HEAPU8 = new Uint8Array(sharedArrBuf);
        this.consumptionPtr = consumption;
        this.bufferPtr = bufferStack;
        this.lockPtr = lockPtr;
        const self = this;
        this.Lock.acquireLock = function () {
            while (Atomics.compareExchange(self.HEAPU8, lockPtr, 0, 1) !== 0) {
                // spinlock
            }
        }

        this.Lock.releaseLock = function () {
            Atomics.store(self.HEAPU8, lockPtr, 0);
        }
    };

    process(inputList, outputList, parameters) {
        const bSize = outputList[0][0].length;
        if (!this.sentBufferSize) {
            this.confirmedBufferSize = bSize;
            this.sentBufferSize = true;
            this.port.postMessage({
                bufferSize: bSize
            });
            return true;
        }

        if (this.confirmedBufferSize !== bSize) {
            console.error("The AudioContext went ahead and resized the buffer without asking :(");
            console.error("Old: ", this.confirmedBufferSize, "; New: ", bSize);
            return false; //kill the audio processor
        }
        
        if (!this.sharedMemory) {
            return true;
        }
        this.Lock.acquireLock();
        const tOut = outputList[0];
        const HEAPU32 = this.HEAPU32;
        const HEAPF32 = this.HEAPF32;
        const HEAPU8 = this.HEAPU8;
        try {
            const consumption = HEAPU8[this.consumptionPtr];
            const ptr = HEAPU32[this.bufferPtr / 4 + consumption];
            if (!ptr) {
                return true;
            }
            for (let c = 0; c < tOut.length; c++) {
                const f32 = tOut[c];
                const dataPtr = HEAPU32[ptr / 4 + c] / 4;
                f32.set(HEAPF32.subarray(dataPtr, dataPtr + this.confirmedBufferSize));
            }
            HEAPU8[this.consumptionPtr] = consumption + 1;
        } finally {
            this.Lock.releaseLock();
        }
        return true;
    }
}

registerProcessor("vial", VialProcessor);