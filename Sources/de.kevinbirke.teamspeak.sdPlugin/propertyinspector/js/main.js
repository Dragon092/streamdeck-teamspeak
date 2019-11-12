// Setup the websocket and handle communication
function connectElgatoStreamDeckSocket(inPort, inUUID, inRegisterEvent, inInfo, inActionInfo) {
    // Parse parameter from string to object
    var actionInfo = JSON.parse(inActionInfo);
    var info = JSON.parse(inInfo);

    // Retrieve action identifier
    var action = actionInfo['action'];

    // Create actions
    if (action == "de.kevinbirke.teamspeak.microphone") {
        // Add brightness slider
        var brightnessSlider = "<div type='range' class='sdpi-item'> \
        <div class='sdpi-item-label' id='brightness-label'></div> \
        <div class='sdpi-item-value'> \
            <input class='floating-tooltip' data-suffix='%' type='range' id='brightness-input' min='1' max='100' value='0'> \
        </div> \
        </div>";
        document.getElementById('placeholder').innerHTML = brightnessSlider;
    }
}
