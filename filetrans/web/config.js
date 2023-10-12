
window.$config = {
  testAddr: "e98ef7645b716746e5505bf901ff7241d19f4ee0",//"3efbaa018f81f9d0ef8b3a3d0d0b767d3482b66b",//
  windowTitle: 'comm.deviceManagement',
  netUrl:"https://rpc.ultronglow.io", //链RPC地址
  netId: "0xbc", //链ID 十六进制 
  baseUrl: "https://super.ultronscan.io",//后台接口地址
  rpcBaseUrl:"https://super.ultronscan.io",//后台RPC转接地址(同baseUrl)
  ajaxRe: true,
  ERROR_TYPES: {
    4: "comm.versionMismatch",
    5: "comm.protocolError",
    7: "comm.invalidName",
    8: "comm.invalidPassword",
    23: "comm.loading",
    24: "comm.statusConformance",
    35: "comm.tooAttempts",
    46: "comm.loginExpired",
    1001: "comm.parameterError",
  },

  DEVUCE_TYPES: {
    1: "comm.WaitingEncapsulation",
    2: "comm.capacityEncapsulation",
    3: "comm.capacityCompleted",
    4: "comm.WaitingEncapsulation",
    5: "comm.pledged",
    6: "comm.unpacking",
    7: "comm.unpacking",

  },
  SOCKET_PATH: {
    login: "wss://*/utglogin",
    devicectl: "wss://*/utgdevicectl",
  },
  avgBlockTime: 4

  /* 1:"容量待封装",
       2: "正在进行容量封装",
       3:"UTGFS容量封装完成",
       4:"正在进行出租容量封装",
       5:"正常服务（可以提供存储租用服务）",
       6:"正在回收出租容量",
       7:"正在解除容量封装"*/
}


