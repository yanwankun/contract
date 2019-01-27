package cn.yan.chain.eos.test;

import cn.yan.chain.eos.Asset;
import cn.yan.chain.eos.utils.EosUtils;
import com.alibaba.fastjson.JSON;

/**
 * Created with IDEA
 *
 * @author: gentlemen_k
 * @emali: test@qq.com
 **/
public class EosUtilsTest {
    private static EosUtils eosUtils = new EosUtils();

    public static void getBalanceTest() {
        eosUtils.getBalance("myeostokeny1", "myeosgame111", "DTC");
    }

    public static void main(String[] args) {
        call();
    }

    public static void call() {
        eosUtils.callSmartContract("myeosgame111", "updatepool", "5KWt1x6mYhC4ryoso5QVU94whsuLP7fkrTPvHio8r16tJax25U7", "myeosgame111");
    }
}
