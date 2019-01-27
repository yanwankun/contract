package cn.com.yan.contract.eth.test;

import cn.com.yan.contract.eth.config.Config;
import cn.com.yan.contract.eth.utils.Web3jUtils;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.InitializingBean;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;

/**
 * Created with IDEA
 *
 * @author: gentlemen_k
 * @emali: test@qq.com
 **/

@Slf4j
@Component
public class Web3JTest  implements InitializingBean {

    @Autowired
    private Web3jUtils web3jUtils;


    /**
     * 每隔几秒钟执行一次操作
     */
    @Scheduled(cron = "0/3 * * * * *")
    public void scheduled(){
        web3jUtils.getSmartContractBalance(Config.getSmartContractAddress(), Config.getWalletAddress());
    }


    @Override
    public void afterPropertiesSet() {
        System.out.println("程序已关闭");
    }
}
