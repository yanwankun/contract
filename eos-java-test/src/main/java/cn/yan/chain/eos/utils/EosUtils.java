package cn.yan.chain.eos.utils;

import cn.yan.chain.eos.Asset;
import com.alibaba.fastjson.JSON;
import io.jafka.jeos.EosApi;
import io.jafka.jeos.EosApiFactory;
import io.jafka.jeos.LocalApi;
import io.jafka.jeos.convert.Packer;
import io.jafka.jeos.core.common.SignArg;
import io.jafka.jeos.core.common.transaction.PackedTransaction;
import io.jafka.jeos.core.common.transaction.TransactionAction;
import io.jafka.jeos.core.common.transaction.TransactionAuthorization;
import io.jafka.jeos.core.request.chain.transaction.PushTransactionRequest;
import io.jafka.jeos.core.response.chain.transaction.PushedTransaction;
import io.jafka.jeos.impl.EosApiRestClientImpl;
import io.jafka.jeos.util.KeyUtil;
import io.jafka.jeos.util.Raw;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.List;

/**
 * Created with IDEA
 *
 * @author: gentlemen_k
 * @emali: test@qq.com
 **/
public class EosUtils {

    public static String kylin_url = "http://kylin.fn.eosbixin.com";
    public static EosApi client;

    public void init() {
        client = new EosApiRestClientImpl(kylin_url);
    }


    public Asset getBalance(String token, String account, String tokenName) {
        init();
        List<String> ret = client.getCurrencyBalance(token,account,tokenName);

        Asset asset = new Asset(ret.get(0));
        System.out.println(JSON.toJSONString(asset));
        return asset;
    }

    public void callSmartContract(String contractName, String methodName, String privateKey, String accountName) {
        EosApi eosApi = EosApiFactory.create(kylin_url);
        SignArg arg = eosApi.getSignArg(120);
        Raw raw = new Raw();

        raw.packAsset("10.0000 EOS");
        raw.packName("eosio.token");
        raw.pack(1001);
        raw.pack(1000001);

        String transferData = raw.toHex();
        List<TransactionAuthorization> authorizations = Arrays.asList(new TransactionAuthorization(accountName, "active"));

        List<TransactionAction> actions = Arrays.asList(
                new TransactionAction(contractName , methodName, authorizations, transferData)
        );
        PackedTransaction packedTransaction = new PackedTransaction();
        packedTransaction.setExpiration(arg.getHeadBlockTime().plusSeconds(arg.getExpiredSecond()));
        packedTransaction.setRefBlockNum(arg.getLastIrreversibleBlockNum());
        packedTransaction.setRefBlockPrefix(arg.getRefBlockPrefix());

        packedTransaction.setMaxNetUsageWords(0);
        packedTransaction.setMaxCpuUsageMs(0);
        packedTransaction.setDelaySec(0);
        packedTransaction.setActions(actions);

        String hash = sign(privateKey, arg, packedTransaction);
        PushTransactionRequest req = new PushTransactionRequest();
        req.setTransaction(packedTransaction);
        req.setSignatures(Arrays.asList(hash));

        PushedTransaction pts = eosApi.pushTransaction(req);

    }

    private String sign(String privateKey, SignArg arg, PackedTransaction t) {
        Raw raw = Packer.packPackedTransaction(arg.getChainId(), t);
        raw.pack(ByteBuffer.allocate(33).array());
        String hash = KeyUtil.signHash(privateKey, raw.bytes());
        return hash;
    }

}
