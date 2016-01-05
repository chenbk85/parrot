var WebSocketClient = require('websocket').client;

var client = new WebSocketClient();
var pktLenIdx = 65526;

function getVariableNumber(buff, offset) {
    var len = buff.readUInt8(offset++, true);
    var numLen = 0;
    if (len < 254) {
        numLen = 1;
    } else if (len === 254) {
        len = buff.readUInt16BE(offset, true);
        numLen = 3;
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
        numLen = 9;
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

//    console.log('parseBinaryData len is %d. offset is %d', len, offset);    
    
    var newBuff = new Buffer(len);
    buff.copy(newBuff, 0, offset, buff.length);    
    if (type === 1 || type === 2) {
        console.log(newBuff);        
        // Data is json.
        var str = newBuff.toString();
        var json = JSON.parse(str);

        console.log('parseItem: json is %j', json);
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
        console.log('message');
        if (message.type === 'utf8') {
            throw new Error('Bad encoding type.');
        }
        
        if (message.type === 'binary') {
            console.log(message.binaryData);
            parseBinaryData(message.binaryData);
        }

        process.nextTick(sendData, connection);
    });

    var i = 0;
    var reqId = 0;
    function sendData(conn) {
        if (conn.connected) {
            var sysJson = {
                route: 1,
                type: 2,
                reqId: reqId
            };
            var dataJson = {
                str: 'Hello world.' + reqId
            };

            ++reqId;

            var sysJsonStr = JSON.stringify(sysJson);
            var dataJsonStr = JSON.stringify(dataJson);
            var buff = new Buffer(2 + sysJsonStr.length + 2 + dataJsonStr.length);
            var idx = 0;
            buff.writeUInt8(1, idx++);
            buff.writeUInt8(sysJsonStr.length, idx++);
            buff.write(sysJsonStr, idx, sysJsonStr.length, 'utf8');
            idx += sysJsonStr.length;
            buff.writeUInt8(2, idx++);
            buff.writeUInt8(dataJsonStr.length, idx++);
            buff.write(dataJsonStr, idx, dataJsonStr.length, 'utf8');

            conn.send(buff);

            if (i < 2) {
                setTimeout(sendData.bind(null, connection), 1000);
            }
        }
    }
    sendData(connection);
});

client.connect('ws://10.24.100.176:9898/');
