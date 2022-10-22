import fs from 'fs';
import { default as pathTools } from 'path';

let compiler = "";

const fixCompileLine = (compileLine) => {
    let command = compileLine.command;
    command = command.replace(/\/([a-zA-Z])\/([^\s]*)/g, (match, c1, c2, ...rest) => {
        return `\"${c1}:/${c2}\"`;
    });
    command = command.split(/(\s+)/).filter((e) => { return e.trim().length > 0; });
    if (command[0].indexOf("em++") !== -1) {
        command.shift();
        command.shift();
        // FIXME: 
        command.unshift("-I\"D:/Development/Nui/test-project/nui/nui/src/nui/../../include\"");
        command.unshift(compiler);
    }
    return {
        ...compileLine,
        command: command.join(' '),
    };
}

let backend = JSON.parse(fs.readFileSync('build/clang_debug/compile_commands.json'));
compiler = fixCompileLine(backend[0]).command.match(/(\\ |[^ ])+/g)[0];
compiler = compiler.substring(1, compiler.length - 1);
let compileCommands = backend;
backend = backend.map(fixCompileLine);
if (fs.existsSync('build/clang_debug/module_minecraft-modpack-maker/compile_commands.json')) {
    let frontend = JSON.parse(fs.readFileSync('build/clang_debug/module_minecraft-modpack-maker/compile_commands.json'));
    frontend = frontend.map(fixCompileLine);
    compileCommands = backend.concat(frontend);
}

fs.writeFileSync('compile_commands.json', JSON.stringify(compileCommands, null, 2), { encoding: 'utf8' });
