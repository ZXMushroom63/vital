class VialSynth extends AudioWorkletProcessor {
    constructor() {
        super();
        this.buffer = new Float32Array(512);
    }

    process(inputs, outputs) {
        const output = outputs[0];

        // Fill the output with random data or your specific logic
        for (let channel = 0; channel < output.length; channel++) {
            const outputChannel = output[channel];
            for (let i = 0; i < outputChannel.length; i++) {
                outputChannel[i] = (Math.random() - 0.5) * 0.000; // Example audio signal
            }
        }

        return true;
    }
}

registerProcessor('vial', MyAudioProcessor);
