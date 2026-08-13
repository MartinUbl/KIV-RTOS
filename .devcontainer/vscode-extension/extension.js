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
            }
        }
    );

    const build = vscode.commands.registerCommand(
        "kernel.build",
        async () => {
            const terminal = getTerminal("KIV-RTOS Build");
            terminal.show();
            terminal.sendText("cd /workspaces/KIV-RTOS/sources && ./build.sh " + expansionBoardSelect.text.split(": ")[1]);
        }
    );

    const flash = vscode.commands.registerCommand(
        "kernel.flash",
        async () => {
            const terminal = getTerminal("KIV-RTOS Flash");
            terminal.show();
            terminal.sendText("uart_flasher /workspaces/KIV-RTOS/sources/build/kernel.srec /dev/ttyCOM");
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

    context.subscriptions.push(selectExpansionBoard, build, flash, serial);

    const expansionBoardSelect = vscode.window.createStatusBarItem(
        vscode.StatusBarAlignment.Left,
        101
    );

    expansionBoardSelect.text = "$(game) Expansion Board: None";
    expansionBoardSelect.tooltip = "Select expansion board";
    expansionBoardSelect.command = "kernel.selectExpansionBoard";
    expansionBoardSelect.show();

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

    const serialButton = vscode.window.createStatusBarItem(
        vscode.StatusBarAlignment.Left,
        98
    );

    serialButton.text = "$(terminal) Serial";
    serialButton.tooltip = "Open serial terminal";
    serialButton.command = "kernel.serial";
    serialButton.show();

    context.subscriptions.push(
        expansionBoardSelect,
        buildButton,
        flashButton,
        serialButton
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

function deactivate() {}

module.exports = {
    activate,
    deactivate
};
