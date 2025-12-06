/**
 * This is the "atomtools" build system, it's just JavaScript and node.js.
 * 1. The difference between "locations" and "targets" are that locations cannot
 *    be built directly, they are just dependencies for either targets or collections
 *    of individual targets (from build.js files).
 *    Targets can be built directly from the command line, just like any other build system.
 *    This way, you have more control and modularity. I would personally never need a bootloader
 *    to be built as a separate process and would rather just have the individual files be built separately.
 * 2. The benefit of having build.js files and locations is that it makes the system modular, so the build
 *    happens within the context of a specific folder rather than the entire system.
 */

import { spawn } from 'node:child_process';
import { exit } from 'node:process';
import fs from 'node:fs';
import which from 'which';
import { promisify } from 'node:util';

declare global {
    interface Console {
        misc: (...args: string[]) => void;
    }
}

function runCommand(command: string, args: string[]): Promise<{ code: number, stdout: string, stderr: string }> {
    return new Promise((resolve, reject) => {
        const child = spawn(command, args);

        let stdout = "";
        child.stdout.on('data', (chunk) => {
            stdout += chunk.toString();
        });
        let stderr = "";
        child.stderr.on('data', (chunk) => {
            stderr += chunk.toString();
        });

        child.on('close', (code: number) => {
            if (code != 0) reject({ code, stdout, stderr });
            resolve({ code, stdout, stderr });
        });
    })
}
let originalConsoleLog = console.log;
let originalConsoleInfo = console.info;
let originalConsoleWarn = console.warn;
let originalConsoleError = console.error;
let originalConsoleDebug = console.debug;
console.log = function (...args: string[]) {
    originalConsoleLog("\x1b[0;32m", ...args, "\x1b[0m");
}
console.info = function (...args: string[]) {
    originalConsoleInfo("\x1b[0;34m", ...args, "\x1b[0m");
}
console.warn = function (...args: string[]) {
    originalConsoleWarn("\x1b[0;33m", ...args, "\x1b[0m");
}
console.error = function (...args: string[]) {
    originalConsoleError("\x1b[0;31m", ...args, "\x1b[0m");
}
console.debug = function (...args: string[]) {
    originalConsoleDebug("\x1b[0;35m", ...args, "\x1b[0m");
}
// gray
console.misc = function (...args: string[]) {
    originalConsoleLog("\x1b[0;90m", ...args, "\x1b[0m");
}

let projectConfiguration: any;
let buildFiles: { [path: string]: any } = {};
const CONFIG_CACHE_PATH = "./.atomtools-cache.json";
const CACHE_VERSION = 1;

function isIterable(obj: any): boolean {
    // checks for null and undefined
    if (obj == null) {
        return false;
    }
    return typeof obj[Symbol.iterator] === 'function';
}

// We escape tools and paths but we also want to escape special variables
// like $BUILD or $DIR
// This just does the most general string replacements: specifics are handled elsewhere.
function escapeCommand(command: string, require: any, directory: string): string {
    let escaped: string = command;
    if (require?.tools) {
        for (let [tool, variable] of Object.entries(require.tools) as ([string, string])[]) {
            let toolPath = projectConfiguration.tools[tool].path;
            escaped = escaped.replaceAll(`$${variable}`, toolPath);
        }
    }
    if (require?.paths) {
        for (let [pathKey, pathEnv] of Object.entries(require.paths) as [string, string][]) {
            let pathValue = projectConfiguration.paths[pathKey].path;
            escaped = escaped.replaceAll(`$${pathEnv}`, pathValue);
        }
    }
    if (require?.directories) {
        for (let [dirKey, dirEnv] of Object.entries(require.directories) as [string, string][]) {
            let dirValue = projectConfiguration.directories[dirKey];
            escaped = escaped.replaceAll(`$${dirEnv}`, dirValue);
        }
    }
    escaped = escaped.replaceAll(`$DIR`, directory);
    escaped = escaped.replaceAll(`$BUILD`, projectConfiguration.build);
    escaped = escaped.replaceAll(`$PROJECT`, process.cwd());
    return escaped;
}

async function populateToolPaths() {
    let tools = projectConfiguration.tools;
    let success = true;
    let promises = []
    for (let tool of Object.keys(tools) as (string)[]) {
        if (tools[tool].path) {
            console.debug(`   -> Using cached ${tool} at ${tools[tool].path}`);
            continue;
        }
        promises.push(
            which(tool).catch(() => null).then(item => {
                if (!item) {
                    console.error(`   Error: Unable to find ${tool} in your PATH...`)
                    console.warn(`   Please install it or set the environment variable ${tool} to the correct path.`)
                    success = false;
                    return;
                }
                tools[tool].path = item;
                return item;
            }).then(() => {
                return runCommand("sh", ["-c", tools[tool].testCommand()]).catch(() => ({ code: -1, stdout: "", stderr: "" }))
            }).then(({
                stdout, stderr, code
            }) => {
                if (code != 0) {
                    console.error(`   Error: Found ${tool} at ${tools[tool].path}, but it doesn't seem to be working correctly.`)
                    console.info(stderr);
                    success = false;
                    return;
                }
                console.debug(`   -> Found ${tool} at ${tools[tool].path}`);
            }
            )
        );
    }
    await Promise.all(promises);
    if (!success) exit(1);
}

async function populateDirectoryPaths() {
    let directories = projectConfiguration.paths;
    let success = true;
    let promises = [];
    for (let dir of Object.keys(directories) as (keyof typeof directories)[]) {
        let item = directories[dir];
        if (item.path) {
            console.debug(`   -> Using cached ${item.name} path at ${item.path}`);
            continue;
        }
        promises.push(runCommand("sh", ["-c", item.find[0] as string]).catch(() => ({ code: -1, stdout: "", stderr: "" })).then(({
            stdout, stderr, code
        }) => {
            if (code != 0) {
                console.error(`   Error: Unable to find ${item.name} path using ${projectConfiguration.tools["i686-astatine-gcc"].path}...`)
                console.info(stderr);
                success = false;
                return;
            }
            let lines = stdout.split("\n");
            for (let line of lines) {
                let match = (item.find[1] as RegExp).exec(line);
                if (match) {
                    let foundPath = match[1] + item.name;
                    if (!fs.existsSync(foundPath)) {
                        console.error(`   Error: Unable to find the ${item.name} directory.`);
                        success = false;
                        break;
                    }
                    console.debug(`   -> Found ${item.name} path at ${foundPath}`);
                    item.path = foundPath;
                    break;
                }
            }
        }))
    }
    await Promise.all(promises);
    if (!success) {
        exit(1);
    }
}

let primaryTarget: string;

async function getBuildFiles() {
    buildFiles = {} as any;
    const importPromises: Promise<void>[] = [];

    for (let sourceRoot of projectConfiguration.source) {
        const queue: string[] = [sourceRoot];
        while (queue.length) {
            const current = queue.pop() as string;
            let entries: string[] = [];
            try {
                entries = fs.readdirSync(current);
            } catch {
                continue;
            }
            for (let entry of entries) {
                const fullPath = `${current}/${entry}`;
                let stat;
                try {
                    stat = fs.statSync(fullPath);
                } catch {
                    continue;
                }
                if (stat.isDirectory()) {
                    queue.push(fullPath);
                    continue;
                }
                if (entry === "build.js" || entry === "build.json") {
                    importPromises.push(
                        import(fullPath).then(file => {
                            if (!file.default?.atomtools) return;
                            console.debug(`   -> Found build file: ${fullPath}`);
                            buildFiles[fullPath] = file.default;
                        }).catch((err) => {
                            console.warn(`   Warning: skipping build file ${fullPath}: ${err}`);
                        })
                    );
                }
            }
        }
    }

    await Promise.all(importPromises);
    console.info(`-> Found ${Object.keys(buildFiles).length} build files`);
}

async function configure() {
    const cacheLoaded = await loadConfigurationCache();
    if (!cacheLoaded) {
        console.info("-> Configuring tools...");
        await populateToolPaths();
        console.info("-> Getting required paths...");
        await populateDirectoryPaths();
        await writeConfigurationCache();
    } else {
        console.info("-> Using cached tool/path resolution...");
    }
    console.info("-> Enumerating build files...");
    await getBuildFiles();
}

// get last modified time of a file or directory
async function getCreationDateOfPath(path: string): Promise<number> {
    try {
        let stat = await promisify(fs.stat)(path);
        return stat.mtimeMs;
    } catch {
        return 0;
    }
}

async function isOutputStale(inputPath: string, outputPath: string): Promise<boolean> {
    // Skip work when the produced output is already newer than its input
    const [inputTime, outputTime] = await Promise.all([
        getCreationDateOfPath(inputPath),
        getCreationDateOfPath(outputPath)
    ]);
    return outputTime < inputTime;
}

async function loadConfigurationCache(): Promise<boolean> {
    if (!fs.existsSync(CONFIG_CACHE_PATH)) return false;
    try {
        const cache = JSON.parse(fs.readFileSync(CONFIG_CACHE_PATH, "utf-8"));
        if (cache.version !== CACHE_VERSION) return false;

        const configStamp = await getCreationDateOfPath("./build.config.js");
        if (cache.configStamp !== configStamp) return false;

        let toolsComplete = true;
        for (let tool of Object.keys(projectConfiguration.tools || {})) {
            const cachedPath = cache.tools?.[tool];
            if (!cachedPath) {
                toolsComplete = false;
                continue;
            }
            projectConfiguration.tools[tool].path = cachedPath;
        }

        let pathsComplete = true;
        for (let pathKey of Object.keys(projectConfiguration.paths || {})) {
            const cachedPath = cache.paths?.[pathKey];
            if (!cachedPath) {
                pathsComplete = false;
                continue;
            }
            projectConfiguration.paths[pathKey].path = cachedPath;
        }

        return toolsComplete && pathsComplete;
    } catch {
        return false;
    }
}

async function writeConfigurationCache() {
    const configStamp = await getCreationDateOfPath("./build.config.js");
    const cache = {
        version: CACHE_VERSION,
        configStamp,
        tools: Object.fromEntries(Object.entries(projectConfiguration.tools || {}).map(([key, value]: [string, any]) => [key, value.path])),
        paths: Object.fromEntries(Object.entries(projectConfiguration.paths || {}).map(([key, value]: [string, any]) => [key, value.path]))
    };
    try {
        fs.writeFileSync(CONFIG_CACHE_PATH, JSON.stringify(cache, null, 2));
    } catch (err: any) {
        console.warn(`   Warning: Unable to write configuration cache: ${err}`);
    }
}

async function buildTarget() {
    console.log(`-> Starting build for target "${primaryTarget}"...`);
    // Step 0: create the build directory
    if (!fs.existsSync(projectConfiguration.build)) {
        fs.mkdirSync(projectConfiguration.build, { recursive: true });
    }

    let target = projectConfiguration.targets.find((t: any) => t.name === primaryTarget);
    // Step 1: let's see what our target is.
    if (!target) {
        console.error(`Error: Target "${primaryTarget}" not found.`);
        exit(1);
    }

    // reset per-run mutation markers
    target.wasModified = false;
    const targetOutputPath = escapeCommand(target.output, target.require || {}, "./");

    // Step 1.5: see if the target is newer than its dependencies
    // if so, we can skip the build entirely
    let targetTime = await getCreationDateOfPath(targetOutputPath);
    // Step 2: let's find dependencies first and foremost.
    // these deps need to be built first.
    let depsModified = false;
    for (let dep of (target.depends || [])) {
        console.info(`-> Building dependency "${dep}"...`);
        // let's look to see if this is a target or location
        let depLocation = projectConfiguration.locations.find((l: any) => l.name === dep);
        let depTarget = projectConfiguration.targets.find((t: any) => t.name === dep);
        if (depTarget) {
            // we just build the target normally
            let currentTarget = primaryTarget;
            primaryTarget = dep;
            await buildTarget();
            primaryTarget = currentTarget;
            depsModified = depsModified || !!projectConfiguration.targets.find((t: any) => t.name === dep)?.wasModified;
            continue;
        }
        if (depLocation) depLocation.wasModified = false;
        let collectedFiles = []
        for (let [dir, buildFile] of Object.entries(buildFiles)) {
            dir = dir.split("/").slice(0, -1).join("/")
            if (buildFile.install && buildFile.install[depLocation.name]) {
                // let's process the build file by first going through
                // the build commands, replacing the escapes as needed.
                if (buildFile.build && isIterable(buildFile.build)) {
                    for (let command of buildFile.build) {
                        let promises = [];
                        if (command.type === "command") {
                            let commandStr = escapeCommand(command.command, buildFile.require || {}, dir);

                            projectConfiguration.locations.find(({name}: any) => name === depLocation.name).wasModified = true;

                            if (command.prerequisites) {
                                for (let [prereqCommand, prereqVar] of command.prerequisites as ([string, string])[]) {
                                    let escapedPrereq = escapeCommand(prereqCommand, buildFile.require || {}, dir);
                                    let prereqResult = await runCommand("bash", ["-c", escapedPrereq]).catch(({ code, stdout, stderr }) => {
                                        console.error(`   Error: Prerequisite command failed with code ${code}`);
                                        console.info(stderr);
                                        exit(1);
                                    });
                                    commandStr = commandStr.replaceAll(`$${prereqVar}`, prereqResult.stdout.trim());
                                }
                            }
                            promises.push(runCommand("bash", ["-c", commandStr]).then(() => {
                                console.misc(`   -> ${commandStr}`); 
                            }).catch(({ code, stdout, stderr }) => { 
                                console.error(`   Error: Command ${commandStr} failed with code ${code}`);
                                console.info(stderr);
                                exit(1);
                            }))
                        } else if (command.type === "foreach") {
                            let inputPattern = escapeCommand(command.input, buildFile.require || {}, dir);
                            let outputPattern = escapeCommand(command.output, buildFile.require || {}, dir);
                            // we also escape by file_base, which just means the filename without extension
                            let inputFiles: string[] = [];

                            // let's glob the input pattern
                            let glob = await import('glob');
                            inputFiles = glob.globSync(inputPattern, { nodir: true });

                            for (let inputFile of inputFiles) {
                                let fileBase = inputFile.split(".").slice(0, -1).join(".") || "";
                                let outputFile = outputPattern.replaceAll("$FILE_BASE", fileBase);
                                if (!fs.existsSync(outputFile.split("/").slice(0, -1).join("/"))) {
                                    fs.mkdirSync(outputFile.split("/").slice(0, -1).join("/"), { recursive: true });
                                }

                                if (!(await isOutputStale(inputFile, outputFile))) {
                                    console.misc(`   -> Skipping ${inputFile} (cached)`);
                                    continue;
                                }
                                for (let subcommand of command.build) {
                                    if (subcommand.type === "command") {
                                        let finalCommand = subcommand.command.replaceAll("$INPUT", inputFile).replaceAll("$OUTPUT", outputFile);
                                        finalCommand = escapeCommand(finalCommand, buildFile.require || {}, dir);

                                        projectConfiguration.locations.find(({name}: any) => name === depLocation.name).wasModified = true;

                                        if (subcommand.prerequisites) {
                                            for (let [prereqCommand, prereqVar] of subcommand.prerequisites as ([string, string])[]) {
                                                let escapedPrereq = escapeCommand(prereqCommand, buildFile.require || {}, dir);
                                                let prereqResult = await runCommand("bash", ["-c", escapedPrereq]).catch(({ code, stdout, stderr }) => {
                                                    console.error(`   Error: Prerequisite command failed with code ${code}`);
                                                    console.info(stderr);
                                                    exit(1);
                                                });
                                                finalCommand = finalCommand.replaceAll(`$${prereqVar}`, prereqResult.stdout.trim());
                                            }
                                        }
                                        
                                        promises.push(runCommand("bash", ["-c", finalCommand]).then(() => {
                                            console.misc(`   -> ${finalCommand}`);
                                        }).catch(({ code, stdout, stderr }) => {
                                            console.error(`   Error: Command failed with code ${code}`);
                                            console.info(stderr);
                                            exit(1);
                                        }))
                                    }
                                }
                            }
                        }
                        await Promise.all(promises);
                    }
                }
                // finally, we collect the installed files
                if (typeof buildFile.install[depLocation.name] === "string") {
                    collectedFiles.push(escapeCommand(buildFile.install[depLocation.name], buildFile.require || {}, dir));
                } else {
                    for (let file of buildFile.install[depLocation.name]) {
                        if (typeof file === "string")
                            collectedFiles.push(escapeCommand(file, buildFile.require || {}, dir));
                        else {
                            let sub = []
                            for (let subfile of file) {
                                sub.push(escapeCommand(subfile, buildFile.require || {}, dir));
                            }
                            collectedFiles.push(sub);
                        }
                    }
                }
            }
        }
        if (projectConfiguration.locations.find(({name}: any) => name === depLocation.name).wasModified) {
            projectConfiguration.targets.find(({name}: any) => name === primaryTarget).wasModified = true;
            depsModified = true;

            let nextCommands = depLocation.build(collectedFiles);
            for (let command of nextCommands) {
                let commandStr = escapeCommand(command, depLocation.require, "./");
                if (command.prerequisites) {
                    for (let [prereqCommand, prereqVar] of command.prerequisites as ([string, string])[]) {
                        let escapedPrereq = escapeCommand(prereqCommand, command.require || {}, "./");
                        let prereqResult = await runCommand("bash", ["-c", escapedPrereq]).catch(({ code, stdout, stderr }) => {
                            console.error(`   Error: Prerequisite command failed with code ${code}`);
                            console.info(stderr);
                            exit(1);
                        });
                        commandStr = commandStr.replaceAll(`$${prereqVar}`, prereqResult.stdout.trim());
                    }
                }
                console.misc(`   -> ${commandStr}`);
                await runCommand("bash", ["-c", commandStr]).catch(({ code, stdout, stderr }) => {
                    console.error(`   Error: Command failed with code ${code}`);
                    console.info(stderr);
                    exit(1);
                });
            }
        } else {
            console.misc(`   -> Using cached build for "${depLocation.name}"`);
        }
    }
    // Step 3: We built all the dependencies to the target
    // so now we build the actual target
    // very simple since targets are assumed to be commands only with no dynamics
    if (!depsModified && fs.existsSync(targetOutputPath)) {
        console.misc(`   -> Using cached build for target "${primaryTarget}" (deps unchanged)`);
        return;
    }
    let targetPrerequisites: any = {};
    if (target.prerequisites) {
        for (let [prereqCommand, prereqVar] of target.prerequisites as ([string, string])[]) {
            let escapedPrereq = escapeCommand(prereqCommand, target.require || {}, "./");
            let prereqResult = await runCommand("bash", ["-c", escapedPrereq]).catch(({ code, stdout, stderr }) => {
                console.error(`   Error: Prerequisite command failed with code ${code}`);
                console.info(stderr);
                exit(1);
            });
            targetPrerequisites[prereqVar] = prereqResult.stdout.trim();
        }
    }
    target.wasModified = true;
    for (let command of target.build) {
        let commandStr = escapeCommand(command, target.require, "./");
        for (let [prereqVar, prereqValue] of Object.entries(targetPrerequisites) as [string, string][]) {
            commandStr = commandStr.replaceAll(`$${prereqVar}`, prereqValue);
        }
        console.misc(`   -> ${commandStr}`);
        await runCommand("bash", ["-c", commandStr]).catch(({ code, stdout, stderr }) => {
            console.error(`   Error: Command failed with code ${code}`);
            console.info(stderr);
            exit(1);
        });
    }
    console.log(`-> Built target "${primaryTarget}" successfully.`);
}

async function main() {
    if (primaryTarget.startsWith("debug-")) {
        let debugTarget = projectConfiguration.debug.find((d: any) => d.name === primaryTarget.replace("debug-", ""));
        let debugTargetName = primaryTarget.replace("debug-", "");
        if (!debugTarget) {
            console.error(`Error: Debug target "${primaryTarget.replace("debug-", "")}" not found.`);
            exit(1);
        }
        console.info(`-> Running debug: "${primaryTarget.replace("debug-", "")}"...`);
        for (let dep of projectConfiguration.debug.find((d: any) => d.name === primaryTarget.replace("debug-", "")).depends) {
            if (!projectConfiguration.targets.find((t: any) => t.name === dep)) {
                console.error(`Error: Target "${dep}" not found for debug.`);
                exit(1);
            }
            primaryTarget = dep;
            await buildTarget();
        }
        await new Promise(resolve => setTimeout(resolve, 1000));
        console.info(`-> Starting debug target "${debugTargetName}"...`);
        for (let command of debugTarget.run) {
            let commandStr = escapeCommand(command, debugTarget.require, "./");
            console.misc(`   -> ${commandStr}`);
            await runCommand("bash", ["-c", commandStr]).catch(({ code, stdout, stderr }) => {
                console.error(`   Error: Command failed with code ${code}`);
                console.info(stderr);
                exit(1);
            });
        }
        return;
    }
    await buildTarget();
}

if (import.meta.main === true) {
    if (!fs.existsSync("./build.config.js")) {
        console.error("Error: no project in directory (unable to find build.config.js).");
        exit(1);
    }
    projectConfiguration = (await import(`./build.config.js`)).default;

    if (process.argv.length > 2) {
        if (process.argv[2] === "clean") {
            console.info("-> Cleaning build directory...");
            if (fs.existsSync(projectConfiguration.build)) {
                fs.rmSync(projectConfiguration.build, { recursive: true, force: true });
            }
            console.info("-> Cleaned build directory.");
            exit(0);
        }
    }

    primaryTarget = process.argv.at(2) || projectConfiguration.targets.find((t: any) => t.default)?.name;
    if (!primaryTarget) {
        console.error("Error: No target specified and no default target set.");
        exit(1);
    }
    if (primaryTarget === "debug") {
        if (projectConfiguration.debug.find((t: any) => t.name === (process.argv.at(3))))
            primaryTarget = "debug-" + process.argv.at(3)
        else {
            console.error("Error: No debug target specified.");
            exit(1);
        }
    } else if (!projectConfiguration.targets.find((t: any) => t.name === primaryTarget)) {
        console.error(`Error: Target "${primaryTarget}" not found in build.config.js.`);
        exit(1);
    }

    await configure();
    await main();
}