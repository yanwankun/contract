package cn.com.yan.contract.eth.config;

import org.web3j.abi.datatypes.generated.Uint256;

import java.math.BigDecimal;
import java.math.BigInteger;

/**
 * Created with IDEA
 *
 * @author: gentlemen_k
 * @emali: test@qq.com
 **/
public class Config {

    /**
     * 智能合约配置地址
     * @return
     */
    public static String getSmartContractAddress() {
        return "89885fc1f76c3f4cc719640e33c315227da7003a";
    }

    /**
     * 配置用户钱包地址
     * @return
     */
    public static String getWalletAddress() {
        return "89885fc1f76c3f4cc719640e33c315227da7003a";
    }

    /**
     * walletIPc配置信息
     * @return
     */
    public static String getIpcAddress() {
        return "/geth/data/geth.ipc";
    }

    public static void main(String[] args) {
        String test  = "00000000000000000000000000000000000000000000d3c29fa5949857360000";

        String te = new BigInteger(test, 16).toString();

        BigDecimal decimal = new BigDecimal(te).divide(new BigDecimal("1000000000000000000"));
        System.out.println("te :" + decimal);

        BigInteger bigInteger = decimal.multiply(new BigDecimal("1000000000000000000")).toBigIntegerExact();
        Uint256 uint256 = new Uint256(bigInteger);
        System.out.println("uint256 : " + uint256);
    }
}
