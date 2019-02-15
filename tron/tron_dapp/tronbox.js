module.exports = {
    networks: {
        shasta: {
            privateKey: '84d9f2d42a52839ca2ed6992af7f2e9617263a3eb9a358f25ccddf704f7826a4',
            consume_user_resource_percent: 30,
            fullNode: "http://127.0.0.1:8090",
            solidityNode: "http://127.0.0.1:8091",
            eventServer: "http://127.0.0.1:8092",
            network_id: "*"
        },
        development: {
            privateKey: '84d9f2d42a52839ca2ed6992af7f2e9617263a3eb9a358f25ccddf704f7826a4',
            consume_user_resource_percent: 30,
            fullNode: "http://127.0.0.1:8090",
            solidityNode: "http://127.0.0.1:8091",
            eventServer: "http://127.0.0.1:8092",
            network_id: "*"
        }
    }
};