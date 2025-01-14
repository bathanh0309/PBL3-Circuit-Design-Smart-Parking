//Khai bao thu vien
const express = require('express'); //Declares and uses Express.js, aNode.js framework for building web applications
const mqtt = require('mqtt'); //Declares and uses the MQTT protocol, a lightweight messaging protocol for IOT
const path = require('path'); //khai bao module path de lam viec voi cac duong dan tep

const app = express(); // khoi tao mot ung dung express
const port = 3000; // khoi tao port

// Kết nối MQTT đến EMQX
const clientId = 'client_' + Math.random().toString(16).substr(2, 8);
const client = mqtt.connect('ws://broker.emqx.io:8083/mqtt', {
    clientId,
    username: 'hieutk1302', 
    password: 'hieutk1302',   
    reconnectPeriod: 1000,
    connectTimeout: 30 * 1000,
    will: {
        topic: 'WillMsg',
        payload: 'Connection Closed abnormally..!',
        qos: 0,
        retain: false
    }
});



app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

app.use(express.static(path.join(__dirname, 'public')));

app.listen(port, () => {
    console.log(`Server running at http://localhost:${port}/`);
});
