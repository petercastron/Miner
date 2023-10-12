package main

import (
	"crypto/sha1"
	"errors"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"strconv"
	"strings"
	"syscall"

	"github.com/UltronGlow/UltronGlow-Origin/common"
	"github.com/UltronGlow/UltronGlow-Origin/crypto"
	"github.com/gin-gonic/gin"
	"golang.org/x/sys/unix"
)

const (
	KB              = 1024
	MB              = 1024 * KB
	GB              = 1024 * MB
	TB              = 1024 * GB
	MinimumFreeSize = 5 * GB
)

func SighHash(data []byte) []byte {
	msg := fmt.Sprintf("\x19Ethereum Signed Message:\n%d%s", len(data), data)
	return crypto.Keccak256([]byte(msg))
}

func Verify(msg, sig, addr string) bool {
	sigByte := common.Hex2Bytes(sig)
	if len(sigByte) != crypto.SignatureLength {
		fmt.Printf("wrong size for signature: got %d, want %d", len(sig), crypto.SignatureLength)
		return false
	}
	sigByte[64] = sigByte[64] - 27
	recoveredPub, err := crypto.Ecrecover(common.Hex2Bytes(msg), sigByte)
	if err != nil {
		fmt.Println("Ecrecover failed, err:", err)
		return false
	}

	pubKey, err := crypto.UnmarshalPubkey(recoveredPub)
	if err != nil {
		fmt.Println("UnmarshalPubkey failed, err:", err)
		return false
	}

	return crypto.PubkeyToAddress(*pubKey) == common.HexToAddress(addr)
}

func GetFreeSize(dir string) uint64 {
	var stat unix.Statfs_t
	unix.Statfs(dir, &stat)
	return stat.Bavail * uint64(stat.Bsize)
}

func getFileName(key string) string {
	h := sha1.New()
	h.Write([]byte(key))
	bs := h.Sum(nil)
	return fmt.Sprintf("%x", bs)
}

func startTusd(path string, addr string, pidFile string) error {
	os.MkdirAll(path, 0755)
	cmd := exec.Command("tusd",
		"-upload-dir",
		path,
		"-unix-sock",
		addr,
		"--hooks-enabled-events",
		//"pre-create,post-create,post-receive,post-terminate,post-finish",
		"post-finish",
		"--hooks-http",
		fmt.Sprintf("http://localhost:%s/tusd", "8081"))
	err := cmd.Start()
	if err != nil {
		log.Println("err:", "start tusd error:", err)
		return err
	}
	pid := cmd.Process.Pid
	ioutil.WriteFile(pidFile, []byte(fmt.Sprintf("%d", pid)), 0664)
	go func() {
		err = cmd.Wait()
		fmt.Printf("Command finished with error:%v", err)
	}()
	return nil
}

func stopTusd(pidFile string) error {
	data, err := ioutil.ReadFile(pidFile)
	if err != nil {
		log.Println("err:", err)
		return err
	}
	pid, _ := strconv.Atoi(string(data))
	process, err := os.FindProcess(pid)
	if err != nil {
		log.Println("err:", err)
		return err
	}
	err = process.Signal(syscall.Signal(syscall.SIGQUIT))
	if err != nil {
		log.Println("err:", err)
		return err
	}
	return nil
}

func rent(path string, size uint64) (mountpoint string, err error) {
	defer func() {
		if err != nil {
			os.RemoveAll(path)
		}
	}()
	log.Println("info:", path, size)
	var f *os.File
	f, err = os.OpenFile(path, os.O_CREATE|os.O_WRONLY|os.O_TRUNC, 0666)
	if err != nil {
		log.Println("err:", err)
		return
	}

	err = syscall.Fallocate(int(f.Fd()), 0, 0, int64(size))
	if err != nil {
		if err == syscall.ENOTSUP {
			err = f.Truncate(int64(size)) //对于不支持Fallocate的平台，直接用Truncate创建指定大小的文件
			if err != nil {
				log.Println("err:", "222", err)
				f.Close()
				return
			}
		} else {
			log.Println("err:", "333", err)
			f.Close()
			return
		}
	}
	f.Close()

	cmd := exec.Command("mkfs.ext4", "-E", "nodiscard", path)
	var output []byte
	output, err = cmd.CombinedOutput()
	if err != nil {
		log.Println("err:", string(output))
		return
	}

	filename := filepath.Base(path)
	mountpoint = filepath.Join(app.MountDir, filename)
	err = os.Mkdir(mountpoint, 0755)
	if err != nil {
		log.Println("err:", err, mountpoint)
		return
	}
	// 检查是否有更多的loop,没有试图创建,
	// 全局锁住，同一时间只能一次创建块设备并mount
	app.Lock()
	defer app.Unlock()
	if checkMoreLoopDev() {
		cmd = exec.Command("mount", "-o", "loop", path, mountpoint)
		output, err = cmd.CombinedOutput()
	} else {
		err = errors.New("no more loop devices")
	}
	if err != nil {
		log.Println("err:", string(output))
		return
	}
	return
}

type RentParam struct {
	RentVoucher      string `json:"rent_voucher" binding:"required"`
	RentAddr         string `json:"rent_addr" binding:"required"`
	RentVolume       uint64 `json:"rent_volume" binding:"required"`
	RentTime         uint64 `json:"rent_time" binding:"required"`
	RentRetrieveTime uint64 `json:"rent_retrieve_time" binding:"required"`
}

func Rent(c *gin.Context) {
	var param RentParam
	err := c.ShouldBindJSON(&param)
	if err != nil {
		log.Println("err:", err)
		c.JSON(http.StatusOK, gin.H{"result": StatusParamInvalid})
		return
	}

	log.Println("info:", "rent space", param.RentVoucher)

	rentalInfo := RentalInfo{
		Voucher:  param.RentVoucher,
		PubKey:   strings.ToLower(param.RentAddr[2:]),
		Size:     param.RentVolume * GB,
		Duration: param.RentTime,
		Deadline: param.RentRetrieveTime,
	}

	freeSize := GetFreeSize(app.DataDir)
	log.Println("freeSize", freeSize)
	if freeSize-rentalInfo.Size < MinimumFreeSize {
		log.Println("err:", "free size", freeSize, "request size", rentalInfo.Size)
		c.JSON(http.StatusOK, gin.H{"result": StatusNoSpace})
		return
	}

	filename := getFileName(rentalInfo.Voucher)
	path := filepath.Join(app.DataDir, filename)
	//check file is exist ? 避免超时重入
	s, e := os.Lstat(path)
	if e == nil && !s.IsDir() {
		filename := filepath.Base(path)
		m2 := filepath.Join(app.MountDir, filename)
		//判断mount点，如果是，说明正确了,否则返回系统错误
		cmd := exec.Command("mountpoint", "-q", m2)
		if e2 := cmd.Run(); e2 == nil {
			c.JSON(http.StatusOK, gin.H{"result": 0})
			return
		}
		c.JSON(http.StatusOK, gin.H{"result": StatusSystem})
		return
	}
	mountpoint, err := rent(path, rentalInfo.Size)
	if err != nil {
		log.Println("err:", err)
		c.JSON(http.StatusOK, gin.H{"result": StatusSystem})
		return
	}

	log.Println("mountpoint:", mountpoint)
	uploadDir := filepath.Join(mountpoint, ".uploads")
	os.MkdirAll(uploadDir, 0755)
	unixAddr := filepath.Join("/tmp", filename+".sock")
	pidFile := filepath.Join("/tmp", filename+".pid")
	if err := startTusd(uploadDir, unixAddr, pidFile); err != nil {
		log.Println("err:", err)
		c.JSON(http.StatusOK, gin.H{"result": StatusSystem})
		return
	}
	rentalInfo.Root = filepath.Join("/", filename)
	app.Lock()
	app.DB.Create(&rentalInfo)
	app.Unlock()

	c.JSON(http.StatusOK, gin.H{"result": 0})
}

type FreeParam struct {
	RentVoucher string `json:"rent_voucher" binding:"required"`
}

func Free(c *gin.Context) {
	var param FreeParam
	if err := c.ShouldBindJSON(&param); err != nil {
		c.JSON(http.StatusOK, gin.H{"result": StatusParamInvalid})
		return
	}
	var rentalInfo RentalInfo
	tx := app.DB.First(&rentalInfo, "voucher = ?", param.RentVoucher)
	if tx.Error != nil || tx.RowsAffected <= 0 {
		log.Println("err:", "invalid voucher", rentalInfo.Voucher)
		c.JSON(http.StatusOK, gin.H{"result": StatusParamInvalid})
		return
	}

	tusdPidFile := filepath.Join("/tmp", rentalInfo.Root+".pid")
	stopTusd(tusdPidFile)

	mountpoint := filepath.Join(app.MountDir, rentalInfo.Root)
	cmd := exec.Command("mountpoint", "-q", mountpoint)
	if err := cmd.Run(); err == nil {
		cmd = exec.Command("umount", mountpoint)
		output, err := cmd.CombinedOutput()
		if err != nil {
			log.Println("err:", string(output))
			c.JSON(http.StatusOK, gin.H{"result": StatusSystem})
			return
		}
	}
	os.Remove(filepath.Join(app.DataDir, rentalInfo.Root))

	app.Lock()
	app.DB.Delete(&RentalInfo{}, "voucher=?", rentalInfo.Voucher)
	app.Unlock()
	c.JSON(http.StatusOK, gin.H{"result": 0})
}

func checkMoreLoopDev() bool {
	loopid := getNextLoopName()
	if loopid == "" {
		return false
	}
	loopName := fmt.Sprintf("/dev/loop%s", loopid)
	_, e := os.Lstat(loopName)
	if e == nil {
		return true
	}
	if os.IsNotExist(e) {
		cmd := exec.Command("mknod", "-m", "0666", loopName, "b", "7", fmt.Sprintf("%s", loopid))
		e := cmd.Run()
		if e == nil {
			return true
		}
		log.Println("err:", loopName, e)

	}
	return false
}
func getNextLoopName() string {
	r := regexp.MustCompile(`/dev/loop(\d+)`)
	cmd := exec.Command("losetup", "-f")
	context, e := cmd.CombinedOutput()
	if e == nil {
		ret := r.FindStringSubmatch(string(context))
		if len(ret) > 1 {
			return ret[1]
		}
	}
	return ""
}

func checkIsMountPoint(path string) bool {
	cmd := exec.Command("mountpoint", "-q", path)
	if e2 := cmd.Run(); e2 == nil {
		return true
	}
	return false
}
