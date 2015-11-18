var WebSocketClient = require('websocket').client;

var client = new WebSocketClient();

function getVariableNumber(buff, offset) {
    var len = buff.readUInt8(offset++, true);
    var numLen = 0;
    if (len < 254) {
        numLen = 1;
    } else if (len === 254) {
        len = buff.readUInt16BE(offset, true);
        numLen = 2;
    } else if (len === 255) {
        if (buff.length < 9) {
            console.log('Buff is not right. 254');
            return null;
        }
        var high = buff.readUInt32BE(offset, true);
        offset += 4;
        high <<= 32;
        var low = buff.readUInt32BE(offset, true);
        len     = high | low;
        numLen = 8;
    }

    return {n : len, o : numLen};
}

function parseItem(buff, offset) {
    if (buff.length <= offset) {
        console.log('parseItem:: bad buff1.');        
        return;
    }

    var type = buff.readUInt8(offset++, true);

    console.log('type is ' + type);
    
    var ret = getVariableNumber(buff, offset);
    var len = 0;    

    if (!ret) {
        return;
    }

    len = ret.n;
    offset += ret.o;

    if (buff.len < offset + len) {
        console.log('parseItem:: bad buff2.');
        return;
    }

    console.log('parseBinaryData len is %d. offset is %d', len, offset);    
    
    var newBuff = new Buffer(len);
    buff.copy(newBuff, 0, offset, buff.length);    
    if (type === 1) {
        // Data is json.
        var str = newBuff.toString();
        console.log('parseItem: json is %j', JSON.parse(str));
    } else if (type === 2) {
        // Data is binary.
        console.log('parseItem: Binary');
    }
}

function parseBinaryData(buff) {
    if (buff.length === 0) {
        console.log('Buff is zero.');
        return;
    }

    var offset = 0;
    var route = 0;
    var ret = getVariableNumber(buff, offset);

    if (!ret) {
        return;
    }

    console.log('parseBinaryData ret is %j', ret);
    
    offset += ret.o;
    route = ret.n;
    
    while (offset < buff.length) {
        offset = parseItem(buff, offset);
    }
}


client.on('connectFailed', function(error) {
    console.log('Connect Error: ' + error.toString());
});

client.on('connect', function(connection) {
    console.log('WebSocket Client Connected');
    connection.on('error', function(error) {
        console.log("Connection Error: " + error.toString());
    });
    connection.on('close', function() {
        console.log('echo-protocol Connection Closed');
    });
    connection.on('message', function(message) {
        if (message.type === 'utf8') {
            throw new Error('Bad encoding type.');
        }
        
        if (message.type === 'binary') {
            console.log(message.binaryData);
            parseBinaryData(message.binaryData);
        }
    });

    var i = 0;
    function sendNumber() {
        if (connection.connected) {
            var str = 'HelloWorld' + i++;
            var buff = new Buffer(str.length);            
            buff.write(str, 0, str.length, 'utf8');
            connection.send(buff);
            setTimeout(sendNumber, 1000);
        }
    }
    sendNumber();
});

client.connect('ws://10.24.100.202:9898/');
