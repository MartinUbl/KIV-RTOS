const vscode = require("vscode");

function activate(context) {

    const selectExpansionBoard = vscode.commands.registerCommand(
        "kernel.selectExpansionBoard",
        async () => {
            const expansionBoards = [
                "None",
                "KIV-DPP-01",
                "KIV-DPP-02"
            ];
            const selected = await vscode.window.showQuickPick(expansionBoards, {
                placeHolder: "Select an expansion board"
            });

            if (selected) {
                expansionBoardSelect.text = `$(game) Expansion Board: ${selected}`;
                context.globalState.update('expansionBoard', selected);
            }
        }
    );

    const selectFlashMode = vscode.commands.registerCommand(
        "kernel.selectFlashMode",
        async () => {
            const flashModes = [
                "SREC (old)",
                "SREC-200",
                "SREC-200+"
            ];
            const selected = await vscode.window.showQuickPick(flashModes, {
                placeHolder: "Select flash mode"
            });

            if (selected) {
                flashModeSelect.text = `$(fold-up) Flash mode: ${selected}`;
                context.globalState.update('flashMode', selected);
            }
        }
    );

    const build = vscode.commands.registerCommand(
        "kernel.build",
        async () => {

            const board = context.globalState.get('expansionBoard') ? context.globalState.get('expansionBoard') : "None";

            const terminal = getTerminal("KIV-RTOS Build");
            terminal.show();
            terminal.sendText("cd /workspaces/KIV-RTOS/sources && ./build.sh " + board);
        }
    );

    const flash = vscode.commands.registerCommand(
        "kernel.flash",
        async () => {
            const flashJob = prepareFlash(context);

            const closeListener = vscode.window.onDidCloseTerminal(async closedTerminal => {
                if (closedTerminal !== flashJob.terminal) {
                    return;
                }

                closeListener.dispose();

                if (closedTerminal.exitStatus?.code === 0) {
                    vscode.window.showInformationMessage("Kernel flashed successfully.");
                }
                else {
                    vscode.window.showErrorMessage("Kernel flashing failed.");
                }
            });

            flashJob.terminal.sendText(flashJob.command);
        }
    );

    const flashAndSerial = vscode.commands.registerCommand(
        "kernel.flashAndSerial",
        async () => {
            const flashJob = prepareFlash(context);

            const closeListener = vscode.window.onDidCloseTerminal(async closedTerminal => {
                if (closedTerminal !== flashJob.terminal) {
                    return;
                }

                closeListener.dispose();

                if (closedTerminal.exitStatus?.code === 0) {
                    vscode.window.showInformationMessage("Kernel flashed successfully.");
                    await vscode.commands.executeCommand("kernel.serial");
                }
                else {
                    vscode.window.showErrorMessage("Kernel flashing failed.");
                }
            });

            flashJob.terminal.sendText(flashJob.command);
        }
    );

    const serial = vscode.commands.registerCommand(
        "kernel.serial",
        async () => {
            const terminal = getTerminal("Serial");
            terminal.show();
            terminal.sendText(
                "picocom -q -b 115200 /dev/ttyCOM"
            );
        }
    );

    const clean = vscode.commands.registerCommand(
        "kernel.clean",
        async () => {
            const terminal = getTerminal("KIV-RTOS Build");
            terminal.show();
            terminal.sendText(
                "./build.sh clean"
            );
        }
    );

    context.subscriptions.push(selectExpansionBoard, selectFlashMode, build, flash, serial, clean, flashAndSerial);

    const expansionBoardSelect = vscode.window.createStatusBarItem(
        vscode.StatusBarAlignment.Left,
        105
    );

    const selectedExpansionBoard = context.globalState.get('expansionBoard');

    expansionBoardSelect.text = "$(game) Expansion Board: " + (selectedExpansionBoard ? selectedExpansionBoard : "None");
    expansionBoardSelect.tooltip = "Select expansion board";
    expansionBoardSelect.command = "kernel.selectExpansionBoard";
    expansionBoardSelect.show();

    const flashModeSelect = vscode.window.createStatusBarItem(
        vscode.StatusBarAlignment.Left,
        102
    );

    const selectedFlashMode = context.globalState.get('flashMode');

    flashModeSelect.text = "$(fold-up) Flash mode: " + (selectedFlashMode ? selectedFlashMode : "SREC (old)");
    flashModeSelect.tooltip = "Select flash mode";
    flashModeSelect.command = "kernel.selectFlashMode";
    flashModeSelect.show();

    const buildButton = vscode.window.createStatusBarItem(
        vscode.StatusBarAlignment.Left,
        100
    );

    buildButton.text = "$(tools) Build";
    buildButton.tooltip = "Build kernel";
    buildButton.command = "kernel.build";
    buildButton.show();

    const flashButton = vscode.window.createStatusBarItem(
        vscode.StatusBarAlignment.Left,
        99
    );

    flashButton.text = "$(rocket) Flash";
    flashButton.tooltip = "Flash kernel";
    flashButton.command = "kernel.flash";
    flashButton.show();

    const flashAndSerialButton = vscode.window.createStatusBarItem(
        vscode.StatusBarAlignment.Left,
        98
    );

    flashAndSerialButton.text = "$(pulse) Flash & Serial";
    flashAndSerialButton.tooltip = "Flash kernel and open serial terminal";
    flashAndSerialButton.command = "kernel.flashAndSerial";
    flashAndSerialButton.show();

    const cleanButton = vscode.window.createStatusBarItem(
        vscode.StatusBarAlignment.Left,
        97
    );

    cleanButton.text = "$(trash) Clean";
    cleanButton.tooltip = "Clean build";
    cleanButton.command = "kernel.clean";
    cleanButton.show();

    const serialButton = vscode.window.createStatusBarItem(
        vscode.StatusBarAlignment.Left,
        96
    );

    serialButton.text = "$(terminal) Serial";
    serialButton.tooltip = "Open serial terminal";
    serialButton.command = "kernel.serial";
    serialButton.show();

    context.subscriptions.push(
        expansionBoardSelect,
        flashModeSelect,
        buildButton,
        flashButton,
        flashAndSerialButton,
        serialButton,
        cleanButton
    );
}

function getTerminal(name) {
    const existing = vscode.window.terminals.find(
        terminal => terminal.name === name
    );

    if (existing) {
        return existing;
    }

    return vscode.window.createTerminal(name);
}

function prepareFlash(context) {
    const serialTerminal = vscode.window.terminals.find(
        terminal => terminal.name === "Serial"
    );

    if (serialTerminal) {
        serialTerminal.dispose();
    }

    const flashMode = context.globalState.get('flashMode') ? context.globalState.get('flashMode') : "SREC (old)";

    const terminal = getTerminal("KIV-RTOS Flash");
    terminal.show();

    let command;
    if (flashMode == "SREC-200+") {
        command = "uart_flasher -b --baud 921600 /workspaces/KIV-RTOS/sources/build/kernel.elf /dev/ttyCOM && exit 0";
    }
    else if (flashMode == "SREC-200") {
        command = "uart_flasher -b /workspaces/KIV-RTOS/sources/build/kernel.elf /dev/ttyCOM && exit 0";
    }
    else {
        command = "uart_flasher /workspaces/KIV-RTOS/sources/build/kernel.srec /dev/ttyCOM && exit 0";
    }

    return { terminal, command };
}

function deactivate() {}

module.exports = {
    activate,
    deactivate
};
