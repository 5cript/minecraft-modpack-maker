var Module = {
    onRuntimeInitialized: () => {
        globalThis.throttle = (id, func, limitTime = 1000) => {
            let throttleContext = globalThis["throttle_" + id];
            if (!throttleContext) {
                throttleContext = {
                    func: func,
                    timeout: null
                };
                globalThis["throttle_" + id] = throttleContext;
            }
            if (throttleContext.timeout) {
                throttleContext.func = func;
            }
            else {
                throttleContext.func();
            }
            throttleContext.timeout = setTimeout(() => {
                throttleContext.func();
            }, limitTime);
        };

        Module.main();
    }
};