package cn.com.yan.contract.eth.utils;

import cn.com.yan.contract.eth.config.Config;
import cn.com.yan.contract.eth.record.TransferRecord;
import cn.com.yan.contract.eth.request.MonitorTransferRequest;
import com.alibaba.fastjson.JSON;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Component;
import org.web3j.abi.EventEncoder;
import org.web3j.abi.TypeReference;
import org.web3j.abi.datatypes.Address;
import org.web3j.abi.datatypes.Event;
import org.web3j.abi.datatypes.generated.Uint256;
import org.web3j.protocol.Web3j;
import org.web3j.protocol.core.DefaultBlockParameter;
import org.web3j.protocol.core.DefaultBlockParameterName;
import org.web3j.protocol.core.methods.request.EthFilter;
import org.web3j.protocol.core.methods.request.Transaction;
import org.web3j.protocol.core.methods.response.EthBlock;
import org.web3j.protocol.core.methods.response.EthBlockNumber;
import org.web3j.protocol.http.HttpService;
import org.web3j.protocol.ipc.UnixIpcService;
import rx.Subscription;

import java.io.IOException;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.util.*;

/**
 * Created with IDEA
 *
 * @author: gentlemen_k
 * @emali: test@qq.com
 **/
@Component
public class Web3jUtils {

    private static final Logger LOG = LoggerFactory.getLogger(Web3jUtils.class);


    private Web3j web3j;
    private ThreadLocal<Set<BigInteger>> blockNumberSet = ThreadLocal.withInitial(HashSet::new);

    public void init() {
        web3j = Web3j.build(new UnixIpcService(Config.getIpcAddress()));
    }


    public void getSmartContractBalance(String contractAddress, String userAddress) {
        init();
        BigInteger integer;
        String transactionStr = "0x70a08231000000000000000000000000" + userAddress;
        BigDecimal amount;
        try {
            integer = web3j.ethGetBalance("0x478FA8925638550CB19F0B2d171DE08F51EdF458",DefaultBlockParameterName.LATEST).send().getBalance();
            LOG.info("myBalance :" + integer);



            String balance = web3j.ethCall(Transaction.createEthCallTransaction("0x478FA8925638550CB19F0B2d171DE08F51EdF458",contractAddress, transactionStr), DefaultBlockParameterName.PENDING).send().getValue();
            LOG.info("balance :" + balance);
            amount = new BigDecimal(balance);
            LOG.info("amount : " + amount);

        } catch (IOException e) {
            e.printStackTrace();
            amount = new BigDecimal(0);
        }

        LOG.info("amount : " + amount);
    }

    /**
     * 监听合约(分步)
     */
    public void contractFilterOfStep(MonitorTransferRequest monitorTransferRequest) {
        new Thread(() -> {
            try {
                BigInteger blockNumber = getBlockNumber();
                LOG.info("BlockChainNumber={}", blockNumber);
                BigInteger startBlock = new BigInteger(monitorTransferRequest.getFirstBlock());
                BigInteger step = new BigInteger("100");
                assert blockNumber != null;
                int num = Integer.parseInt(String.valueOf(blockNumber.subtract(startBlock).divide(step)));
                for (int i = 0; i < num; i++) {
                    // 监听合约(分步)
                    contractFilterStep(monitorTransferRequest, startBlock, startBlock.add(step));
                    startBlock = startBlock.add(step);
                }
                // 监听合约
                contractFilter(monitorTransferRequest, startBlock);
            } catch (NumberFormatException e) {
                LOG.error("监控合约异常终止,e={}", e.getMessage());
            }
        }).start();
    }

    /**
     * 监听合约
     */
    private void contractFilter(MonitorTransferRequest monitorTransferRequest, BigInteger startBlock) {
        Event event = new Event("Transfer",
                Arrays.asList(
                        new TypeReference<Address>() {
                        },
                        new TypeReference<Address>() {
                        }),
                Collections.singletonList(new TypeReference<Uint256>() {
                }));
        EthFilter filter = new EthFilter(
                DefaultBlockParameter.valueOf(startBlock),
                DefaultBlockParameterName.LATEST,
                monitorTransferRequest.getContractAddress());
        filter.addSingleTopic(EventEncoder.encode(event));
        // 监听并处理
        contractExtract(filter);
    }

    /**
     * 监听并处理
     */
    private void contractExtract(EthFilter filter) {
        Subscription subscription = web3j.ethLogObservable(filter).subscribe(log -> {
            BigInteger blockNumber = log.getBlockNumber();
            LOG.info("BlockNumber=", blockNumber);
            blockNumberSet.get().add(blockNumber);
            List<String> topics = log.getTopics();
            String fromAddress = topics.get(1);
            String toAddress = topics.get(2);
            String value = log.getData();
            String timestamp = "";

            try {
                EthBlock ethBlock = web3j.ethGetBlockByNumber(DefaultBlockParameter.valueOf(log.getBlockNumber()), false).send();
                timestamp = String.valueOf(ethBlock.getBlock().getTimestamp());
            } catch (IOException e) {
                LOG.warn("Block timestamp get failure,block number is {}", log.getBlockNumber());
                LOG.error("Block timestamp get failure,{}", e);
            }

            TransferRecord transferRecord = new TransferRecord();
            transferRecord.setFromAddress("0x" + fromAddress.substring(26));
            transferRecord.setToAddress("0x" + toAddress.substring(26));
            transferRecord.setValue(new BigDecimal(new BigInteger(value.substring(2), 16)).divide(BigDecimal.valueOf(1000000000000000000.0), 18, BigDecimal.ROUND_HALF_EVEN));
            transferRecord.setTimeStamp(timestamp);
        });
//        subscription.unsubscribe();
    }

    /**
     * 监听合约(分步)
     */
    private void contractFilterStep(MonitorTransferRequest monitorTransferRequest, BigInteger startBlock, BigInteger endBlock) {
        Event event = new Event("Transfer",
                Arrays.asList(
                        new TypeReference<Address>() {
                        },
                        new TypeReference<Address>() {
                        }),
                Collections.singletonList(new TypeReference<Uint256>() {
                }));
        EthFilter filter = new EthFilter(
                DefaultBlockParameter.valueOf(startBlock),
                DefaultBlockParameter.valueOf(endBlock),
                monitorTransferRequest.getContractAddress());
        filter.addSingleTopic(EventEncoder.encode(event));
        // 监听并处理
        contractExtract(filter);
    }

    /**
     * 区块数量
     */
    private BigInteger getBlockNumber() {
        EthBlockNumber send;
        try {
            send = web3j.ethBlockNumber().send();
            return send.getBlockNumber();
        } catch (IOException e) {
            LOG.warn("请求区块链信息异常 >> 区块数量,{}", e);
        }
        return null;
    }

    /**
     * 遍历旧区块
     *
     * @param startBlock 起始区块高度
     * @param endBlock   截止区块高度
     */
    public void replayBlocksObservable(BigInteger startBlock, BigInteger endBlock) {
        web3j.replayBlocksObservable(
                DefaultBlockParameter.valueOf(startBlock),
                DefaultBlockParameter.valueOf(endBlock),
                false).
                subscribe(ethBlock -> {
                    EthBlock.Block block = ethBlock.getBlock();
                    System.out.println("replay block, BlockNumber=" + block.getNumber());
                    System.out.println(JSON.toJSONString(block));
                });
    }

    /**
     * 遍历旧交易
     *
     * @param startBlock 起始区块高度
     * @param endBlock   截止区块高度
     */
    public void replayTransactionsObservable(BigInteger startBlock, BigInteger endBlock) {
        web3j.replayTransactionsObservable(
                DefaultBlockParameter.valueOf(startBlock),
                DefaultBlockParameter.valueOf(endBlock)).
                subscribe(transaction -> {
                    System.out.println("replay transaction, BlockNumber=" + transaction.getBlockNumber());
                    System.out.println(JSON.toJSONString(transaction));
                });
    }

    /**
     * 遍历旧区块,监听新区快
     *
     * @param startBlock 起始区块高度
     */
    public void catchUpToLatestAndSubscribeToNewBlocksObservable(BigInteger startBlock) {
        web3j.catchUpToLatestAndSubscribeToNewBlocksObservable(
                DefaultBlockParameter.valueOf(startBlock), false)
                .subscribe(ethBlock -> {
                    EthBlock.Block block = ethBlock.getBlock();
                    System.out.println("block," + block.getNumber());
                    System.out.println(JSON.toJSONString(block));
                });
    }

    /**
     * 遍历旧交易,监听新交易
     *
     * @param startBlock 起始区块高度
     */
    public void catchUpToLatestAndSubscribeToNewTransactionsObservable(BigInteger startBlock) {
        web3j.catchUpToLatestAndSubscribeToNewTransactionsObservable(
                DefaultBlockParameter.valueOf(startBlock))
                .subscribe(transaction -> {
                    System.out.println("transaction, BlockNumber=" + transaction.getBlockNumber());
                    System.out.println(JSON.toJSONString(transaction));
                });
    }

}
