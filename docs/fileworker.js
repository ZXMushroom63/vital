self.onmessage = function(event) {
    const fileReader = new FileReader();
    const { file, SAB, lockPtr } = event.data;
    const HEAPU8 = new Uint8Array(SAB);

    fileReader.onload = function(e) {
        HEAPU8.set(new Uint8Array(fileReader.result), 1);
        Atomics.store(HEAPU8, lockPtr, 1);
        console.log("[FILEWORKER] Read request completed for: ", file.name || file);
    };
    
    fileReader.readAsArrayBuffer(file);
};