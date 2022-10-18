var Module = {
    onRuntimeInitialized: () => {
        console.log('WASM Runtime initialized bla');
        Module.main();
    },
    wasmMemory: new ArrayBuffer(1024 * 1024 * 1024),
};

export default Module;