var GameDiceToken = artifacts.require("./GameDiceToken.sol");

module.exports = function(deployer) {
    deployer.deploy(GameDiceToken, 10000000000, "YWKTS", 5, "WK");
};
