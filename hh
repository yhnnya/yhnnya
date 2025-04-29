/**
 * @fileoverview 君品荟自动登录并签到（支持多账户）
 * @author 
 * @QuantumultX
 * @run-at script
 * @name 君品荟签到
 */

const PHONES = $prefs.valueForKey("JPH_PHONES");
const PASSWORDS = $prefs.valueForKey("JPH_PASSWORD_ENCS");

const BASE_URL = "https://fm.exijiu.com";
const LOGIN_URL = `${BASE_URL}/api/login/phoneLogin`;
const CHECK_SIGN_URL = `${BASE_URL}/api/customer/daily/checkTodaySignIn`;
const SIGN_IN_URL = `${BASE_URL}/api/customer/daily/fillSignIn`;

if (!PHONES || !PASSWORDS) {
  console.log("请设置 JPH_PHONES 和 JPH_PASSWORD_ENCS 环境变量！");
  $done();
  return;
}

const phones = PHONES.split("&");
const encryptedPasswords = PASSWORDS.split("&");

if (phones.length !== encryptedPasswords.length) {
  console.log(`账号数量不一致！手机号:${phones.length}, 密码:${encryptedPasswords.length}`);
  $done();
  return;
}

console.log(`共检测到 ${phones.length} 个账户，开始处理...\n`);

(async () => {
  let tokens = [];

  for (let i = 0; i < phones.length; i++) {
    const phone = phones[i];
    const encryptedPassword = encryptedPasswords[i];
    const accountName = `账户 ${i + 1} (${phone.slice(0, 3)}****${phone.slice(-4)})`;

    try {
      console.log(`\n[${accountName}] 开始登录...`);

      const loginResp = await $task.fetch({
        method: "POST",
        url: LOGIN_URL,
        headers: {
          "Content-Type": "application/json",
          "Origin": "https://mall.exijiu.com",
          "Referer": "https://mall.exijiu.com/",
          "User-Agent": "Mozilla/5.0 (iPhone; CPU iPhone OS 15_8_3 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Mobile/15E148",
          "dataType": "json",
          "Channel": "app"
        },
        body: JSON.stringify({
          phone: phone,
          password: encryptedPassword,
          channelCode: "xj_mall"
        })
      });

      const loginJson = JSON.parse(loginResp.body);
      if (loginJson.success && loginJson.code === "10000") {
        const token = loginJson.data?.token;
        if (token) {
          tokens.push(token);
          console.log(`[${accountName}] 登录成功，开始签到流程...`);

          const checkResp = await $task.fetch({
            method: "POST",
            url: CHECK_SIGN_URL,
            headers: {
              "Content-Type": "application/json",
              "X-access-token": token,
              "User-Agent": "Mozilla/5.0 (iPhone; CPU iPhone OS 15_8_3 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Mobile/15E148",
              "Channel": "app"
            },
            body: "{}"
          });

          const checkJson = JSON.parse(checkResp.body);
          if (checkJson.success && checkJson.code === "10000") {
            if (checkJson.data === true) {
              console.log(`[${accountName}] 今日已签到。`);
            } else {
              console.log(`[${accountName}] 今日未签到，尝试签到...`);

              const signResp = await $task.fetch({
                method: "POST",
                url: SIGN_IN_URL,
                headers: {
                  "Content-Type": "application/json",
                  "X-access-token": token,
                  "User-Agent": "Mozilla/5.0 (iPhone; CPU iPhone OS 15_8_3 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Mobile/15E148",
                  "Channel": "app"
                },
                body: JSON.stringify({ channelCode: "xj_mall" })
              });

              const signJson = JSON.parse(signResp.body);
              if (signJson.success && signJson.code === "10000") {
                console.log(`[${accountName}] 签到成功，积分: ${signJson.data?.pointValue || "未知"}`);
              } else {
                console.log(`[${accountName}] 签到失败：${signJson.message || "未知错误"}`);
              }
            }
          } else {
            console.log(`[${accountName}] 检查签到状态失败：${checkJson.message || "未知错误"}`);
          }
        } else {
          console.log(`[${accountName}] 登录成功但未获取到 Token。`);
        }
      } else {
        console.log(`[${accountName}] 登录失败：${loginJson.message || "未知错误"}`);
      }

    } catch (err) {
      console.log(`[${accountName}] 请求异常: ${err}`);
    }
  }

  // 存储 Token（可选）
  if (tokens.length > 0) {
    const tokenStr = tokens.join("&");
    $prefs.setValueForKey(tokenStr, "JUNPINHUI_TOKENS");
    console.log(`\n共成功获取 ${tokens.length} 个 token，已存储至 JUNPINHUI_TOKENS`);
  } else {
    console.log(`\n未获取到任何有效 token`);
  }

  $done();
})();