## 一. 目录结构

```
--dist      (发布文件目录)
  --utg     (实际的发布目录)
--docs      (文档目录)
--src       (go源代码)
--web       (前端源代码)
--readme.md (本问题件)
```

## 二. 编译条件以及过程

- 编译条件
  - 需要golang 1.9以上版本
  - 需要make工具
  - 前端web需要npm 8以上版本
- 编译过程
  - cd src
  - make
- 编译结果
  - 编译成功会在dist目录产生utg.tar.gz 文件 
