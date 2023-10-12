<template>
  <div class="app-container">
    <div class="subject">
 <!-- 右边部分 -->
     <div class="subjectLeft">
      <div class="titleName">U T G</div>
       <div style="width: 100%;margin-top: 30px; cursor: pointer;" @click="spaceBtn(item)" v-for="(item,index) in  spaceName" :key="index">
         <div class="spaceName">
           <div style="width: 50px;margin-left: 10px;">
             <img class="spaceImg" @click="nextBtn(item)" src="../../assets/images/kongjian.png" alt="">
            </div>
           <div style="width: 110px;margin-right: 10px;">
             <div class="spaceText">{{item.topName}}</div>
             <div style="width:100%;margin:8px 0;">
               <el-progress :percentage="item.jindu" :format="format" :show-text=false></el-progress>
             </div>
             <div class="spaceSize">{{item.used | parseFileSize}}/{{item.size | parseFileSize }}</div>
           </div>
         </div>
       </div>
     </div>
       <!-- 右边部分 -->
      <div style="width: 100%;height: 100vh;">
        <div class="fliel-class fl-sbat" style="" v-if="spaceName.length>0">
            <div class="textName">{{this.topName}}</div>
            <div class="fl-sbat timeClass">
              <div>
                  <span class="rentalTime">Rental time</span>
                  <span class="rentalTime1">{{spacesData.duration}} Day</span>
              </div>
              <div>
                  <span class="rentalTime">Expiration time</span>
                  <span class="rentalTime1">{{spacesData.deadline | formatCreateTime}}</span>
              </div>
            </div>
        </div>

      <div class="fliel-class" style="display: flex;justify-content: space-between;align-items: center;">
        <div class="fl-sbat" v-if="spaceName.length>0">
          <div class="downloadBtn"  style="margin-right: 14px;" @click="handleSubmit">
          <img src="../../assets/images/upimg.png" alt="" >
          <input type="file" 
          @change="changefolie"
          name="avatar"
          style="display: none;"
          ref="avatarInput"
          >
          <div >Upload</div>
        </div>
        <div class="downloadBtn" @click="donglod">
          <img src="../../assets/images/upimg.png" alt="">
          <div>Download</div>
        </div>
        </div>
        <div v-else class="fl-sbat">
          <div class="downloadBtn"  style="margin-right: 14px;background-color: #999;cursor: not-allowed;" >
          <img src="../../assets/images/upimg.png" alt="" >
          <input type="file" 
          @change="changefolie"
          name="avatar"
          style="display: none;"
          ref="avatarInput"
          >
          <div >Upload</div>
        </div>
        <div class="downloadBtn"  style="background-color: #999;cursor: not-allowed;">
          <img src="../../assets/images/upimg.png" alt="">
          <div>Download</div>
        </div>
        </div>
        <div class="fl-sbat">
          <div  style="margin-right: 5px;">
          <img @click="modelBtn(modelShow)" class="switchImg" src="../../assets/images/max1.png" alt="" v-if="!modelShow">
          <img class="switchImg" src="../../assets/images/max.png" alt="" v-else>
        </div>
         <div  @click="modelBtn(modelShow)">
          <img  class="switchImg" src="../../assets/images/man1.png" alt="" v-if="modelShow">
          <img  class="switchImg" src="../../assets/images/man.png" alt="" v-else>
        </div>
        </div>
      </div>
 
    <!-- <div class="fliel-class" style="display: flex;">
      <div>
        <i class="el-icon-caret-left" style="margin-right: 10px;color:#999;"></i>
        <i class="el-icon-caret-left" style="margin-right: 10px;color:#025AFa;"></i>
      </div>
      <div>
        <i class="el-icon-caret-righ              t" style="color:#999;"></i>
        <i class="el-icon-caret-right" style="color:#025AFa;"></i>
      </div>
    </div> -->
      <div v-if="modelShow" style="width:100%;margin-top: 40px;">
        <div class="fliel-class" style="display:flex;justify-content: left;align-items: center;margin: auto;">
          <input style="margin-top: 2px;margin-right: 10px;" type="checkbox" :value="inputShow" v-model="inputShow"
            @change="allSshow(inputShow)">
          <span class="fl-text">Select All</span>
        </div>
        <div class="fliel-class fl list-class">
          <div class="list-cn" v-for="(item, index) in fileData" :key="index">
            <!-- <img v-if="!item.isdir" class="list-img1" :src="item.imgle" alt="">
            <img v-else class="list-img1" src="../../assets/images/fleImg.png" alt=""> -->
            
            <img v-if="isdir" class="list-img1"   src="../../assets/images/fle-img.png" alt="">
            <img v-else-if="item.type==6" class="list-img1"  src="../../assets/images/qita.png" alt="">
              <img v-else-if="item.type==1" class="list-img1"  src="../../assets/images/fleImg.png" alt="">
              <img v-else-if="item.type==4" class="list-img1"  src="../../assets/images/wode.png" alt="">
              <img v-else-if="item.type==5" class="list-img1"  src="../../assets/images/zip.png" alt="">
              <img v-else-if="item.type==3" class="list-img1"  src="../../assets/images/exe.png" alt="">
              <img v-else-if="item.type==2" class="list-img1"   src="../../assets/images/mp3.png" alt="">
              <img v-else-if="item.type==7" class="list-img1"   src="../../assets/images/mp4.png" alt="">
            <div style="display:flex;align-items: center;">
              <input v-if="!item.isdir" style="margin-top: 8px;margin-right: 5px;" type="checkbox" :value="item.is_tag"
              v-model="item.is_tag" :id="index" @change="change(item)">
            <span v-else style="width: 5px;height: 5px;color: #f4f4f4;">11</span>
            <span v-if="item.isdir" class="list-text" @click="nextBtn(item)">{{ item.path }}</span>
            <span v-else class="list-text" style="color: #999;">{{ item.name }}</span>
            </div>
           
          </div>
        </div>
      </div>
      <div v-else class="fliel-class" style="">
        <el-row style="border-bottom: 1px solid #ededed;padding: 10px 0px;border-top: 1px solid #ededed;">
          <el-col :span="8" style="border-right: 1px solid #f4f4f4;">
            <div class="" style="display:flex;justify-content: left;align-items: center;">
              <input style="margin-top: 2px;margin-right: 10px;" type="checkbox" :value="inputShow" v-model="inputShow"
                @change="allSshow(inputShow)">
              <span class="fltop-text"> File name</span>
            </div>
          </el-col>
          <el-col :span="6" style="border-right: 1px solid #f4f4f4;">
            <div class="fltop-text1" style="text-align: center;">
              Modification time
            </div>
          </el-col>
          <!-- <el-col :span="4" style="border-right: 1px solid #f4f4f4;">
            <div class="fltop-text1" style="text-align:center ;">
              所属文件夹
            </div>
          </el-col> -->
          <el-col :span="5" style="border-right: 1px solid #f4f4f4;">
            <div class="fltop-text1" style="text-align:center ;">
              Type
            </div>
          </el-col>
          <el-col :span="5" style="border-right: 1px solid #f4f4f4;">
            <div class="fltop-text1" style="text-align:center ;">
              Size
            </div>
          </el-col>
        </el-row>
        <div @mouseenter="onMouseOver(item)"  @mouseleave="onMouseOut(item)" v-for="(item, index) in fileData" :key="index">
        <el-row  style="border-bottom: 1px solid #ededed;padding: 10px 0px;" >
          <el-col :span="8">
            <div v-if="!item.isdir" class="" style="display:flex;justify-content: left;cursor: pointer;align-items: center;">
              <input v-if="!item.isdir" style="margin-right: 10px;" type="checkbox" :value="item.is_tag"
                v-model="item.is_tag" :id="index" @change="change(item)">
                <img v-if="isdir" class="list-img1"   src="../../assets/images/fle-img.png" alt="">
            <img v-else-if="item.type==6" class="list-img1"  src="../../assets/images/qita.png" alt="">
              <img v-else-if="item.type==1" class="text-img1"  src="../../assets/images/fleImg.png" alt="">
              <img v-else-if="item.type==4" class="text-img1"  src="../../assets/images/wode.png" alt="">
              <img v-else-if="item.type==5" class="text-img1"  src="../../assets/images/zip.png" alt="">
              <img v-else-if="item.type==3" class="text-img1"  src="../../assets/images/exe.png" alt="">
              <img v-else-if="item.type==2" class="text-img1"   src="../../assets/images/mp3.png" alt="">
              <img v-else-if="item.type==7" class="text-img1"   src="../../assets/images/mp4.png" alt="">
              <div class="fliel-span">{{ item.name }}</div>
            </div>
            <div v-else class="" style="display:flex;justify-content: left;cursor: pointer;align-items: center;">
              <img class="text-img" @click="nextBtn(item)" src="../../assets/images/fle-img.png" alt="">
              <span @click="nextBtn(item)" class="fliel-span">{{ item.name }}</span>
            </div>
          </el-col>

          <el-col  :span="6" >
            <div class="" v-if="!item.onMouseShow" style="text-align: center;margin-top: 11px;font-size: 12px;;height: 18px;">
               {{ item.mod_time | formatCreateTime }}
            </div>
            <div class="fl" v-else style="margin-top: 11px;height: 18px;">
              <img class="switchImg" @click="onDownloadFiles(item)" src="../../assets/images/daonglod.png" alt="" style="margin-right: 10px;">
              <img class="switchImg" @click="deleteBtn(item)" src="../../assets/images/delete.png" alt="">
            </div>
          </el-col>
          <el-col :span="5">
            <div class="" v-if="!item.onMouseShow" style="text-align:center ;margin-top: 11px;font-size: 12px;;height: 18px;">
             {{ item.name | fileTypeName }}
             
            </div>
          </el-col>
          <el-col :span="5">
            <div class="" v-if="!item.onMouseShow" style="text-align:center ;margin-top: 11px;font-size: 12px;;height: 18px;">
              {{ item.size | parseFileSize }}
            </div>
          </el-col>
        </el-row>
      </div>
      </div>
    </div>
    </div>
  </div>
</template>
<script>
import { formatDate } from "@/utils/date";
import { getToken, setToken, removeToken } from '@/utils/auth'
import { login,spaces,fileList,hello,download,deletes} from "@/api/home";
import StreamSaver from 'streamsaver';
import { ethers } from 'ethers';
import * as tus from "tus-js-client";
var css =window.location.host
const urls="http://"+css.split(':')[0]+ ":" + window.location.port
// const urls="http://192.168.4.53:8080/"
export default {
  name: "home",
  data() {
    return {
      files: [
        {
          name:"测试",
          falseType:1, 
        },
        {
          name:"测试2",
          falseType:1, 
        }
      ],
      modelShow: false,
      modelText:1,
      inputShow:false,
      format(percentage) {
        return percentage === 100 ? '' : `${percentage}%`;
      },
      spaceName:[],
      spacesData:{},
      inputtype:'1223',
      fileData:[],
      fileing:[],
      topName:''
    };
  },
  created() {
    // removeToken()
    if(getToken()==undefined){
      this.connect();
    }else{
      this.spaces()
     
    }
    
  },
  filters: {
    formatCreateTime(time) {
      let date = new Date(time * 1000);
      // let date = new Date(time);
      // console.log(date,'date')
      let dates = date.getDate()
      // console.log(dates,'getDate()')
      // return formatDate(date, "yyyy-MM-dd hh:mm:ss");
      return formatDate(date, "yyyy-MM-dd");
    },
    parseFileSize(s) {
      if (s < 1024) {
        return s + "g"
      } else if (s < 1024 * 1024) {
        return (s / 1024).toFixed(1) + "K"
      } else if (s < 1024 * 1024 * 1024) {
        return (s / 1024 / 1024).toFixed(1) + "M"
      }else if(s< 1024 * 1024 * 1024 * 1204){
        return (s / 1024 / 1024 / 1024).toFixed(1) + "G"
      }else{
        return (s / 1024 / 1024 / 1024 / 1024).toFixed(1) + "T"
      }
      
    },

  fileTypeName(name){ 
let index= name.lastIndexOf(".");
// console.log(index,'index')
//获取后缀
let varext = name.substring(index+1);
if(index==-1){
  return "catalogue"
}else{
  return varext
}
  },
},
  methods: {
    modelBtn(modelShow){
      this.modelShow=!modelShow

    },
    //检测钱包
    testing(){
      // console.log(window.ethereum,'window.ethereum');
      if (typeof window.ethereum === 'undefined') {
	        alert("Please install MetaMask.");
      } else {
	// app init code
}
    },
    //连接钱包
    connect(){
     this.testing();
     ethereum.request({ method: 'eth_requestAccounts' }).then(resbas=>{
      // console.log(resbas,'resbas')
        this.handleAccountsChanged(resbas)
     }).catch((err) => {
            if (err.code === 4001) {
                alert('Please connect to MetaMask.');
            } else {
                console.error(err);
            }
        });
    },
    async handleAccountsChanged(accounts){ 
      const provider = new ethers.providers.Web3Provider(window.ethereum);
      await provider.send("eth_requestAccounts", []) 
       const signer = provider.getSigner(); //获取签名者信息
      //  console.log(signer,'signer')
      //钱包地址
       let userPk =null;
       userPk= await signer.getAddress();
       //生成随机数
       let data=Math.random()*10
          data=data.toString()
      let challenge=null
        hello().then(response=>{
          data+response.data.challenge
          challenge=response.data.challenge
            // console.log(response,'后台给的随机数') 
        })
       let signature = await signer.signMessage(data) 
       console.log(signature,'signature')
      let account=null
      if (accounts.length === 0) {
        alert('Please connect to MetaMask.');
      	} else if (accounts[0] !== account) {
	       	account = accounts[0];
	    }
      //验证登录
      let datas={
          data:data,
          sign:this.getCaption(signature),
          pubkey:userPk,
          challenge:challenge
      } 
      removeToken()
      // console.log(getToken(),'getToken()')
        await login(datas).then(response=>{
        console.log(response,'登录成功')
        setToken( response.data.token);
         this.spaces();
         this.fileList()
      })
    },
    getCaption (obj) {
    const res = obj.substring(2, obj.length)
    return res
    },
    spaces(){
      spaces().then(response=>{
        //  console.log('列出空间')
        let list=[];
        response.data.spaces.map(item=>{
          let datalist={
            created_at:item.created_at,
            deadline:item.deadline,
            duration: item.duration,
            pubkey:item.pubkey,
            root:item.root,
            topName:item.root.substring(1),
            size:item.size,
            ticket:item.ticket,
            used:item.used,
            jindu:parseFloat(item.used) / parseFloat(item.size) * 1000 / 10,
          }
          list.push(datalist)
        })
        this.spaceName=list
        this.spacesData=list[0];
        // console.log(this.spacesData.root.substring(1),'this.spacesData.root.substring(1)')
        this.topName= this.spacesData.root.substring(1); 

        this.fileList()
      })
    },
    // 鼠标移入
    onMouseOver(item){
      setTimeout(function(){
        item.onMouseShow =!item.onMouseShow
      },400);
      
      // this.fileData.map((items => {
      //   if (items.id!==item.id) {
      //     item.onMouseShow=true
      //   }
      // }))
      // console.log( item.onMouseShow,'鼠标进去了');
    },
    // 鼠标移出
    onMouseOut(item){
      setTimeout(function(){
        item.onMouseShow =!item.onMouseShow
      },400);
     
      // this.fileData.map((items => {
      //   if (items.id!==item.id) {
      //     item.onMouseShow=false
      //   }
      // }))
      // console.log(item.onMouseShow,'鼠标出去了');
    },
   //文件上传
   handleSubmit(){
     this.$refs.avatarInput.click()
   },
   changefolie(e){
     let root=this.spacesData.root.substring(1, this.spacesData.root.length)
    var file = e.target.files[0]
    var upload = new tus.Upload(file, {
        endpoint: urls+'/v1/file/upload'+'?space='+root,
        // "http://192.168.4.53:8080/v1/file/upload?space="+root,
        // retryDelays: [0, 3000, 5000, 10000, 20000],
        retryDelays: [],
        metadata: {
            filename: file.name,
            targetPath: this.spacesData.root,
            pubkey: this.spacesData.pubkey
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
    upload.start()
    this.fileList()
   },
   //获取文件列表
   fileList(){
     let data={
       path:this.spacesData.root
     }
    fileList(data).then(response=>{
  let list=[];
        response.data.files.map(item=>{
          let datalist={
            isdir:item.isdir,
            name:item.name,
            path:item.path,
            size:item.size,
            type:this.fileType(item.name),
            mod_time:item.mod_time,
            is_tag:item.is_tag,
            onMouseShow:false
          }
          list.push(datalist)
        })
        this.fileData=list
    //  console.log(this.fileData,'获取的数据列表')
    })
   },
  //  判断文件类型
  fileType(name){ 
let index= name.lastIndexOf(".");
//获取后缀
let varext = name.substring(index+1);
 if(varext=='jpg'||varext=='png'||varext=='jpeg'||varext=='bmp'||varext=='gif'||varext=='webp'||varext=='psd'||varext=='svg'||varext=='tiff'){
   return 1
 }else if(varext=='mp3'){
  return 2
 }else if(varext=='exe'){
  return 3
 }else if(varext=='word'){
  return 4
 }else if(varext=='zip'){
  return 5
 }else if(varext=='mp4'){
  return 7
 }else{
  return 6
 }
  },
  change(item) {
      item.is_tag != item.is_tag
      let arr = [];
      this.fileData.map((item => {
        if (!item.is_tag) {
          this.inputShow = false
        }
        if (item.is_tag === true) {
          arr.push(item)
        }
      }))
      this.fileing = arr
      console.log(this.fileing,'this.fileing')
      if(this.fileing.length==this.fileData.length){
          this.inputShow=true
      }else{
        this.inputShow=false
      }
      // var size = 0
      // this.fileing.forEach(file => {
      //   size += file.size
      // });
      // this.size = size
    },
    //下载文件
    onDownloadFiles(item){
      let donwloadurl= urls+'/v1/file/download'  
      // "http://192.168.4.53:8080/v1/file/download"
      this.downloadFile(donwloadurl + "?path=" + item.path, item.name)
    },
    donglod(){
      let donwloadurl=urls+'/v1/file/download' 
      // "http://192.168.4.53:8080/v1/file/download"
      this.fileing.forEach(file => {
        this.downloadFile(donwloadurl + "?path=" + file.path, file.name)
      });
    },
    downloadFile(url, name) {
      fetch(url,{ headers:{
        'Authorization':'Bearer'+' ' + getToken()
        }}).then(res => {
        if (res.status != 200) {
          Message({
            message: "文件下载出错"+res.status,
            type: 'error',
            duration: 3 * 1000
          })
          return
        }
        const fileStream = StreamSaver.createWriteStream(name, {
          size: res.headers.get("content-length")
        })
        console.log(res,'fileStream')
        res.body.pipeTo(fileStream).then(() => {
          // console.log("download complete")
        });
      })
    },
    allSshow(inputShow) {
      console.log('11111111')
      inputShow != inputShow;
      let arr = []
      this.fileData.map((item => {
        if (!item.is_dir) {
          item.is_tag = inputShow;
          arr.push(item);
        }
      }))
      this.fileing = arr;
      var size = 0
      this.fileing.forEach(file => {
        size += file.size
      });
      this.size = size
      if (!inputShow) {
        this.size = 0
      }
    },
    //删除文件
    deleteBtn(item){
      let data={
        paths:[]
      }
      data.paths.push(item.path)
      
      deletes(data).then(response=>{
        console.log(response,'删除成功')
        this.fileList()
      })
    },
    spaceBtn(item){
      this.spacesData=item;
      this.topName= this.spacesData.root.substring(1); 
      this.fileList()
    }
  }
};
</script>

<style scoped>
.subject{
  width: 100%;
  display: flex;
  align-items: center;
}
.subjectLeft{
  width: 228px;
  height: 100vh;
  background-color: #2b2b35;
}
.titleName{
width: 100%;
text-align: center;
color: #fff;
font-weight: 900;
font-size: 32px;
margin-top: 26px;
}
.spaceName{
  width: 160px;
  height: 78px;
  border: 2px solid #4B8BFF;
  margin: auto;
  background: #fff;
  display: flex;
  justify-content: center;
  align-items: center;
}
.spaceImg{
  width: 28px;
  height: 28px;
}
.spaceText{
  font-size: 14px;
font-family: Microsoft YaHei;
font-weight: 400;
color: #323232;
margin-bottom:8px; 
overflow: hidden;
  text-overflow: ellipsis;
  display: -webkit-box;
  -webkit-box-orient: vertical;
  -webkit-line-clamp: 1;

}
.spaceSize{
  font-size: 11px;
font-family: Microsoft YaHei;
font-weight: 400;
color: #A5A4A4;
}
.fl-sbat{
  display: flex;
  justify-content: space-between;
  align-items: center;
}
.textName{
  font-size: 18px;
font-family: Microsoft YaHei;
font-weight: 600;
color: #282828;
}
.timeClass{
  width: 341px;
  height: 26px;
  border-radius: 26px;
  background: #F4f4f4;
  padding: 5px 8px;
}
.rentalTime{
  font-size: 12px;
font-family: Microsoft YaHei;
font-weight: 500;
color: #808080;
}
.rentalTime1{
  font-size: 14px;
font-family: Microsoft YaHei;
font-weight: 500;
color: #025AFA;
}
.switchImg{
width: 16px;
height: 16px;
}
.fltop-text{
  font-size: 13px;
  margin-top: 4px;
}
.fltop-text1{
  font-size: 13px;
  margin-top: 4px;
}
.fl{
  display: flex;
}





.conten-top {
  width: 100%;
  height: 60px;
  margin-top: 20px;
  /* background-color: gray; */
}

.downloadBtn {
  width: 100px;
height: 26px;
background: #025AFA;
border-radius: 13px;
  display: flex;
  justify-content: center;
  align-items: center;
  line-height: 40px;
  font-size: 14px;
  color: #fff;
  cursor: pointer;
}

.promptlyBtn {
  width: 120px;
  height: 40px;
  background: #11af6d;
  line-height: 40px;
  font-size: 14px;
  color: #fff;
  cursor: pointer;
  border-radius: 5px;
  text-align: center;
}

.downloadBtn img {
  width: 14px;
  height: 14px;
  margin-right: 8px;
}

.exitBtn {
  width: 150px;
  height: 40px;
  background: #edeef3;
  color: #2d4379;
  text-align: center;
  line-height: 40px;
  font-size: 14px;
  border-radius: 5px;
  margin-left: 5px;
  cursor: pointer;
}

.fallback {
  width: 60%;
  margin: auto;
  margin-top: 40px;
  height: 27px;
  cursor: pointer;
}

.fallbacks {
  width: 80px;
  background-color: #11af6d;
  font-size: 14px;
  padding: 5px 15px;
  text-align: center;
  border-radius: 3px;
  color: #Fff;
}

.exitBtn:hover {
  color: #11af6d;
}

.contenimg {
  width: 220px;
  height: 33px;
  margin-top: 15px;
  cursor: pointer;
}

.top-flex {
  width: 60%;
  height: 100%;
  margin: auto;
  /* border: 1px solid red; */
  display: flex;
  justify-content: space-between;
}

.prText>>>.el-progress__text {
  color: #fff;
}

.title {
  display: flex;
  align-items: center;
  justify-content: center;
  /* margin-top: 20px; */
}

.title-text {
  margin-left: 20px;
  font-size: 28px;
  color: #fff;
}

.conten-en {
  width: 100%;
  background: #dbba92 url("../../assets/images/tag-light.png")no-repeat;
  background-size: cover;
  height: 250px;
  display: flex;
  justify-content: center;
  align-items: center;
  margin-top: 20px;
}

.conten-cs {
  display: flex;
  justify-content: space-around;
  width: 60%;
  margin: auto;
}

.conten-tow {
  display: flex;
  justify-content: space-between;
}

.ml {
  margin-left: 100px;
}

.rimg {
  width: 28px;
  height: 28px;
  margin-top: 20px;
}

.name-text {
  color: #fff;
  font-weight: 700;
  font-family: Jost, sans-serif;
  font-size: 21px;
  margin-top: 20px;
}

.mask {
  position: fixed;
  top: 0px;
  left: 0px;
  width: 100%;
  height: 100vh;
  background: rgba(27, 32, 24, 0.6);
}

.mask-div {
  width: 90%;
  height: 100%;
  background: #2b2b35;
  animation: mymove 1s;
  animation-fill-mode: forwards;
}

@keyframes mymove {
  from {
    /* width: 1%; */
    /* margin-left: -90%; */
    transform: translateX(-90%);
  }

  to {
    /* width: 85%; */
    /* margin-left: 0%; */
    transform: translateX(0%);
  }
}

.call {
  float: right;
  margin: 10px 7px;
  width: 30px;
  height: 30px;
}

.text-p {
  width: 100%;
  text-align: center;
  color: #fff;
  font-size: 13px;
  margin-top: 70px;
}

.list-class {
  /* width: 60%; */
 
  flex-wrap: wrap;
  margin-top: 30px;
}

.fliel-class {
  width: 90%;
  margin: auto;
  margin-top: 20px;
}

.fliel-span {
  /* margin-top: 11px; */
  font-size: 12px;
  margin-left: 5px;
  overflow: hidden;
  text-overflow: ellipsis;
  display: -webkit-box;
  -webkit-box-orient: vertical;
  -webkit-line-clamp: 1;
}
.text-img{
  width: 30px;
  height: 30px;
}
.text-img1{
  width: 20px;
  height: 20px;
}
.list-cn {
  width: 64px;
  margin-right: 10px;
  /* border-radius: 10px; */
  /* border: 1px solid red; */
  /* box-shadow: 0 10px 30px #c6d1d0; */
  /* padding-bottom: 20px;
  margin-bottom: 20px; */
}
.list-img {
  width: 20px;
  height: 20px;
  /* border-radius: 10px 10px 0px 0px; */
  object-fit: cover;
}
.list-img1{
  width: 64px;
  height: 64px;
}
.list-text {
  width: 88%;
  margin: auto;
  color: #1a274e;
  font-weight: 700;
  font-family: Jost, sans-serif;
  font-size: 12px;
  margin-top: 10px;
  /* text-decoration: underline; */
  cursor: pointer;
  overflow: hidden;
  text-overflow: ellipsis;
  display: -webkit-box;
  -webkit-box-orient: vertical;
  -webkit-line-clamp: 1;
}

.list-text1 {
  width: 90%;
  margin: auto;
  color: #2d4379;
  font-size: 15px;
  font-family: Muli, sans-serif;
  font-weight: 400;
  margin-top: 15px;
}

@-webkit-keyframes mymove {}

@media screen and (max-width:415px) {
  .title {
    display: none;
  }

  .conten-cs {
    display: block;
  }

  .promptlyBtn {
    width: 110%;
    margin: auto;
    margin-top: 30px;
  }

  .ml {
    margin-left: 0px;
  }

  .conten-en {
    height: 350px;
  }

  .top-flex {
    width: 90%;
  }

  .fallback {
    width: 90%;
  }

  .name-text {
    font-size: 13px;
  }

  .list-class {
    width: 90%;
    display: block;
  }

  .fliel-class {
    width: 90%;
  }


  .list-cn {
    width: 100%;
  }

  .list-img {
    width: 100%;
    height: 20px;
  }

}

@media only screen and (min-width:414px) and (max-width:1050px) {
  .top-flex {
    flex-wrap: wrap;
  }

  .conten-top {
    height: 120px;
    /* border: 1px solid red; */
  }

  .conten-cs {
    display: block;
  }

  .promptlyBtn {
    width: 90%;
    margin: auto;
    margin-top: 30px;
  }

  .rimg {
    display: none;
  }

  .list-cn {
    width: 80%;
    margin: auto;
  }

  .list-img {
    width: 100%;
    height: 20px;
  }
}

@media only screen and (min-width:1050px) and (max-width:1300px) {
  .list-cn {
    width: 350px;
  }
}

@media only screen and (min-width:1050px) {
  .rimg {
    display: none;
  }

}

@media only screen and (min-width:1300px) {
  .list-img {
    height: 20px;
  }
}

.name-top-img {
  width: 80px;
  height: 80px;
  border-radius: 40px;
}
</style>
