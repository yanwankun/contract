package cn.yan.chain.eos;

import java.math.BigDecimal;

/**
 * Created with IDEA
 *
 * @author: gentlemen_k
 * @emali: test@qq.com
 **/
public class Asset {

    private String assetName;
    private BigDecimal amount;

    public String getAssetName() {
        return assetName;
    }

    public void setAssetName(String assetName) {
        this.assetName = assetName;
    }

    public BigDecimal getAmount() {
        return amount;
    }

    public void setAmount(BigDecimal amount) {
        this.amount = amount;
    }

    public Asset(String str) {
        String[] temp = str.split(" ");
        this.amount = new BigDecimal(temp[0]);
        this.assetName = temp[1];
    }
}
