package main

import (
	"errors"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"sort"
	"strings"
	"syscall"

	"github.com/gin-gonic/gin"
)

var errInvalidPubkey = errors.New("invalid pubkey")
var errInvalidPath = errors.New("invalid path")

type FileInfo struct {
	Path    string `json:"path"`
	Name    string `json:"name"`
	Size    int64  `json:"size"`
	ModTime int64  `json:"mod_time"`
	IsDir   bool   `json:"isdir"`
}

func checkPathValid(paths []string, pubkey string) error {
	var rentalInfos []RentalInfo
	r := app.DB.Find(&rentalInfos, "pub_key=?", pubkey)
	if r.RowsAffected == 0 {
		log.Println("err")
		return errInvalidPubkey
	}
	roots := make(map[string]struct{})
	for _, path := range paths {
		index := strings.Index(path[1:], "/")
		if index > 0 {
			roots[path[:index+1]] = struct{}{}
		} else {
			roots[path] = struct{}{}
		}
	}
	for root := range roots {
		found := false
		for _, rentInfo := range rentalInfos {
			if root == rentInfo.Root {
				found = true
				break
			}
		}
		if !found {
			log.Println("err:", "invalid root", root)
			return errInvalidPath
		}
	}
	return nil
}

// Function to get
// disk usage of path/disk
func DiskUsage(path string) (all uint64, free uint64, used uint64) {
	fs := syscall.Statfs_t{}
	err := syscall.Statfs(path, &fs)
	if err != nil {
		return
	}
	all = fs.Blocks * uint64(fs.Bsize)
	free = fs.Bfree * uint64(fs.Bsize)
	used = all - free
	return
}

func Spaces(c *gin.Context) {
	pubkey := c.GetString("pubkey")
	var rentalInfos []RentalInfo
	app.DB.Debug().Find(&rentalInfos, "pub_key=?", pubkey)
	for i := range rentalInfos {
		_, _, rentalInfos[i].Used = DiskUsage(filepath.Join(app.MountDir, rentalInfos[i].Root))
	}
	c.JSON(http.StatusOK, gin.H{"code": http.StatusOK, "spaces": rentalInfos})
}

func ListFile(c *gin.Context) {
	path := c.Query("path")
	if path == "" {
		log.Println("err:", "path is nil")
		c.JSON(http.StatusOK, gin.H{"code": StatusParamInvalid})
		return
	}
	pubkey := c.GetString("pubkey")
	if err := checkPathValid([]string{path}, pubkey); err != nil {
		log.Println("err:", err)
		c.JSON(http.StatusOK, gin.H{"code": StatusParamInvalid})
		return
	}
	absPath := filepath.Join(app.MountDir, path)
	files, err := ioutil.ReadDir(absPath)
	if err != nil {
		log.Println("err:", err, absPath)
		c.JSON(http.StatusOK, gin.H{"code": StatusSystem})
		return
	}
	var filterFiles []os.FileInfo
	for _, file := range files {
		if !file.IsDir() {
			filterFiles = append(filterFiles, file)
		}
	}
	files = filterFiles

	sort.Slice(files, func(i, j int) bool {
		return files[i].IsDir()
	})

	fileInfos := make([]*FileInfo, 0)
	for _, file := range files {
		fileInfo := &FileInfo{
			Path:    filepath.Join(path, file.Name()),
			Name:    file.Name(),
			Size:    file.Size(),
			ModTime: file.ModTime().Unix(),
			IsDir:   file.IsDir(),
		}
		fileInfos = append(fileInfos, fileInfo)
	}
	c.JSON(http.StatusOK, gin.H{"code": http.StatusOK, "files": fileInfos})
}

type DeleteParam struct {
	Paths []string `json:"paths"`
}

func DeleteFile(c *gin.Context) {
	var param DeleteParam
	if err := c.ShouldBindJSON(&param); err != nil {
		log.Println("err:", err)
		c.JSON(http.StatusOK, gin.H{"code": StatusParamInvalid})
		return
	}
	pubkey := c.GetString("pubkey")
	if err := checkPathValid(param.Paths, pubkey); err != nil {
		log.Println("err:", err)
		c.JSON(http.StatusOK, gin.H{"code": StatusParamInvalid})
		return
	}
	// 目前只支持单个root内处理
	emptyDir := "/tmp/empty_dir/"
	os.Mkdir(emptyDir, 0755)
	for _, path := range param.Paths {
		absPath := filepath.Join(app.MountDir, path)
		fileInfo, err := os.Stat(absPath)
		if err != nil {
			log.Println("err:", err, absPath)
			continue
		}
		if fileInfo.IsDir() {
			cmd := exec.Command("rsync", "-a", "--delete", emptyDir, filepath.Join(absPath, "/"))
			err := cmd.Run()
			if err != nil {
				log.Println("err:", err)
				c.JSON(http.StatusOK, gin.H{"code": StatusSystem})
				return
			}
			// 如果参数默认带有/, 则认为是清空目录，否则则是删除改目录
			if !strings.HasPrefix(absPath, "/") {
				if err := os.Remove(absPath); err != nil {
					log.Println("err:", err)
					c.JSON(http.StatusOK, gin.H{"code": StatusSystem})
					return
				}
			}
		} else {
			if err := os.Remove(absPath); err != nil {
				log.Println("err:", err)
				c.JSON(http.StatusOK, gin.H{"code": StatusSystem})
				return
			}
		}
	}
	c.JSON(http.StatusOK, gin.H{"code": http.StatusOK})
}

type MkdirParam struct {
	Path string `json:"path"`
}

func Mkdir(c *gin.Context) {
	var param MkdirParam
	if err := c.ShouldBindJSON(&param); err != nil {
		log.Println("err:", err)
		c.JSON(http.StatusOK, gin.H{"code": StatusParamInvalid})
		return
	}
	pubkey := c.GetString("pubkey")
	if err := checkPathValid([]string{param.Path}, pubkey); err != nil {
		log.Println("err:", err)
		c.JSON(http.StatusOK, gin.H{"code": StatusParamInvalid})
		return
	}

	absPath := filepath.Join(app.MountDir, param.Path)
	if err := os.Mkdir(absPath, 0755); err != nil {
		log.Println("err:", err)
		c.JSON(http.StatusOK, gin.H{"code": StatusSystem})
		return
	}
	c.JSON(http.StatusOK, gin.H{"code": http.StatusOK})
}

type ClearParam struct {
	RentVoucher string `json:"rent_voucher" binding:"required"`
	PublicKey   string `json:"pubkey" binding:"required"`
}

func ClearCache(c *gin.Context) {
	var param ClearParam
	if err := c.ShouldBindJSON(&param); err != nil {
		c.JSON(http.StatusOK, gin.H{"result": StatusParamInvalid})
		return
	}
	var rentalInfo RentalInfo
	tx := app.DB.First(&rentalInfo, "voucher = ?", param.RentVoucher)
	if tx.Error != nil || tx.RowsAffected <= 0 {
		log.Println("err:", "invalid voucher", rentalInfo.Voucher)
		c.JSON(http.StatusOK, gin.H{"code": StatusParamInvalid})
		return
	}
	if strings.Compare(rentalInfo.PubKey, param.PublicKey) != 0 {
		log.Println("err:", rentalInfo.PubKey, param.PublicKey)
		c.JSON(http.StatusOK, gin.H{"result": StatusNoPermission})
		return
	}
	tusdPidFile := filepath.Join("/tmp", rentalInfo.Root+".pid")
	stopTusd(tusdPidFile)

	uploadDir := filepath.Join(app.MountDir, rentalInfo.Root, ".uploads")
	os.RemoveAll(uploadDir)
	os.Mkdir(uploadDir, 0755)

	unixAddr := filepath.Join("/tmp", rentalInfo.Root+".sock")
	startTusd(uploadDir, unixAddr, tusdPidFile)

	c.JSON(http.StatusOK, gin.H{"code": http.StatusOK})
}
