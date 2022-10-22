(() => {
    globalThis.compareDates = (leftDateString, rightDateString) => {
        const leftDate = new Date(leftDateString);
        const rightDate = new Date(rightDateString);
        return leftDate.getTime() < rightDate.getTime();
    };

    globalThis.isChrome = (globalThis.chrome !== undefined);
})();
