var serviceId = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
var characteristicId = "beb5483e-36e1-4688-b7f5-ea07361b26a8";

var myCharacteristic = null;
var dec = new TextDecoder();
var enc = new TextEncoder();

document.querySelector("#connect").onclick = function () {
  navigator.bluetooth.requestDevice({
    filters: [{ services: [serviceId] }]
  }).then(device => {
    return device.gatt.connect();
  }).then(server => {
    return server.getPrimaryService(serviceId);
  }).then(service => {
    return service.getCharacteristic(characteristicId);
  }).then(characteristic => {
    myCharacteristic = characteristic;
  }).catch(error => {
    console.error("Error connecting to the Bluetooth device:", error);
  });
};


// Function to send message
function sendMessage(message) {
  if (!myCharacteristic) {
    alert("Please connect to the Bluetooth device first.");
    return;
  }

  myCharacteristic.writeValue(enc.encode(message)).then(() => {
    console.log("Sent message:", message);
  }).catch(error => {
    console.error("Error sending message:", error);
  });
}


let timeData = [];  // Timestamps
let tempData = [];  // Temperature readings

// Initialize Empty Graph
Plotly.newPlot('temperatureGraph', [{
    x: timeData,
    y: tempData,
    mode: 'lines+markers',
    type: 'scatter',
    marker: { color: 'red' }
}], {
    title: 'Real-Time Temperature',
    xaxis: { title: 'Time' },
    yaxis: { title: 'Temperature (°C)' }
});

function appendToGraph() {
  if (!myCharacteristic) {
    console.error("Bluetooth device not connected");
    return;
  }

  myCharacteristic.readValue().then(value => {
    // Assuming the temperature is in the value of the characteristic in Fahrenheit
    let temperature = parseFloat(dec.decode(value)); // Decode and parse the temperature

    let currentTime = new Date(); // Get current timestamp
    let formattedTime = currentTime.toLocaleTimeString(); // Get readable time

    // Append the temperature and time to the data arrays
    timeData.push(formattedTime);
    tempData.push(temperature);

    // If the arrays exceed 15 items, remove the oldest one
    if (timeData.length > 15) {
      timeData.shift();
      tempData.shift();
    }

    // Update the graph with the new data
    Plotly.update('temperatureGraph', {
      x: [timeData],
      y: [tempData]
    });

    console.log(`Temperature: ${temperature} °F, Time: ${formattedTime}`);
  }).catch(error => {
    console.error("Error reading temperature:", error);
  });
}

// Append Data Every 10 Seconds
setInterval(appendToGraph, 10000); // Update every 10 seconds
