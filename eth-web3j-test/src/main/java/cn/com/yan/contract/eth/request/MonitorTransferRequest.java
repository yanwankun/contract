package cn.com.yan.contract.eth.request;

import java.io.Serializable;

/**
 * Created with IDEA
 *
 * @author: gentlemen_k
 * @emali: test@qq.com
 **/
public class MonitorTransferRequest implements Serializable {

    private String contractAddress;

    private String firstBlock;

    public MonitorTransferRequest() {
    }

    public MonitorTransferRequest(String contractAddress, String firstBlock) {
        this.contractAddress = contractAddress;
        this.firstBlock = firstBlock;
    }

    public String getContractAddress() {
        return contractAddress;
    }

    public void setContractAddress(String contractAddress) {
        this.contractAddress = contractAddress;
    }

    public String getFirstBlock() {
        return firstBlock;
    }

    public void setFirstBlock(String firstBlock) {
        this.firstBlock = firstBlock;
    }

    @Override
    public String toString() {
        return "MonitorTransferRequest{" +
                "contractAddress='" + contractAddress + '\'' +
                ", firstBlock='" + firstBlock + '\'' +
                '}';
    }
}
