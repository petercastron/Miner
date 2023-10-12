##  UTG项目设备API文档

### 修订历史
| 版本 | 日期       | 说明                   | 作者   |
| ---- | ---------- | ---------------------- | ------ |
| v1.0 | 2022-11-10 | 初稿                   | 杨功勇 |
| v1.1 | 2022-11-10 | 修改认证方式为JWT方案  | 杨功勇 |
| v1.2 | 2022-11-11 | 新增`3.9 创建目录`接口 | 杨功勇 |
| v1.3 | 2023-03-23 | 新增`3.10 清除Cache`接口 | 吴波 |

### 1. 说明
  本文档为UTG文件管理设备端API文档，主要为UTG项目提供空间租用和释放功能, 为前端租用客户提供管理文件功能。

### 2. 接口调用及公共参数
服务端口：8080  
所有POST请求采用json方式放入HTTP包体，GET请求参数在URL中  
错误码说明：
  | 错误码 | 说明       |
  | ------ | ---------- |
  | 0或200 | 成功       |
  | 401    | token过期  |
  | 1001   | 无效的参数 |
  | 1002   | 空间不足   |
  | 1003   | 系统错误   |

### 3. 接口
####  3.1 空间租用（UTG本地调用，不对外开放）
- 接口URL
    ```
    POST /v1/utg/rent
    ```
- 请求参数
  
    | 参数名称           | 必选 | 类型   | 描述                        |
    | ------------------ | ---- | ------ | --------------------------- |
    | rent_voucher       | 是   | string | 租用凭证                    |
    | rent_addr          | 是   | string | 公钥地址                    |
    | rent_volume        | 是   | int    | 租用空间大小, 单位：GB      |
    | rent_time          | 是   | int    | 租用空间时长， 单位：天     |
    | rent_retrieve_time | 是   | int    | 租用空间过期时间， 单位：秒 |

- 返回
  - HTTP Code: 200 正常返回
    ```json
    {
        "result": 错误码
    }
    ```


#### 3.2 空间回收（UTG本地调用，不对外开放）
- 接口URL
    ```
    POST /v1/utg/free
    ```
- 请求参数  

    | 参数名称     | 必选 | 类型   | 描述     |
    | ------------ | ---- | ------ | -------- |
    | rent_voucher | 是   | string | 租用凭证 |
- 返回
  - HTTP Code: 200 正常返回
  ```json
    {
        "result": 错误码
    }
  ```

#### 3.3 获取挑战数
获取挑战数，客户端使用此挑战数加上客户端自定义数据整合后，生成签名，请求登录接口时，需要将挑战数也带上来进行验证。
- 接口URL
    ```
    GET /hello
    ```

- 请求参数
  无

- 返回
  - HTTP Code: 200 正常返回
    ```json
    {
      "challenge":"xxxxxxx"
    }
    ```
    | 参数名称  | 必选 | 类型   | 描述   |
    | --------- | ---- | ------ | ------ |
    | challenge | 是   | string | 挑战数 |

#### 3.4 登录
登录接口，获取jwt token，后续文件相关操作都需要在头部中增加
- 接口URL
    ```
    POST /login
    ```
- 请求参数

    | 参数名称  | 必选 | 类型   | 描述                               |
    | --------- | ---- | ------ | ---------------------------------- |
    | sign      | 是   | string | 签名                               |
    | data      | 是   | string | 签名的原始数据，用户自定义         |
    | pubkey    | 是   | string | 钱包地址，用来验证钱包地址是否合法 |
    | challenge | 是   | string | 挑战数                             |
- 返回
  - HTTP Code: 200 正常返回
    ```json
    {
        "code": "xxxxx",
        "token":"jwt_token",
        "expire": "2022-11-10 17:29:33"
    }
    ```
    | 参数名称 | 必选 | 类型   | 描述           |
    | -------- | ---- | ------ | -------------- |
    | code     | 是   | int    | 返回码，200-ok |
    | token    | 是   | string | jwt token      |
    | expire   | 是   | string | 过期时间       |
 
  
#### 3.5 刷新Token
`jwt token`具有时效，如果token已经过期，需要手动刷新token
- 头部
    ```
    Authorization: Bearer <token>
    ```
- 接口URL
    ```
    POST /refresh_token
    ```
- 返回
  - HTTP Code: 200 正常返回
    ```json
    {
        "code": 200,
        "token":"jwt_token"
    }
    ```
    | 参数名称 | 必选 | 类型   | 描述                             |
    | -------- | ---- | ------ | -------------------------------- |
    | code     | 是   | int    | 返回码，200-ok 401-token expired |
    | token    | 是   | string | jwt token                        |

#### 3.6 列出空间
- 头部
    ```
    Authorization: Bearer <token>
    ```
- 接口URL
    ```
    GET /v1/file/spaces?path=xxxx
    ```
- 请求参数
    | 参数名称 | 必选 | 类型   | 描述     |
    | -------- | ---- | ------ | -------- |
    | path     | 是   | string | 访问路径 |
- 返回
  - HTTP Code: 200 正常返回
    ```json
    {
        "code": 0,
        "spaces":[{
            "size":10240000000,
            "root":"/234234",
            "duration":28880,
            "deadline":1668054110,
            "created_at":1668054110
        }]
    }
    ```
    | 参数名称   | 必选 | 类型   | 描述                             |
    | ---------- | ---- | ------ | -------------------------------- |
    | code       | 是   | int    | 返回码，200-ok 401-token expired |
    | spaces     | 是   | array  | 空间列表                         |
    | ticket     | 是   | string | 租用凭证                         |
    | pubkey     | 是   | string | 钱包地址                         |
    | size       | 是   | int64  | 大小                             |
    | root       | 是   | string | 空间根目录                       |
    | duration   | 是   | int64  | 租用时长                         |
    | deadline   | 是   | int64  | 截至时间                         |
    | created_at | 是   | int64  | 创建时间                         |

#### 3.7 上传
上传采用tusd开源模块，已经提供了js包，示例代码：
```js
 // Create a new tus upload
    var upload = new tus.Upload(file, {
        endpoint: "http://{{host}}/v1/file/upload",
        retryDelays: [0, 3000, 5000, 10000, 20000],
        metadata: {
            filename: file.name,
            targetPath: targetPath,
            pubkey: pubkey
        },
        onError: function(error) {
            console.log("Failed because: " + error)
        },
        onProgress: function(bytesUploaded, bytesTotal) {
            var percentage = (bytesUploaded / bytesTotal * 100).toFixed(2)
            console.log(bytesUploaded, bytesTotal, percentage + "%")
        },
        onSuccess: function() {
            console.log("Download %s from %s", upload.file.name, upload.url)
        }
    })
```
上述代码中，endpoint中的host为设备端地址+端口（8080）。metadata中需要指定targetPath以及pubkey参数，该参数表示上传的目标地址，该目标地址为相对root中的地址

#### 3.8 下载
下载支持续传
- 头部
  ```
    Authorization: Bearer <token>
    ```
- 接口URL
    ```
    GET /v1/file/download?path=/path
    ```
- 请求参数  
    | 参数名称 | 必选 | 类型   | 描述     |
    | -------- | ---- | ------ | -------- |
    | path     | 是   | string | 访问路径 |
- 返回
  - HTTP Code: 200 有效参数
    ```json
    {
      "code": 401,
      "msg": "token expired"
    }
    ```
    | 参数名称 | 必选 | 类型   | 描述                             |
    | -------- | ---- | ------ | -------------------------------- |
    | msg      | 是   | string | 错误消息                         |
    | code     | 是   | int    | 返回码，200-ok 401-token expired |
   
#### 3.9 获取文件列表
删除文件同时支持清空目录，清空目录在路径后面带上"/"，例如清空/a目录，则参数为"/a/"。
- 头部
  ```
  Authorization: Bearer <token>
  ```

- 接口URL
    ```
    GET /v1/file/list
    ```

- 请求参数
    | 参数名称 | 必选 | 类型   | 描述 |
    | -------- | ---- | ------ | ---- |
    | path     | 是   | string | 路径 |

- 返回
  - HTTP Code: 200 无效参数
    ```json
    {
      "files":[{
          "path":"/xxxx/a.jpg",
          "name":"a.jpg",
          "size":10240000000,
          "isdir":false,
          "mod_time":1605050,
      }]
    }
    ```
    | 参数名称 | 必选 | 类型   | 描述                             |
    | -------- | ---- | ------ | -------------------------------- |
    | code     | 是   | int    | 返回码，200-ok 401-token expired |
    | path     | 是   | string | 路径                             |
    | name     | 是   | string | 文件名                           |
    | size     | 是   | int    | 文件大小                         |
    | mod_time | 是   | int    | 最后修改时间                     |
    | isdir    | 是   | bool   | 是否为目录                       |
 
#### 3.10 创建目录
- 头部
  ```
  Authorization: Bearer <token>
  ```
- 接口URL
    ```
    POST /v1/file/mkdir
    ```

- 请求参数
  | 参数名称 | 必选 | 类型   | 描述 |
  | -------- | ---- | ------ | ---- |
  | path     | 是   | string | 路径 |

- 返回
  - HTTP Code: 200 正常
    ```json
    {
      "code": 401,
      "msg": "token expired"
    }
    ```
    | 参数名称 | 必选 | 类型   | 描述                             |
    | -------- | ---- | ------ | -------------------------------- |
    | code     | 是   | int    | 返回码，200-ok 401-token expired |
    | msg      | 是   | string | 错误消息                         |

#### 3.11  删除文件或目录
删除文件同时支持清空目录，清空目录在路径后面带上"/"，例如清空/a目录，则参数为"/a/"。
- 头部
  ```
  Authorization: Bearer <token>
  ```
- 接口URL
    ```
    POST /v1/file/delete
    ```

- 请求参数
    | 参数名称 | 必选 | 类型  | 描述         |
    | -------- | ---- | ----- | ------------ |
    | paths    | 是   | array | 删除路径列表 |

- 返回
  - HTTP Code: 200 正常
    ```json
    {
        "code": 401,
        "msg": "token expired"
    }
    ```
    | 参数名称 | 必选 | 类型   | 描述                             |
    | -------- | ---- | ------ | -------------------------------- |
    | code     | 是   | int    | 返回码，200-ok 401-token expired |
    | msg      | 是   | string | 错误消息                         |
  

#### 3.12 清除Cache
清除因为tusd的临时文件，必须在没有upload的进程下进行
- 头部
  ```
  Authorization: Bearer <token>
  ```
- 接口URL
    ```
    POST /v1/file/clearCache
    ```

- 请求参数
    | 参数名称 | 必选 | 类型  | 描述         |
    | -------- | ---- | ----- | ------------ |
    | pubkey | 是   | array | 公钥 |
	| rent_voucher | 是 | 租用凭证 |

- 返回
  - HTTP Code: 200 正常
    ```json
    {
        "resule": 200,
    }
      


